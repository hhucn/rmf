/* 
 * File:   TBUS_FastDatarateCalculator.cpp
 * Author: goebel
 * 
 * Created on 31. März 2015, 13:23
 */

#include "TBUS_FastDatarateCalculator.h"

#define DEBUG_FAST_DATARATE_CALCULATOR (false)



// External logging framework
#include "../../pantheios/pantheiosHeader.h"
//some usings to make the code look shorter
using pantheios::log_ERROR;
using pantheios::log_NOTICE;
using pantheios::log_INFORMATIONAL;
using pantheios::log_DEBUG;
using pantheios::integer;
using pantheios::real;
using pantheios::boolean;

#include "../../rmfMakros.h"
//#include "../../networkThreadStructs.h"
#include "../../timeHandler.h"
#include "../measureTBUS.h"
#include <arpa/inet.h>
#include <assert.h>     /* assert - include after rmfMarkos!*/ 



/* --------------------------------------------------------------------------------
 * F A S T D A T A R A T E C A L C U L A T O R
 * -------------------------------------------------------------------------------- */

/**
 * Default constructor
 */
TBUS_FastDatarateCalculator::TBUS_FastDatarateCalculator(const uint32_t minimalPacketSize, const uint32_t maxChirpLength) {
    nLastChirpNr_ = 0;
    nRecBytes_ = 0;
    nPacketCounter_ = 0;
    nCurrReceiveFirstNs_ = 0LL;
    nDatarateLastChirp_ = 0LL;
    nDatarateCurrentChirp_ = 0LL;
    nLossLastChirp_ = 0;
    nLossCurrentChirp_ = 0;
    sp_i = 0;
    sp_0 = minimalPacketSize;
    //currentChirp_.reserve(maxChirpLength);
    currentChirpReceivedPackets_ = 0;
    lastValidDelay = 100000000LL; //start with 100ms delay
}

/**
 * Default destructor
 */
TBUS_FastDatarateCalculator::~TBUS_FastDatarateCalculator() {
}

void TBUS_FastDatarateCalculator::setmTBUS(measureTBUS * tbusPointer) {
    mTBUS_ = tbusPointer;
}

/**
 * calculate the loss rate with send sent packets and received received packets
 * @param received
 * @param send
 * @return lossrate [0..100] * LOSSFACTOR (to allow a higher resolution using uint32_t)
 */
inline uint32_t TBUS_FastDatarateCalculator::calculateLossRate(uint32_t received, uint32_t send) {
    assert(send > 0);
    if (send > 0) {
        return (send - received) * LOSSFACTOR / send;
    } else {
        log_ERROR("TRDD| calculateLossRate called with send==0");
        return 0;
    }
}

/**
 * calculate the datarate 
 * @param bytesReceived
 * @param timespanNs
 * @return datarate in Bytes/s
 */
inline int64_t TBUS_FastDatarateCalculator::calculateDatarate(uint32_t bytesReceived, int64_t timespanNs) {
    assert(timespanNs > 0);
    if (timespanNs > 0) {
        return bytesReceived * BILLION / timespanNs; //bytes/s; ;
    } else {
        log_ERROR("TRDD| calculateDatarate called with timespanNs<=0");
        return 1;
    }
}

/**
 * calculates the nextTtx based in fixedpay and the packetsizes and datarates given
 * @param fixedPay
 * @param a_i
 * @param nextTtx
 * @param nextTtxBasedOnChirpnr
 * @param nextTtxBasedOnPacketnr
 * @param newTtx
 */
void TBUS_FastDatarateCalculator::calculateNextTtxAndSetIt(const TBUSFixedPayload & fixedPay, const int64_t & datarate,
        int64_t & nextTtx, uint32_t & nextTtxBasedOnChirpnr, uint32_t & nextTtxBasedOnPacketnr, bool & newTtx) {
    static int64_t diff;
    static int64_t normalTtx;

    int64_t a_i = datarate;

    if (fixedPay.getPacketnr() > fixedPay.getChirplength() / 20 &&
            fixedPay.getChirpnr() == currentChirpFront_.getChirpnr() && currentChirpReceivedPackets_ >= 2) {
        uint32_t bytesReceived = (currentChirpReceivedPackets_ - 1) * fixedPay.getMeasurementPacketLength();
        uint32_t timespanNs = currentChirpBack_.getReceivetimeNs() - currentChirpFront_.getReceivetimeNs();
        if (0 != timespanNs) {
            a_i = calculateDatarate(bytesReceived, timespanNs);
            if (a_i <= 0) {
                a_i = datarate; //fall back to last valid datarate if we are unlucky with our new guess
            }
        } else {
            log_ERROR("TRDD| timespan is zero ", integer(currentChirpFront_.getChirpnr()), " : ", integer(currentChirpFront_.getPacketnr()), " - ", integer(currentChirpBack_.getPacketnr()));
        }
    }

    if (a_i != 0) {
        int64_t t_erx_sipe_nipe = fixedPay.getReceivetimeNs() + (fixedPay.getChirplength() - fixedPay.getPacketnr()) * fixedPay.getMeasurementPacketLength() * BILLION / a_i;
        int64_t terx_sipz_1 = (t_erx_sipe_nipe + sp_0 * BILLION / a_i) + t_guard;
        int64_t tetx_sipz_1 = terx_sipz_1 - lastValidDelay; //the new ttx
        normalTtx = fixedPay.getSendtimeFirstNs() + measureTBUS::MEASUREMENT_INTERVALL;
        if (tetx_sipz_1 >= normalTtx) { // we do not reduce below the normal measurement interval
            nextTtx = tetx_sipz_1;
            nextTtxBasedOnChirpnr = fixedPay.getChirpnr();
            nextTtxBasedOnPacketnr = fixedPay.getPacketnr();
            newTtx = true;
            diff = nextTtx - normalTtx;
            mTBUS_->dataFileWriterMeasureReceiverNewTtx(fixedPay, nextTtx, normalTtx, diff, newTtx);
        } else {
            //just log it but don't use it
            diff = tetx_sipz_1 - normalTtx;
            mTBUS_->dataFileWriterMeasureReceiverNewTtx(fixedPay, tetx_sipz_1, normalTtx, diff, newTtx);
        }
    } else {
        log_ERROR("TRDD| a_i == 0 ,", integer(currentChirpFront_.getChirpnr()), " : ", integer(currentChirpFront_.getPacketnr()), " - ", integer(currentChirpBack_.getPacketnr()));
    }
}

void TBUS_FastDatarateCalculator::checkForICSICBetweenChirps(const TBUSFixedPayload & fixedPay, const int64_t & a_i,
        int64_t & nextTtx, uint32_t & nextTtxBasedOnChirpnr, uint32_t & nextTtxBasedOnPacketnr, bool & newTtx) {
    static uint32_t lastValidDelayChirpNr = 0;
    static int64_t lastConsoleLog = 0LL;

    //TODO NG: approximate t_gap_i if a_i == 0
    //calculate t_gap_i
    if (nLastChirpNr_ > 0 && a_i != 0LL) {
        assert(a_i > 0);
        uint32_t f_ipe = fixedPay.getPacketnr();
        uint32_t sp_ipe = fixedPay.getMeasurementPacketLength();
        int64_t trx_sipe_fipe = fixedPay.getReceivetimeNs();
        int64_t trx_bf_sipe_1 = trx_sipe_fipe - (sp_0 * BILLION / a_i) - ((f_ipe - 1) * sp_ipe * BILLION / a_i);

        int64_t trx_si_li = currentChirpBack_.getReceivetimeNs();
        uint32_t n_i = currentChirpBack_.getChirplength();
        uint32_t l_i = currentChirpBack_.getPacketnr();

        int64_t trx_si_ni = trx_si_li + (n_i - l_i) * sp_i / a_i;
        int64_t t_gap_i = trx_bf_sipe_1 - trx_si_ni;

        int64_t delta_trx_i = currentChirpBack_.getReceivetimeNs() - currentChirpFront_.getReceivetimeNs();
        int64_t trxdiff = fixedPay.getReceivetimeNs() - currentChirpBack_.getReceivetimeNs();
        int64_t ttxdiff = fixedPay.getSendtimeNs() - currentChirpBack_.getSendtimeNs();

        if (fixedPay.getReceivetimeNs() - lastConsoleLog >= CONSOLE_LOG_THROTTLE) {
            lastConsoleLog = fixedPay.getReceivetimeNs();
            log_NOTICE("TRDD| t_gap_", integer(currentChirpBack_.getChirpnr()), " = ", integer(t_gap_i / 1000000LL),
                    "ms, delta_trx_i=", integer(delta_trx_i / 1000000LL), "ms , trx_diff=", integer(trxdiff / 1000000LL), "ms");
        }

        mTBUS_->dataFileWriterMeasureReceiverGaps(currentChirpBack_, fixedPay, t_gap_i, delta_trx_i, trxdiff, ttxdiff);

        if (t_gap_i > t_guard) {
            if (fixedPay.getPacketnr() <= 5 && lastValidDelayChirpNr < fixedPay.getChirpnr()) {
                //We use the delay of the first arriving packet of a chirp if t_guard is ok and it is one of the first 5 packets of the chirp
                lastValidDelay = fixedPay.getPacketDelay(); // - (fixedPay.getSendtimeNs() - fixedPay.getSendtimeFirstNs());
                lastValidDelayChirpNr = fixedPay.getChirpnr();
                //store this information in the delay logfile
                mTBUS_->dataFileWriterMeasureReceiverDelay(fixedPay);
                //no need to calc nexttx here as this is the first received packet of the chirp and thus can not overrule a nexttx from this chirp
                //and the current chirp seems to arrive in time
            }
        } else {
            // gap too small thus we handle this as if there would be congestion and calculate a new ttx
            calculateNextTtxAndSetIt(fixedPay, a_i, nextTtx, nextTtxBasedOnChirpnr, nextTtxBasedOnPacketnr, newTtx);
        }
    }
}

/**
 * calculates parameters of the current chirp
 * @param payload the payload we just received
 * @param datarate the data rate calculated from the payload
 * @param loss // loss = (1 - received/sent) * LOSSFACTOR
 * @param chirpnr the chirp number of the payload
 * @param packetnr the packetnumber of the payload
 * @return 0 if the datarate and loss should be used and propagated,
 *          -1 if the datarate and loss calculation derived fromt he first 5% of a new chirp
 *          -2 if the datarate was already propagated earlier (first packet of new chirp arrives)
 *          -3 if the packet belonged to a former chirp (out of order packet)
 */
int32_t TBUS_FastDatarateCalculator::calculateDatarateAndNextTtx(const TBUSFixedPayload & fixedPay, int64_t & datarate, uint32_t & loss,
        int64_t & nextTtx, uint32_t & nextTtxBasedOnChirpnr, uint32_t & nextTtxBasedOnPacketnr, bool & newTtx) {

    static int64_t lastConsoleLog = 0LL;
    uint32_t chirpLength = fixedPay.getChirplength();
    uint32_t packetLength = fixedPay.getPacketLength();
    uint32_t receivedChirpNr = fixedPay.getChirpnr();
    uint32_t receivedPacketNr = fixedPay.getPacketnr();

    bool isRelatedToOldChirp = receivedChirpNr < nLastChirpNr_;
    bool isRelatedToNewChirp = receivedChirpNr > nLastChirpNr_;

    newTtx = false;

    // if the packet is out of order, we will do nothing
    if (isRelatedToOldChirp) {
        log_NOTICE("TFDC| TBUS_FastDatarateCalculator received packet, which is related to old chirp ",
                "(current chirp=", integer(nLastChirpNr_), ", packet chirp=", integer(receivedChirpNr), ") ", fixedPay.toStringReceived());
        return -3;
    } else if (isRelatedToNewChirp) {
        //first we calculate the loss rate for the last chirp that we received (we assume no out of order packets)
        if (nLastChirpNr_ > 0) {
            loss = calculateLossRate(nPacketCounter_, nChirpLength_);
            nLossLastChirp_ = loss;
            datarate = nDatarateLastChirp_ = nDatarateCurrentChirp_; //don't calculate a new data rate 
            int64_t packets = nPacketCounter_ <= 1 ? 1 : nPacketCounter_ - 1; //force packets!=0 to avoid floating point exceptin in following log_notice
            assert(packets >= 1);
            if (fixedPay.getReceivetimeNs() - lastConsoleLog >= CONSOLE_LOG_THROTTLE) {
                lastConsoleLog = fixedPay.getReceivetimeNs();

                assert(LOSSFACTOR != 0);
                log_NOTICE("TFDC| last chirp ", integer(nLastChirpNr_), ": datarate=",
                        integer(datarate), " droprate=", real(loss * 100.0 / LOSSFACTOR), "% ,", integer(nPacketCounter_), " / ", integer(nChirpLength_),
                        " x ", integer(nRecBytes_ / packets));
            }
            for (uint32_t i = nLastChirpNr_ + 1; i < receivedChirpNr; i++) { //all intermittent chirps were completely lost
                log_NOTICE("TFDC| chirp ", integer(i),
                        " was completely lost!");
            }
            checkForICSICBetweenChirps(fixedPay, datarate, nextTtx, nextTtxBasedOnChirpnr, nextTtxBasedOnPacketnr, newTtx);
        }
        //log_DEBUG("TRDD| TBUS_FastDatarateCalculator received packet, which is related to new chirp (old chirp=",integer(nChirpNr_), ", new chirp=", integer(receivedChirpNr), ")");

        // reset values
        nLastChirpNr_ = receivedChirpNr;
        nRecBytes_ = 0; //do not count the size of the first received packet for data rate calculation
        nCurrReceiveFirstNs_ = fixedPay.getReceivetimeNs();
        nPacketCounter_ = 1;
        nChirpLength_ = chirpLength;

        //clear the currentChirp_ ex-vector and safe the values we still need
        currentChirpReceivedPackets_ = 1;
        currentChirpFront_ = fixedPay;
        currentChirpBack_ = fixedPay;

        //we can't calculate a data rate with just one packet but we can already calc a new droprate
        nLossCurrentChirp_ = calculateLossRate(nPacketCounter_, receivedChirpNr);
        mTBUS_->dataFileWriterMeasureReceiverDatarate(fixedPay, datarate, loss, false); //datarate is not used
        return -2;
    } else { //(isRelatedToCurrentChirp) 
        //log_DEBUG("TRDD| TBUS_FastDatarateCalculator received packet, which is related to current chirp.");
        ++nPacketCounter_;
        nRecBytes_ += packetLength;
        sp_i = packetLength; // not the first packet of this chirp

        int64_t deltarec = fixedPay.getReceivetimeNs() - nCurrReceiveFirstNs_;
        datarate = calculateDatarate(nRecBytes_, deltarec); //bytes/s
        if (datarate != 0) nDatarateCurrentChirp_ = datarate;

        // loss of current chirp
        loss = calculateLossRate(nPacketCounter_, receivedPacketNr);
        loss = loss < 0 ? 0 : loss > 100 * LOSSFACTOR ? 100 * LOSSFACTOR : loss; // sanity check, because some times packets are overtaking
        assert(loss >= 0);
        assert(loss <= 100 * LOSSFACTOR);
        nLossCurrentChirp_ = loss;

        nLastPacketNr_ = receivedPacketNr;
        currentChirpBack_ = fixedPay;
        currentChirpReceivedPackets_++;

        calculateNextTtxAndSetIt(fixedPay, datarate, nextTtx, nextTtxBasedOnChirpnr, nextTtxBasedOnPacketnr, newTtx);

        assert(LOSSFACTOR != 0LL);
        if (DEBUG_FAST_DATARATE_CALCULATOR) log_NOTICE("TRDD| TBUS_FastDatarateCalculator current chirp ", integer(nLastChirpNr_), ":", integer(receivedPacketNr), " datarate=",
                integer(datarate), " droprate=", real(loss * 100.0 / LOSSFACTOR), "%");


        mTBUS_->dataFileWriterMeasureReceiverDatarate(fixedPay, datarate, loss, true); //datarate is used
        return 0;
    }
}

/**
 * calculates parameters of the current chirp
 * @param payload the payload we just received
 * @param datarate the data rate calculated from the payload
 * @param loss // loss = (1 - received/sent) * LOSSFACTOR
 * @param chirpnr the chirp number of the payload
 * @param packetnr the packetnumber of the payload
 * @return 0 if the datarate and loss should be used and propagated,
 *          -1 if the datarate and loss calculation derived fromt he first 5% of a new chirp
 *          -2 if the datarate was already propagated earlier (first packet of new chirp arrives)
 *          -3 if the packet belonged to a former chirp (out of order packet)
 */
int32_t TBUS_FastDatarateCalculator::calculateDatarate(const TBUSFixedPayload & fixPay, int64_t & datarate, uint32_t & loss) {
    uint32_t chirpLength = fixPay.getChirplength();
    uint32_t packetLength = fixPay.getPacketLength();
    uint32_t receivedChirpNr = fixPay.getChirpnr();
    uint32_t receivedPacketNr = fixPay.getPacketnr();

    bool isRelatedToOldChirp = receivedChirpNr < nLastChirpNr_;
    bool isRelatedToNewChirp = receivedChirpNr > nLastChirpNr_;
    //bool isRelatedToCurrentChirp = receivedChirpNr == nChirpNr_;

    // if the packet is out of order, we will do nothing
    if (isRelatedToOldChirp) {
        log_NOTICE("TRDD| TBUS_FastDatarateCalculator received packet, which is related to old chirp ",
                "(current chirp=", integer(nLastChirpNr_), ", packet chirp=", integer(receivedChirpNr), ") ", fixPay.toStringReceived());
        return -3;
    } else if (isRelatedToNewChirp) {
        //first we calculate the loss rate for the last chirp that we received (we assume no out of order packets)
        if (nLastChirpNr_ > 0) {
            loss = calculateLossRate(nPacketCounter_, nChirpLength_);
            nLossLastChirp_ = loss;
            datarate = nDatarateLastChirp_ = nDatarateCurrentChirp_; //don't calculate a new data rate 
            assert(LOSSFACTOR != 0LL);
            log_NOTICE("TRDD| TBUS_FastDatarateCalculator last chirp ", integer(nLastChirpNr_), ": datarate=",
                    integer(datarate), " droprate=", real(loss * 100.0 / LOSSFACTOR), "% ", integer(nPacketCounter_), " / ", integer(nChirpLength_));

            for (uint32_t i = nLastChirpNr_ + 1; i < receivedChirpNr; i++) { //all intermittent chirps were completely lost
                log_NOTICE("TRDD| TBUS_FastDatarateCalculator chirp ", integer(i),
                        " was completely lost!");
            }
        }
        //log_DEBUG("TRDD| TBUS_FastDatarateCalculator received packet, which is related to new chirp (old chirp=",integer(nChirpNr_), ", new chirp=", integer(receivedChirpNr), ")");

        // reset values
        nLastChirpNr_ = receivedChirpNr;
        nRecBytes_ = 0; //do not count the size of the first received packet for data rate calculation
        nCurrReceiveFirstNs_ = fixPay.getReceivetimeNs();
        nPacketCounter_ = 1;
        nChirpLength_ = chirpLength;

        //we can't calculate a data rate with just one packet but we can already calc a new droprate
        nLossCurrentChirp_ = calculateLossRate(nPacketCounter_, receivedChirpNr);
        return -2;
    } else { //(isRelatedToCurrentChirp) {
        //log_DEBUG("TRDD| TBUS_FastDatarateCalculator received packet, which is related to current chirp.");
        ++nPacketCounter_;
        nRecBytes_ += packetLength;

        int64_t deltarec = fixPay.getReceivetimeNs() - nCurrReceiveFirstNs_;
        datarate = calculateDatarate(nRecBytes_, deltarec); //bytes/s
        nDatarateCurrentChirp_ = datarate;

        // loss of current chirp
        loss = calculateLossRate(nPacketCounter_, receivedPacketNr);
        loss = loss < 0 ? 0 : loss > 100 * LOSSFACTOR ? 100 * LOSSFACTOR : loss; // sanity check, because some times packets are overtaking
        assert(loss >= 0);
        assert(loss <= 100 * LOSSFACTOR);
        assert(LOSSFACTOR != 0LL);
        nLossCurrentChirp_ = loss;
        if (DEBUG_FAST_DATARATE_CALCULATOR) log_NOTICE("TRDD| TBUS_FastDatarateCalculator current chirp ", integer(nLastChirpNr_), ":", integer(receivedPacketNr), " datarate=",
                integer(datarate), " droprate=", real(loss * 100.0 / LOSSFACTOR), "%");

        //only propagate datarate if packet is not in the first 5% of a chirp as this might introduce high variance
        if (receivedPacketNr < nChirpLength_ / 20) {
            return -1;
        } else {
            return 0;
        }
    }
}



