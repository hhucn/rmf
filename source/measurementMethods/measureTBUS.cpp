/**
 * @authors Tobias Krauthoff <tobias.krauthoff@hhu.de, krauthoff@cs.uni-duesseldorf.de>
 * @authors Nobert Goebel <goebel@cs.uni-duesseldorf.de>
 * @date 2015-03-25
 * Adaptive measurment method with self induced congestion detection
 * original version by Tobias Krauthoff
 * modified by Norbert Goebel
 * INFORMATION: The countdown timer is currently not used!
 */

#include "measureTBUS.h"

// External logging framework
#include "../pantheios/pantheiosHeader.h"

// Includes RMF header files
#include "../inputHandler.h"
#include "../networkHandler.h"
#include "../networkThreadStructs.h"
#include "../resultCalculator.h"
//#include "../exttimespec.h"
#include <iomanip>
#include <deque>
#include <errno.h>
#include <cmath>
#include <arpa/inet.h> //ntohl
#include "../packetPayloads/TBUSFixedPayload.h"
#include "../packetPayloads/TBUSDynamicPayload.h"
#include <stdlib.h> //radn())
#include "../rmfMakros.h"
#include <assert.h>     /* assert - include after rmfMarkos!*/ 

// needed for the alix-boards
#ifndef INT32_MAX
#define INT32_MAX 2147483647
#endif

//some usings to make the code look shorter
using pantheios::log_ERROR;
using pantheios::log_NOTICE;
using pantheios::log_INFORMATIONAL;
using pantheios::log_DEBUG;
using pantheios::integer;
using pantheios::real;
using pantheios::boolean;


pthread_mutex_t measureTBUS_backchannel_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t measureTBUS_safeguard_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t measureTBUS_fillsendingQueue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t measureTBUS_safeguard_timeout_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t measureTBUS_nextttx_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t measureTBUS_nextTtx_Emergency_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t measureTBUS_safeguard_timeout_cond = PTHREAD_COND_INITIALIZER;

//static variables
const uint32_t measureTBUS::ETHERNET_MTU_SIZE = 1500; // in bytes
//const int64_t measureTBUS::MEASUREMENT_INTERVALL = 1000000000LL; //one second
const int64_t measureTBUS::MEASUREMENT_INTERVALL = 200000000LL; //200 ms

const string SEP = ",";

/**
 * measureTBUS standard constructor.
 * Initializes the instance with default values.
 */
measureTBUS::measureTBUS() {
    // set for debugging purpose
    cout.setf(ios::fixed);
    cout.precision(10);
    initValues();
    // exttimespec::testing();

    //just some Payloadtests:
    //    TBUSFixedPayload fixedPayload(0x0001020304050607LL, 0x08090a0b0c0d0e0fLL, 0x10111213L, 0x14151617L, 0x18191a1bL, 0x1c1d1e1fL); //eins, zwei, drei, vier, fuenf, sechs);
    //    char buffer[500];
    //    memset(&buffer, 0, 500);
    //    fixedPayload.insertPayloadInPacket(buffer);
    //    //&buffer[0]) == buffer !!!
    //    TBUSFixedPayload fp2(buffer);
    //    for (uint32_t i = 0; i < fp2.getPayloadSize(); i++) {
    //        if (((int) buffer[i]) != i) {
    //            std::cout << "buf[" << i << "] != " << (int) buffer[i] << "<===\n";
    //        } else {
    //            std::cout << "buf[" << i << "] == " << (int) buffer[i] << "\n";
    //        }
    //    }
    //
    //    TBUSDynamicPayload dp1(0x0001020304050607LL, 0x08090a0b0c0d0e0fLL, 0x1011121314151617LL, 0x18191a1bL, 0x1c1d1e1fL, 0x20212223L, 0x24252627L, 0x28292a2b);
    //    dp1.insertPayloadInPacket(&buffer[100]);
    //    TBUSDynamicPayload dp2(&buffer[100]);
    //    for (uint32_t i = 0; i < dp2.getPayloadSize() + 100; i++) {
    //        if (((int) buffer[100 + i]) != i) {
    //            std::cout << "buf[" << 100 + i << "] != " << (int) buffer[100 + i] << "<===\n";
    //        } else {
    //            std::cout << "buf[" << 100 + i << "] == " << (int) buffer[100 + i] << "\n";
    //        }
    //    }
    //    std::cout << "Finished testing \n";
}

/**
 * measureTBUS standard destructor.
 * Closes the sender and receiver logfiles.
 */
measureTBUS::~measureTBUS() {

    // delete arrays
    // ...

    // destroy all condition variables;
    if (pthread_cond_destroy(&measureTBUS_nextTtx_Emergency_cond) == EBUSY)
        log_ERROR("MEME| Cannot destroy condition variable which was for the safeguard channel to signal congestion, because it is busy!");
    if (pthread_cond_destroy(&measureTBUS_safeguard_timeout_cond) == EBUSY)
        log_ERROR("MEME| Cannot destroy condition variable which was for the safeguard channel to signal positive feedback, because it is busy!");

    // just for the case, do it again
    closeEveryFile();
    stopRunningThreads();
}

/**
 * Initializes the object variables to their default values
 */
int32_t measureTBUS::initValues() {
    // booleans
    syncingDone_ = false; //syncing of the measurement parameters did not finish yet
    isAdaptive_ = true; // true, when chirp properties should be changed, based on data rate
    isLoggingDone_ = false; // true, when logging was done
    isUseOfCongestionDetector_ = true; // use if sicd
    adjustOnWrongInitialDatarate_ = false; //if set to true the sending rate might be adjusted above the initially given maxsendingrate
    //disabled cause it negatively influcences cellular network measurements

    // all numbers
    nChirpLength_ = 0; // depends on data rate and will be calculated
    nCurrentDatarate_ = 16 * 1000; // small value, because slow start (if chaning, look in get_latestInformationOutOfChannels)
    nInitialDatarate_ = 16 * 1000; // start data rate, needed for times after self induced congestion
    ExpEndtime_ = 0; // will defined later
    ExpStarttime_ = 0; // will defined later
    nChannelload_ = 50; // 50 percent of channel utiliziation for getting a good spreading factor
    nCountSmoothening_ = 3; // value from the result calculator before it was in his own class
    nCheckSicEveryKthPacket_ = 25; // check for SIC every kth packet in current chirp
    nFastDataCalcDatarate_ = 0LL; // will be set by the fast data calculator
    nMaximalChirpLengthDownlink_ = 100000; // maximal chirp length for the dowload
    nMaximalChirpLengthUplink_ = 50000; // maximal chirp length for the upload
    nMaximalDatarateDownlink_ = 1500 * 1000; // maximal data rate for the download
    nMaximalDatarateUplink_ = 1500 * 1000; // maximal data rate for the upload
    nPacktIdOfCurrentCongestedPacket_ = 0; // packet of the current packet, where congestion was detected for
    nPacketSize_ = 1000; // hader size: packet size - IP-Header - UDP_Header - Own Header - dynamic payload
    nPacketHeaderSizeEthernetIpUdp_ = network.getHeaderSizeEthernetIpUdp();
    nMinimalPacketSize_ = nPacketHeaderSizeEthernetIpUdp_ + TBUSFixedPayload::payloadsize_ + TBUSDynamicPayload::payloadsize_;
    ProcessTimer_ = 5000000LL; // time for waking up in fill sending queue; 5ms

    // other variables and timestructs
    lastSafeguardPacketBecauseCongestionSent_ = 0; // zero because we have not sent anything yet
    lastSafeguardPacketBecauseTimeoutSent_ = 0; // zero because we have not sent anything yet
    lastTimeToSleep_ = 0; // last value of the sleep time
    nextTimeToSleep_ = 0; // next value of the sleeptime, needed for feedback
    lastAbsoluteTimeToSleep_ = 0; // last absoulte timestamp after last sleeptime
    lastReceivedPacket_.id = 0; // if of the last received packet
    //lastChirpStart_ = 0; // sent time of the last chirp
    nextTtxUpstream_ = 0; //sent time of our next chirp
    lastReceivedPacket_.complete_payload = NULL; // alloc pointer with zero

    //    // set callback class and network handler
    //    countdownTimerFeedback_.set_callbackClass(this);
    //    countdownTimerTimeout_.set_callbackClass(this);
    //    countdownTimerTimeout_.set_callbackReason(countdownTimerTimeout_.CALLBACK_REASON_TIMEOUT);
    // DEPRECATED
    //    // init backchannel
    //    pthread_mutex_lock(&measureTBUS_backchannel_mutex);
    //    backChannel_send_ = TBUS_BackChannel();
    //    backChannel_received_ = TBUS_BackChannel();
    //    pthread_mutex_unlock(&measureTBUS_backchannel_mutex);
    fastDatarateCalculator_ = TBUS_FastDatarateCalculator(nMinimalPacketSize_, ((inputH.get_nodeRole() == 0) ? nMaximalChirpLengthDownlink_ : nMaximalChirpLengthDownlink_));


    //    // init safeguardchannel, when choosen
    //    if (inputH.get_safeguardChannel() == 1) {
    //        pthread_mutex_lock(&measureTBUS_safeguard_mutex);
    //        //DEPRECATED        safeguardChannel_send_ = TBUS_SafeguardChannel();
    //        //        safeguardChannel_received_ = TBUS_SafeguardChannel();
    //        pthread_mutex_unlock(&measureTBUS_safeguard_mutex);
    //    }

    return 0;
}

/*
 * Closes output files
 * @return 0
 */
int32_t measureTBUS::closeEveryFile() {
    log_DEBUG("MEME| Entering closeEverything()");

    outputFileReceiverRaw_.close();
    outputFileSenderRaw_.close();
    safeguardFileReceiver_.close();
    safeguardFileSender_.close();
    outputFileReceiverDelay_.close();
    outputFileReceiverDatarate_.close();
    outputFileReceiverNewTtx_.close();
    outputFileReceiverGaps_.close();
    outputFileSenderNewTtx_.close();

    log_DEBUG("MEME| Leaving closeEverything()");

    return 0;
}

/**
 * Stops gpsHelper, ntpHelper and countdownTimer
 * @return 0
 */
int32_t measureTBUS::stopRunningThreads() {


    //DEPRECATED as restarting ntp is a no go during a measurement
    //    // stop ntp helper
    //    if (ntpHelper_.isRunning()) {
    //        ntpHelper_.stop();
    //        if (ntpHelper_.join() == -1) {
    //            log_DEBUG("MEME| Error while joining ntpHelper_ thread");
    //        }
    //    } else {
    //        log_DEBUG("MEME| Could not join gpsHelper thread, beacuse it is not running");
    //    }

    //    // stop countdowntimer
    //    if (countdownTimerFeedback_.isRunning()) {
    //        countdownTimerFeedback_.stop();
    //        if (countdownTimerFeedback_.join() == -1) {
    //            log_DEBUG("MEME| Error while joining countdownTimer thread");
    //        }
    //    } else {
    //        log_DEBUG("MEME| Could not join countdownTimer thread, beacuse it is not running");
    //    }

    return 0;
}

/**
 * Opens all sender and receiver logfiles
 * @return 0 when all logfiles are open, -1 otherwise
 */
int32_t measureTBUS::openLogfiles() {
    if (openLogfile_measureSender() == -1) return -1;
    if (openLogfile_measureReceiver() == -1) return -1;
    if (openLogfile_measureReceiverDelay() == -1) return -1;
    if (openLogfile_measureReceiverDatarate() == -1) return -1;
    if (openLogfile_measureReceiverNewTtx() == -1) return -1;
    if (openLogfile_measureReceiverGaps() == -1) return -1;
    if (openLogfile_measureSenderNewTtx() == -1) return -1;
    if (inputH.get_safeguardChannel() == 1) {
        if (openLogfile_safeguardReceiver() == -1) return -1;
        if (openLogfile_safeguardSender() == -1) return -1;
    }
    return 0;
}

/**
 * Generic method to open a logfile inputH.get_logfilePath()/nodeRole.fileNameSuffix
 * and paste header for the file afterwards
 * @param fileNameSuffix
 * @param file
 * @param header 
 * @param precision the precision the output should have
 * @return 0 in success, -1 if opening the file failed
 */
int32_t measureTBUS::openLogFile(const string fileNameSuffix, ofstream& file, const string& header, const int precision) {

    string logfile = inputH.get_logfilePath() + (inputH.amIServer() ? "server." : "client.") + fileNameSuffix;
    // Initialize OutputFile
    //file.open(logfile.c_str(), ios::out);
    file.open(logfile.c_str(), ios::app); 
    if (!file.is_open()) {
        log_ERROR("MEME| Error: Creating \"", logfile.c_str(), "\" failed.");
        return -1;
    }

    file.setf(ios::fixed);
    file << setprecision(precision);
    file << "# " << header << "\n";
    return 0;
}

/*
 * Opens logfiles for raw data from receiver
 * @return 0 on success, -1 when an error occured
 */
int32_t measureTBUS::openLogfile_measureReceiver() {
    log_DEBUG("MEME| Entering openLogfileMeasureReceiver()");
    payloadStruct pay;
    bool ret = openLogFile("mReceiverRawData", outputFileReceiverRaw_, pay.getStringHeaderReceived(), 10);
    log_DEBUG("MEME| Leaving openLogfileMeasureReceiver()");
    return ret;
}

/*
 * Opens logfiles for raw data from receiver
 * @return 0 on success, -1 when an error occured
 */
int32_t measureTBUS::openLogfile_measureSenderNewTtx() {
    string name = "mSenderNewTtx";
    string header = "#Logs the calculated nexTtx for the incoming direction after each incoming packet incoming packet and marks if this is later than normal.\n"
            "#bochirp,bopnr,newTTx,normalTtx,difference,sendlater(bool)";

    log_DEBUG("MEME| Entering openLogfile ", name);
    bool ret = openLogFile(name, outputFileSenderNewTtx_, header, 10);
    log_DEBUG("MEME| Leaving openLogfile ", name);
    return ret;
}

/*
 * Opens logfiles for raw data from receiver
 * @return 0 on success, -1 when an error occured
 */
int32_t measureTBUS::openLogfile_measureReceiverNewTtx() {
    string name = "mReceiverNewTtx";
    string header = "#Logs the incoming newTtx in the dynamic payload and checks if this is affecting our nextttx\n"
            "#Chirp,Packet,     ttx[ns]     ,      trx[ns]      ,     newTTx[ns]    ,   normalTtx[ns]   , diff[ns]   , sendlater(bool)";
    //               "    1,    2,1432220380676515000,1432220378671185000,1432220380866335906,1432220381676346000,-810010094,0
    log_DEBUG("MEME| Entering openLogfile ", name);
    bool ret = openLogFile(name, outputFileReceiverNewTtx_, header, 10);
    log_DEBUG("MEME| Leaving openLogfile ", name);
    return ret;
}

/**
 * Logs delay information for received chirps as long as they produced valid results
 * @param fixedPay
 * @return always 0
 */
int32_t measureTBUS::dataFileWriterMeasureReceiverNewTtx(const TBUSFixedPayload & fixedpay, const int64_t newTtx, const int64_t normalTtx, const int64_t diff, const bool sendLater) {
    outputFileReceiverNewTtx_
            << setw(5) << fixedpay.getChirpnr() << SEP << setw(5) << fixedpay.getPacketnr() << SEP
            << fixedpay.getSendtimeNs() << SEP << fixedpay.getReceivetimeNs() << SEP
            << newTtx << SEP << normalTtx << SEP << setw(12) << diff << SEP << sendLater << "\n";
    return 0;
}

/*
 * Opens logfiles for raw data from receiver
 * @return 0 on success, -1 when an error occured
 */
int32_t measureTBUS::openLogfile_measureReceiverGaps() {
    string name = "mReceiverGaps";
    string header = "Logs the predicted or final gaps between incoming chirps\n"
            "#                    chirp i                       |               chirp j>=i+1                        |        chirp i          |    chirp i,j\n"
            "#Chirp, Pack,      ttx[ns]     ,      trx[ns]      ,chirp, pack,      ttx[ns]      ,      trx[ns]      , t_gap_i[ns],deltatrx[ns], trxdiff[ns], ttxdiff[ns]";
    log_DEBUG("MEME| Entering openLogfile ", name);
    bool ret = openLogFile(name, outputFileReceiverGaps_, header, 10);
    log_DEBUG("MEME| Leaving openLogfile ", name);
    return ret;
}

/**
 * Logs delay information for received chirps as long as they produced valid results
 * @param fixedPay
 * @return always 0
 */
int32_t measureTBUS::dataFileWriterMeasureReceiverGaps(const TBUSFixedPayload & back, const TBUSFixedPayload & next, const int64_t t_gap_i, const int64_t delta_trx_i, const int64_t trxdiff, const int64_t ttxdiff) {
    outputFileReceiverGaps_
            << setw(5) << back.getChirpnr() << SEP << setw(5) << back.getPacketnr() << SEP
            << back.getSendtimeNs() << SEP << back.getReceivetimeNs() << SEP
            << setw(5) << next.getChirpnr() << SEP << setw(5) << next.getPacketnr() << SEP
            << next.getSendtimeNs() << SEP << next.getReceivetimeNs() << SEP
            << setw(12) << t_gap_i << SEP << setw(12) << delta_trx_i << SEP
            << setw(12) << trxdiff << SEP << setw(12) << ttxdiff << "\n";
    return 0;
}

/*
 * Opens logfiles for raw data from receiver
 * @return 0 on success, -1 when an error occured
 */
int32_t measureTBUS::openLogfile_measureReceiverDelay() {
    string name = "mReceiverDelay";
    string header = "Logs the valid delay of the current chirp estimates after each received packet\n"
            "#Chirp,Pack,      ttx[ns]      ,      trx[ns]      , delay[ns]";
    log_DEBUG("MEME| Entering openLogfile ", name);
    bool ret = openLogFile(name, outputFileReceiverDelay_, header, 10);
    log_DEBUG("MEME| Leaving openLogfile ", name);
    return ret;
}

/**
 * Logs delay information for received chirps as long as they produced valid results
 * @param fixedPay
 * @return always 0
 */
int32_t measureTBUS::dataFileWriterMeasureReceiverDelay(const TBUSFixedPayload & fixedpay) {
    outputFileReceiverDelay_
            << setw(5) << fixedpay.getChirpnr() << SEP << setw(5) << fixedpay.getPacketnr() << SEP
            << fixedpay.getSendtimeNs() << SEP << fixedpay.getReceivetimeNs() << SEP
            << setw(10) << fixedpay.getPacketDelay() << "\n";
    return 0;
}

/*
 * Opens logfiles for raw data from receiver
 * @return 0 on success, -1 when an error occured
 */
int32_t measureTBUS::openLogfile_measureReceiverDatarate() {
    string name = "mReceiverDatarate";
    string header = "#Logs the valid datarate of the current chirp estimates after each received packet\n"
            "#chirp,pack,     ttx[ns]       ,      trx[ns]      ,Rate[Byte/s],   loss[%]    , datarate used(bool)";
    log_DEBUG("MEME| Entering openLogfile ", name);
    bool ret = openLogFile(name, outputFileReceiverDatarate_, header, 10);
    log_DEBUG("MEME| Leaving openLogfile ", name);
    return ret;
}

/**
 * Logs delay information for received chirps as long as they produced valid results
 * @param fixedPay
 * @return always 0
 */
int32_t measureTBUS::dataFileWriterMeasureReceiverDatarate(const TBUSFixedPayload & fixedpay, int64_t datarate, uint32_t lossrate, bool used) {
    outputFileReceiverDatarate_
            << setw(5) << fixedpay.getChirpnr() << SEP << setw(5) << fixedpay.getPacketnr() << SEP
            << fixedpay.getSendtimeNs() << SEP << fixedpay.getReceivetimeNs() << SEP
            << setw(12) << datarate << SEP << setw(12) << ((100.0 * lossrate) / LOSSFACTOR) << SEP << used << "\n";
    return 0;
}

/*
 * Opens logfiles for raw data from sender
 * @return 0 on success, -1 when an error occured
 */
int32_t measureTBUS::openLogfile_measureSender() {
    log_DEBUG("MEME| Entering openLogfileMeasureSender()");
    payloadStruct pay;
    bool ret = openLogFile("mSenderRawData", outputFileSenderRaw_, pay.getStringHeaderSend(), 10);
    log_DEBUG("MEME| Leaving openLogfileMeasureSender()");
    return ret;
}

/*
 * Opens safeguard logfiles for raw data from receiver
 * @return 0 on success, -1 when an error occured
 */
int32_t measureTBUS::openLogfile_safeguardReceiver() {
    log_DEBUG("MEME| Entering openLogfileSafeguardReceiver()");
    TBUSDynamicPayload pay;
    bool ret = openLogFile("sReceiverData", safeguardFileReceiver_, pay.getStringHeaderReceived(), 10);
    log_DEBUG("MEME| Leaving openLogfileSafeguardReceiver()");
    return ret;
}

/*
 * Opens safeguard logfiles for raw data from sender
 * @return 0 on success, -1 when an error occured
 */
int32_t measureTBUS::openLogfile_safeguardSender() {
    log_DEBUG("MEME| Entering openLogfileSafeguardSender()");
    TBUSDynamicPayload pay;
    bool ret = openLogFile("sSenderData", safeguardFileSender_, pay.getStringHeaderSend(), 10);
    log_DEBUG("MEME| Leaving openLogfileSafeguardSender()");
    return ret;
}

/**
 * Logs all information contained in send packets transfered using the measurement channel
 * @param packeyLoad contains all the information
 * @return always 0
 */
int32_t measureTBUS::dataFileWriterMeasureSender(const payloadStruct & payload) {
    static class timeHandler timing(inputH.get_timingMethod());
    static int64_t lastConsoleLog = 0LL;
    static uint32_t currentChirp = 0;
    static TBUSFixedPayload fix = payload.fixPay;

    if (currentChirp < payload.fixPay.getChirpnr()) { //get some statistics about the send durations for the chirps
        if (currentChirp != 0) {
            if (payload.fixPay.getReceivetimeNs() - lastConsoleLog >= CONSOLE_LOG_THROTTLE) { //throttle the logging
                lastConsoleLog = payload.fixPay.getReceivetimeNs();
                log_NOTICE("MEME| Sending chirp ", integer(currentChirp), " took ", integer((fix.getSendtimeNs() - fix.getSendtimeFirstNs()) / 1000000LL), " ms");
            }
        }
        currentChirp = payload.fixPay.getChirpnr();
    }
    fix = payload.fixPay;

    if (DEBUG_DATAFILEWRITER) log_DEBUG("MEME| Entering dataFileWriter_measureSender()");
    outputFileSenderRaw_ << payload.fixPay.toStringSend(SEP) << SEP <<
            payload.dynPay.toStringSend(SEP) << "\n";
    if (DEBUG_DATAFILEWRITER) log_DEBUG("MEME| Leaving dataFileWriter_measureSender()");
    return 0;
}

/**
 * Logs all information contained in received packets transfered using the measurement channel
 * @param packeyLoad contains all the information
 * @return always 0
 */
int32_t measureTBUS::dataFileWriterMeasureReceiver(const payloadStruct & payload) {
    if (DEBUG_DATAFILEWRITER) log_DEBUG("MEME| Entering dataFileWriter_measureSender()");
    outputFileReceiverRaw_ << payload.fixPay.toStringReceived(SEP) << SEP <<
            payload.dynPay.toStringReceived(SEP) << "\n";
    if (DEBUG_DATAFILEWRITER) log_DEBUG("MEME| Leaving dataFileWriter_measureSender()");
    return 0;
}

/**
 * Loggs all informations out of udp_safeguard_receiveQueue (ids, sendtime)
 *
 * @param recSec, seconds when the packet was send
 * @param recNanosec, nanoseconds when the packet was send
 *
 * @return 0
 */
int32_t measureTBUS::dataFileWriterSafeguardReceiver(const TBUSDynamicPayload & dynpay) {
    if (DEBUG_DATAFILEWRITER) log_DEBUG("MEME| Entering dataFileWriter_safeguardReceiver()");
    safeguardFileReceiver_ << dynpay.toStringReceived(SEP) << "\n";
    if (DEBUG_DATAFILEWRITER) log_DEBUG("MEME| Leaving dataFileWriter_safeguardReceiver()");
    return 0;
}

/**
 * Loggs all informations out of udp_safeguard_sendQueue (ids, sendtime)
 *
 * @param recSec, seconds when the packet was send
 * @param recNanosec, nanoseconds when the packet was send
 * @return 0
 */
int32_t measureTBUS::dataFileWriterSafeguardSender(const TBUSDynamicPayload & dynpay) {
    if (DEBUG_DATAFILEWRITER) log_DEBUG("MEME| Entering dataFileWriter_safeguardSender()");
    safeguardFileSender_ << dynpay.toStringSend(SEP) << "\n";
    if (DEBUG_DATAFILEWRITER) log_DEBUG("MEME| Leaving dataFileWriter_safeguardSender()");
    return 0;
}


/**
 * Checks the status of the network threads (so if some passeddied)
 *
 * @return 0, if everything is ok, >0, otherweiseif any thread died
 */
int32_t measureTBUS::get_countOfDeadThreads() {
    int terminatedThreads = network.getThreadStatus("MEME");
    if (terminatedThreads == 0) return 0;

    // Uplink => Client sending, server receiving   ||   => Both send and receive
    if ((inputH.get_measuredLink() == 0 && inputH.get_nodeRole() == 1) || (inputH.get_measuredLink() == 2)) {
        if ((terminatedThreads & 0x2) == 2) log_ERROR("MEME| mSendingThread died before its time ! Stopping experiment");
        if ((terminatedThreads & 0x8) == 8) log_ERROR("MEME| senderDataDistributer died before its time ! Stopping experiment");
    }

    // Downlink => Server sending, Client receiving   ||   Both     => Both send and receive
    if ((inputH.get_measuredLink() == 1 && inputH.get_nodeRole() == 0) || (inputH.get_measuredLink() == 2)) {
        if ((terminatedThreads & 0x1) == 1) log_ERROR("MEME| mReceivingThread died before its time ! Stopping experiment");
        if ((terminatedThreads & 0x4) == 4) log_ERROR("MEME| receiverDataDistributer died before its time ! Stopping experiment");
    }

    // Sending or receiving in safeguard thread ?
    if (inputH.get_safeguardChannel() == 1) {
        if ((terminatedThreads & 0x10) == 16) log_ERROR("MEME| sSendingThread died before its time ! Stopping experiment");
        if ((terminatedThreads & 0x20) == 32) log_ERROR("MEME| sReceivingThread died before its time ! Stopping experiment");
    }

    log_DEBUG("MEME| Leaving fillSendingQueue(deadThread)");
    return (terminatedThreads);
}


/* --------------------------------------------------------------------------------
 * F I L L   S E N D I N G   Q U E U E   A N D   C A L C U L A T I O N S
 * -------------------------------------------------------------------------------- */

/*
 * Complex process: Calculates some chirp parameters and enqueues the chirp for sending
 * @return 0 on success, -1 when an error occured
 */
int32_t measureTBUS::fillSendingQueue() {
    if (DEBUB_FILLSENDINGQUEUE) log_DEBUG("MEME| Entering fillSendingQueue()");
    static int64_t lastConsoleLog = 0LL;
    // We dont need to be complete accurate with the ending time, ignore nanoseconds
    static class timeHandler timing(inputH.get_timingMethod());
    timing.probeTime();
    ExpEndtime_ = timing.getLastProbedTimeNs() + inputH.get_experimentDuration() * BILLION;
    if (DEBUB_FILLSENDINGQUEUE) log_DEBUG("MEME| Experiment end is current time additional experiment duration: ", integer(ExpEndtime_));
    //log_NOTICE("MEME| Safeguard packet send     ")

    // variables for "sending-loop"s
    uint32_t basedOnChirp = 0;
    uint32_t basedOnPacket = 0;
    int64_t datarate = nCurrentDatarate_;

    pthread_mutex_lock(&measureTBUS_nextttx_mutex);
    nextChirpnr_ = 1;
    nextTtxUpstreamNormal_ = timing.getLastProbedTimeNs(); // initialize the sendtime for the first chirp 
    nextTtxUpstream_ = timing.getLastProbedTimeNs(); // initialize the sendtime for the first chirp 
    pthread_mutex_unlock(&measureTBUS_nextttx_mutex);

    //    bool hasSleptDueToCongestion = false;
    timespec sleeptime;
    int64_t nextChirpTtx;
    bool waitingFinished;
    int ret_pctw = 0;

    while (true) {
        // wait for sending - send ProcessTimer_ before next send event
        waitingFinished = false;
        while (!waitingFinished) {
            pthread_mutex_lock(&measureTBUS_nextttx_mutex);
            nextChirpTtx = nextTtxUpstream_; //get the current nextTtxUpsteam which might be changed during waiting!
            pthread_mutex_unlock(&measureTBUS_nextttx_mutex);
            timing.probeTime();
            if (timing.getLastProbedTimeNs() >= nextChirpTtx - ProcessTimer_) {
                waitingFinished = true;
                if (timing.getLastProbedTimeNs() > nextChirpTtx && nextChirpnr_ != 1) {
                    if (ret_pctw == ETIMEDOUT) {
                        log_ERROR("MEME| nexttx already passed after waiting in fillsendingqueue - raise ProcessTimer_! ", real((timing.getLastProbedTimeNs() - nextChirpTtx) / 1000000.0), "ms");
                    } else {
                        log_NOTICE("MEME| nexttx already passed - wake up by dynpay was to late! ", real((timing.getLastProbedTimeNs() - nextChirpTtx) / 1000000.0), "ms");
                    }
                }
            } else {
                sleeptime = timeHandler::ns_to_timespec(nextChirpTtx - (15LL * ProcessTimer_) / 10LL); // calculate sleep time and good night
                if (DEBUB_FILLSENDINGQUEUE) log_DEBUG("MEME| Wait Time while filling sending queue: ", timeHandler::timespecToString(sleeptime));
                // wait for signal, which might be send by setNewTtxForNextChirp(), or until the timer expires

                if (DEBUB_FILLSENDINGQUEUE) log_DEBUG("MEME| fillsending sleep now,nexttx,sleeptime ", integer(timing.getLastProbedTimeNs()), " , ", integer(nextChirpTtx), " , ", timeHandler::timespecToString(sleeptime));
                pthread_mutex_lock(&measureTBUS_fillsendingQueue_mutex);
                ret_pctw = pthread_cond_timedwait(&measureTBUS_nextTtx_Emergency_cond, &measureTBUS_fillsendingQueue_mutex, &sleeptime);
                pthread_mutex_unlock(&measureTBUS_fillsendingQueue_mutex);
                if (DEBUB_FILLSENDINGQUEUE) log_DEBUG("MEME| fillsending now - sleeptime = ", integer(timing.getCurrentTimeNs() - timeHandler::timespec_to_ns(sleeptime)));
            }
        }

        //check if the sendingqueue is empty and wait if not
        int waitedForEmptyQueue = network.waitForEmptySendingQueue();
        if (waitedForEmptyQueue == 1) {
            pthread_mutex_lock(&measureTBUS_nextttx_mutex);
            nextChirpTtx = max(timing.getCurrentTimeNs() + ProcessTimer_, nextTtxUpstream_);
            pthread_mutex_unlock(&measureTBUS_nextttx_mutex);
        }

        // get the last information
        if (getLatestInformationOutOfChannels(basedOnChirp, basedOnPacket, datarate) == 0) {
            nCurrentDatarate_ = datarate;
        } else {
            log_DEBUG("MEME| No valid Information in Back- and SafeguardChannel");
        }

        // should the algorithm be adaptive or static
        if (isAdaptive_) {
            calculateChirpAndChannelProperties(basedOnChirp, basedOnPacket);
        }

        timing.start();
        int queuelength = 0;
        if ((queuelength = enqueueChirp(nextChirpTtx)) == -1) {
            break; // experiment timed out
        } else {
            int64_t current = timing.getCurrentTimeNs();
            if (current - lastConsoleLog >= CONSOLE_LOG_THROTTLE) {
                log_NOTICE("MEME| Enqueued chirp ", integer(nextChirpnr_ - 1), " at ", integer(nextChirpTtx), " with almost "
                        , integer(nChirpLength_), " x ", integer(nPacketSize_), " Bytes = ", integer((nChirpLength_ - 1) * nPacketSize_ + nMinimalPacketSize_),
                        " using datarate ", integer(datarate), " took ", integer(timing.stop() / 1000000LL), "ms.");
                lastConsoleLog = current;
            }
        }

        //check if we received a MEASURE_TBUS_DONE message via controlChannel
        if (syncingDone_) {
            static string controlMessage = "MEASURE_TBUS_DONE";
            string receivedMessage = network.readFromControlChannel(inputH.get_nodeRole(), 0, "MEME", false);
            if (receivedMessage.compare(controlMessage) == 0) {
                log_NOTICE("MEME| -> Received ", controlMessage, " message. Shutting down.");
                ExpEndtime_ = timing.getCurrentTimeNs();
            }
        }

        // Check for dead threads...
        int deadThreadCount = get_countOfDeadThreads();
        if (deadThreadCount > 0) {
            log_ERROR("MEME| Dead thread in fillSendingQueue()");
            return deadThreadCount;
        }

    }

    network.set_finishedFillingSendingQueue(true);
    if (DEBUB_FILLSENDINGQUEUE) log_DEBUG("MEME| Leaving fillSendingQueue()");
    return 0;
}

/**
 * Returns the time difference between now and the last time a chirp was sent
 * @return double
 */
int64_t measureTBUS::get_timeSinceLastChirpWasEnqueued() {
    static timeHandler timing(inputH.get_timingMethod());
    return timing.getCurrentTimeNs() - lastChirpStart_;
}

/**
 * Calculates the chirp size in dependence of packet size, data rate and percentage of channel load
 * 
 * calculate packetsize as kind of staircase function
 * datarate in byte/s
 * @return 0
 */
int32_t measureTBUS::calculateChirpAndChannelProperties(uint32_t chirpnr, uint32_t packetnr) {
    int64_t datarateForCalc = nCurrentDatarate_;
    int64_t bytesToSend = nCurrentDatarate_ * MEASUREMENT_INTERVALL * nChannelload_ / 100 / 1000000000LL;

    //nChirpLength_ = 400; //start with 400 packets
    nChirpLength_ = 20; //send at least 20 packets if possible, even if they are smaller than the mtu

    //nPacketSize_ = (datarateForCalc * nChannelload_ / 100 / nChirpLength_);
    nPacketSize_ = (bytesToSend / nChirpLength_);
    if (nPacketSize_ > ETHERNET_MTU_SIZE) { //TODO NG check ethernet MTU size
        nPacketSize_ = ETHERNET_MTU_SIZE;
        assert(nPacketSize_ != 0);
        nChirpLength_ = (bytesToSend / nPacketSize_);
    } else if (nPacketSize_ < nMinimalPacketSize_) {
        nPacketSize_ = nMinimalPacketSize_;
        assert(nPacketSize_ != 0);
        nChirpLength_ = (bytesToSend / nPacketSize_);
    }

    // shape maximal chirp length
    if (inputH.get_nodeRole() == 0 && nChirpLength_ > nMaximalChirpLengthUplink_) {
        log_NOTICE("MEME| Calculated chirplength is greater than ", integer(nMaximalChirpLengthUplink_), " which we will use instead!");
        nChirpLength_ = nMaximalChirpLengthUplink_;
    }
    if (inputH.get_nodeRole() == 1 && nChirpLength_ > nMaximalChirpLengthDownlink_) {
        log_NOTICE("MEME| Calculated chirpsize is greater than ", integer(nMaximalChirpLengthDownlink_), " which we will use instead!");
        nChirpLength_ = nMaximalChirpLengthDownlink_;
    }
    // check pathological case
    if (nChirpLength_ < 3 && datarateForCalc > 0) {
        nChirpLength_ = 3;
        nPacketSize_ = nMinimalPacketSize_;
        log_NOTICE("MEME| Pathological case, where the datarate < 0.01, so that chirp length < 3), ",
                "assign chirpsize to 3 and packetsize to ", integer(nPacketSize_));
    }

    assert(nPacketSize_ != 0);
    log_DEBUG("MEME| New chirpsize = ", integer(datarateForCalc), "*",
            integer(nChannelload_), "/100/", integer(nPacketSize_), "=",
            integer((bytesToSend / nPacketSize_)),
            " based on ", LOG_CHIRP_PACKET(chirpnr, packetnr), " and rate=", integer(datarateForCalc), " Byte/s");

    return 0;
}

/**
 * Returns the latest information out of backchannel or safeguard channel
 * @param basedOnId, return uint32_t value for baseID
 * @param rateInt, return uint32_t for datarate (integer part)
 * @param rateFrac, return uint32_t for datarate (fraction part)
 * @return 0 when there is information, -1 otherwise
 */
int32_t measureTBUS::getLatestInformationOutOfChannels(uint32_t &basedOnChirp, uint32_t & basedOnPacket, int64_t & datarate) {
    if (backChannel_received_.getBasedOnChirpnr() == 0) {
        return -1;
    }
    pthread_mutex_lock(&measureTBUS_backchannel_mutex);
    datarate = backChannel_received_.getDatarate();
    basedOnChirp = backChannel_received_.getBasedOnChirpnr();
    basedOnPacket = backChannel_received_.getBasedOnPacketnr();
    pthread_mutex_unlock(&measureTBUS_backchannel_mutex);
    return 0;
}

/**
 * Will enqueue a chirp into sending queue, also piggyback recent information
 * @param chirpnr, recent chirp nummer
 * @param basedOnId, id on which to base the measurements
 * @return the queuesize after enqueuing finished or  -1 if the experiment time expires
 */
int32_t measureTBUS::enqueueChirp(const int64_t nextTtx) {
    if (DEBUG_ENQUEUE_CHIRP) log_DEBUG("MEME| Entering enqueueChirp()");
    uint32_t packetnr = 1;

    int ret = 0;
    if (nextTtx > ExpEndtime_) {
        return -1;
    }

    //create the first packet of the chirp (which does not include any padding in the payload)
    //DEPRECATED as we now have a specific function checking packetsize and thus can include the 
    //packetsize of the measurement packets 2..n in each packet of a chirp
    //class TBUSFixedPayload fixedPayload(nextTtx, nextTtx, nextChirpnr_, packetnr, nMinimalPacketSize_, nChirpLength_);
    class TBUSFixedPayload fixedPayload(nextTtx, nextTtx, nextChirpnr_, packetnr, nMinimalPacketSize_, nChirpLength_, nPacketSize_);

    if (DEBUG_ENQUEUE_CHIRP) log_DEBUG("MEME| Enqueuing chirp with ", fixedPayload.toStringSend());
    // send first packet
    network.mSendingQueue_enqueue(fixedPayload);
    if (DEBUG_ENQUEUE_CHIRP) log_DEBUG("MEME| Enqueued packet  chirp:", integer(nextChirpnr_), ", nr: ", integer(packetnr), ".");

    // Enqueue the packets of the chirp using the same sendtime
    for (packetnr = 2; packetnr <= nChirpLength_; packetnr++) {
        //use the same sendtime as it will be corrected prior to sending in the threadedMeasurementSender
        fixedPayload = TBUSFixedPayload(nextTtx, nextTtx, nextChirpnr_, packetnr, nPacketSize_, nChirpLength_, nPacketSize_);
        ret = network.mSendingQueue_enqueue(fixedPayload);
        if (DEBUG_ENQUEUE_CHIRP) log_DEBUG("MEME| Enqueued packet  chirp:", integer(nextChirpnr_), ", nr: ", integer(packetnr), ".");
    }

    //update nextTtx values
    pthread_mutex_lock(&measureTBUS_nextttx_mutex);
    nextChirpnr_++;
    nextTtxUpstreamNormal_ = nextTtx + MEASUREMENT_INTERVALL;
    nextTtxUpstream_ = nextTtx + MEASUREMENT_INTERVALL;
    //nextTtxUpstream_ = nextTtxUpstream_ > nextTtxUpstreamNormal_ ? nextTtxUpstream_ : nextTtxUpstreamNormal_; //maybe we already received a larger nextTtx  for the next chirp
    pthread_mutex_unlock(&measureTBUS_nextttx_mutex);
    if (DEBUG_ENQUEUE_CHIRP) log_DEBUG("MEME| Leaving enqueueChirp()");
    return ret;
}


/* --------------------------------------------------------------------------------
 * A L G O  -  P A R S I N G
 * -------------------------------------------------------------------------------- */

/**
 * Main handler for parsing input string
 *
 * @param givenParameter, parameter string
 * @return 0 on success, -1 when an error occured
 */
int32_t measureTBUS::parse_parameter(string &givenParameter) {
    log_DEBUG("MEME| Entering parseParameter() in measureTBUS: ", givenParameter);

    int retval = parse_adaptSizes(givenParameter)
            | parse_startRate(givenParameter)
            | parse_channelload(givenParameter)
            | parse_useOfCongestionDector(givenParameter)
            | parse_chirpLengthDownlink(givenParameter)
            | parse_chirpLengthUplink(givenParameter)
            | parse_maximalDatarateDownlink(givenParameter)
            | parse_maximalDatarateUplink(givenParameter);

    if (!givenParameter.empty()) {
        log_ERROR("MEME| ERROR: There are unknown Parameter left. \"", givenParameter.c_str(), "\"");
        retval = -1;
        exit(-1);
    }

    log_DEBUG("MEME| Leaving parseParameter()");

    return (retval);
}

/**
 * Searches the fix in the given input string.
 * Generates a substring from the position of the tag to the following "|".
 *
 * @param tag, parameter tag
 * @param input, parameter string
 * @param retval, string pointer to put generated substring in
 *
 * @return 0, everything is ok
 * @return -1, an error occured
 */
int32_t measureTBUS::search_parameter(const string fix, string &input, string &retval) {
    size_t tag_startpos = input.find(fix);
    size_t first_separator = input.find('|', tag_startpos);
    size_t last_separator = input.find('|', first_separator + 1);

    if (tag_startpos == string::npos) {
        return 0; // parameter not found
    }

    if ((first_separator == string::npos) || (last_separator == string::npos)) {
        log_ERROR("MEME| ERROR: Parameter '", fix, "' malformed.");
        return -1; // parameter not found
    }

    retval = input.substr(first_separator + 1, last_separator - first_separator - 1);
    input = input.erase(tag_startpos, last_separator + 1 - tag_startpos);

    // There should be no more Experiment duration Parameters left, otherwise exit
    if (input.find(fix) != string::npos) {
        log_ERROR("MEME| ERROR: Multiple parameter instances of '", fix, "'.");
        return -1; // multiple instances
    }

    return 0;
}

/**
 * Parses boolean: should the algorithm be adaptic
 *
 * @param givenParameter, parameter string
 * @return 0 on success, -1 when an error occured
 */
int32_t measureTBUS::parse_adaptSizes(string &givenParameter) {
    string param = "";
    if (search_parameter("-mT_a|", givenParameter, param)) {
        log_INFORMATIONAL("MEME| -> Boolean for adaption not found. Default = ", boolean(isAdaptive_));
        return -1; // Error while searching for parameter
    }

    if (param.empty()) {
        log_INFORMATIONAL("MEME| -> Boolean for adaption not found. Default = ", boolean(isAdaptive_));
        return 0; // Parameter not found, use default init value
    }

    bool tmp_adapt = 0;
    if (parse_boolean(param, tmp_adapt)) {
        return -1; // Not an boolean value
    }

    this->isAdaptive_ = tmp_adapt;

    // There should be no more Experiment duration Parameters left, otherwise exit
    if (givenParameter.find("-mT_a|") != string::npos) {
        log_ERROR("MEME| Error: Boolean for adaption parameter malformed or more then one -mT_a.");
        return -1;
    }

    log_INFORMATIONAL("MEME| -> Found boolean for adaption = ", boolean(tmp_adapt));
    return 0; // Chirp size found
}

/**
 * Parses boolean: should the detector be used
 *
 * @param givenParameter, parameter string
 * @return 0 on success, -1 when an error occured
 */
int32_t measureTBUS::parse_useOfCongestionDector(string &givenParameter) {
    string param = "";
    if (search_parameter("-mT_d|", givenParameter, param)) {
        log_INFORMATIONAL("MEME| -> Boolean for use of congestion detector not found. Default = ", boolean(isUseOfCongestionDetector_));
        return -1; // Error while searching for parameter
    }

    if (param.empty()) {
        log_INFORMATIONAL("MEME| -> Boolean for use of congestion detector not found. Default = ", boolean(isUseOfCongestionDetector_));
        return 0; // Parameter not found, use default init value
    }

    bool tmp_detector = 0;
    if (parse_boolean(param, tmp_detector)) {
        return -1; // Not an boolean value
    }

    this->isUseOfCongestionDetector_ = tmp_detector;

    // There should be no more Experiment duration Parameters left, otherwise exit
    if (givenParameter.find("-mT_d|") != string::npos) {
        log_ERROR("MEME| Error: Boolean for use of congestion detector parameter malformed or more then one -mT_d.");
        return -1;
    }

    log_INFORMATIONAL("MEME| -> Found boolean for use of congestion detector = ", boolean(tmp_detector));
    return 0;
}

/**
 * Parses start darate out of given parameter string
 *
 * @param givenParameter, parameter string
 * @return 0 on success, -1 when an error occured
 */
int32_t measureTBUS::parse_startRate(string &givenParameter) {
    string param = "";
    if (search_parameter("-mT_s|", givenParameter, param)) {
        log_INFORMATIONAL("MEME| -> Initial data rate not found. Default = ", integer(nCurrentDatarate_), "[Byte/s]");
        return -1; // Error while searching for parameter
    }

    if (param.empty()) {
        log_INFORMATIONAL("MEME| -> Initial data rate not found. Default = ", integer(nCurrentDatarate_), "[Byte/s]");
        return 0; // Parameter not found, use default init value
    }

    int64_t tmp_datarate = 0;
    if (parse_long(param, tmp_datarate)) {
        return -1; // Not an integer value
    }

    this->nInitialDatarate_ = tmp_datarate;
    this->nCurrentDatarate_ = tmp_datarate;

    // There should be no more Experiment duration Parameters left, otherwise exit
    if (givenParameter.find("-mT_s|") != string::npos) {
        log_ERROR("MEME| Error: Initial data rate parameter malformed or more then one mT_s.");
        return -1;
    }

    log_INFORMATIONAL("MEME| -> Found initial data rate = ", integer(tmp_datarate), "[Byte/s]");
    return 0; // parameter found
}

/**
 * Parses start maximal chirp length for the dowlink
 *
 * @param givenParameter, parameter string
 * @return 0 on success, -1 when an error occured
 */
int32_t measureTBUS::parse_chirpLengthDownlink(string &givenParameter) {
    string param = "";
    if (search_parameter("-mT_cld|", givenParameter, param)) {
        log_INFORMATIONAL("MEME| -> Maximal chirp length for the download not found. Default = ", integer(nMaximalChirpLengthDownlink_));
        return -1; // Error while searching for parameter
    }

    if (param.empty()) {
        log_INFORMATIONAL("MEME| -> Maximal chirp length for the download found. Default = ", integer(nMaximalChirpLengthDownlink_));
        return 0; // Parameter not found, use default init value
    }

    double tmp_length = 0;
    if (parse_double(param, tmp_length)) {
        return -1; // Not an integer value
    }

    if (tmp_length > 0xFFFFFFFFL) {
        this->nMaximalChirpLengthDownlink_ = 0xFFFFFFFFL;
        log_ERROR("MEME| -> Maximal chirp length larger than uint32. Changed it to ", integer(nMaximalChirpLengthDownlink_));
    } else {
        this->nMaximalChirpLengthDownlink_ = tmp_length;
    }

    // There should be no more Experiment duration Parameters left, otherwise exit
    if (givenParameter.find("-mT_cld|") != string::npos) {
        log_ERROR("MEME| Error: Maximal chirp length for the download parameter malformed or more then one mT_cld.");
        return -1;
    }

    log_INFORMATIONAL("MEME| -> Found maximal chirp length for the download = ", integer(nMaximalChirpLengthDownlink_));
    return 0; // parameter found
}

/**
 * Parses start maximal chirp length for the uplink
 *
 * @param givenParameter, parameter string
 * @return 0 on success, -1 when an error occured
 */
int32_t measureTBUS::parse_chirpLengthUplink(string &givenParameter) {
    string param = "";
    if (search_parameter("-mT_clu|", givenParameter, param)) {
        log_INFORMATIONAL("MEME| -> Maximal chirp length for the upload not found. Default = ", integer(nMaximalChirpLengthUplink_));
        return -1; // Error while searching for parameter
    }

    if (param.empty()) {
        log_INFORMATIONAL("MEME| -> Maximal chirp length for the upload found. Default = ", integer(nMaximalChirpLengthUplink_));
        return 0; // Parameter not found, use default init value
    }

    double tmp_length = 0;
    if (parse_double(param, tmp_length)) {
        return -1; // Not an integer value
    }

    if (tmp_length > 0xFFFFFFFFL) {
        this->nMaximalChirpLengthUplink_ = 0xFFFFFFFFL;
        log_ERROR("MEME| -> Maximal chirp length larger than uint32. Changed it to ", integer(nMaximalChirpLengthUplink_));
    } else {
        this->nMaximalChirpLengthUplink_ = tmp_length;
    }


    // There should be no more Experiment duration Parameters left, otherwise exit
    if (givenParameter.find("-mT_clu|") != string::npos) {
        log_ERROR("MEME| Error: Maximal chirp length for the upload parameter malformed or more then one mT_clu.");
        return -1;
    }

    log_INFORMATIONAL("MEME| -> Found maximal chirp length for the upload = ", integer(nMaximalChirpLengthUplink_));
    return 0; // parameter found
}

/**
 * Parses start maximal data rate for the uplink
 *
 * @param givenParameter, parameter string
 * @return 0 on success, -1 when an error occured
 */
int32_t measureTBUS::parse_maximalDatarateUplink(string &givenParameter) {
    string param = "";
    if (search_parameter("-mT_mdu|", givenParameter, param)) {
        log_INFORMATIONAL("MEME| -> Maximal data rate for the upload not found. Default = ", integer(nMaximalDatarateUplink_), "[Byte/s]");
        return -1; // Error while searching for parameter
    }

    if (param.empty()) {
        log_INFORMATIONAL("MEME| -> Maximal data rate for the upload found. Default = ", integer(nMaximalDatarateUplink_), "[Byte/s]");
        return 0; // Parameter not found, use default init value
    }

    double tmp_maxdata = 0;
    if (parse_double(param, tmp_maxdata)) {
        return -1; // Not an integer value
    }

    this->nMaximalDatarateUplink_ = tmp_maxdata;

    // There should be no more Experiment duration Parameters left, otherwise exit
    if (givenParameter.find("-mT_mdu|") != string::npos) {
        log_ERROR("MEME| Error: Maximal datarate for the upload parameter malformed or more then one mT_mdu.");
        return -1;
    }

    log_INFORMATIONAL("MEME| -> Found maximal datarate for the upload = ", integer(nMaximalDatarateUplink_), "[Byte/s]");
    return 0; // parameter found
}

/**
 * Parses start maximal datarate for the downlink
 *
 * @param givenParameter, parameter string
 * @return 0 on success, -1 when an error occured
 */
int32_t measureTBUS::parse_maximalDatarateDownlink(string &givenParameter) {
    string param = "";
    if (search_parameter("-mT_mdd|", givenParameter, param)) {
        log_INFORMATIONAL("MEME| -> Maximal data rate for the download not found. Default = ", integer(nMaximalDatarateDownlink_), "[Byte/s]");
        return -1; // Error while searching for parameter
    }

    if (param.empty()) {
        log_INFORMATIONAL("MEME| -> Maximal data rate for the download found. Default = ", integer(nMaximalDatarateDownlink_), "[Byte/s]");
        return 0; // Parameter not found, use default init value
    }

    double tmp_maxdata = 0;
    if (parse_double(param, tmp_maxdata)) {
        return -1; // Not an integer value
    }

    this->nMaximalDatarateDownlink_ = tmp_maxdata;

    // There should be no more Experiment duration Parameters left, otherwise exit
    if (givenParameter.find("-mT_mdd|") != string::npos) {
        log_ERROR("MEME| Error: Maximal data rate for the download parameter malformed or more then one mT_mdd.");
        return -1;
    }

    log_INFORMATIONAL("MEME| -> Found maximal datarate for the download = ", integer(nMaximalDatarateDownlink_), "[Byte/s]");
    return 0; // parameter found
}

/**
 * Parses threshold out of given parameter string
 *
 * @param givenParameter, parameter string
 * @return 0 on success, -1 when an error occured
 */
int32_t measureTBUS::parse_channelload(string &givenParameter) {
    string param = "";
    if (search_parameter("-mT_l|", givenParameter, param)) {
        log_INFORMATIONAL("MEME| -> Start channel load not found. Default = ", integer(nChannelload_));
        return -1; // Error while searching for parameter
    }

    if (param.empty()) {
        log_INFORMATIONAL("MEME| -> Start load not found. Default = ", integer(nChannelload_));
        return 0; // Parameter not found, use default init value
    }

    int tmp_channelload = 0;
    if (parse_integer(param, tmp_channelload)) {
        return -1; // Not an integer value
    }

    if (tmp_channelload <= 0 && tmp_channelload > 100) {
        log_INFORMATIONAL("MEME| -> Channel load is not out [0,1). Default = ", integer(nChannelload_));
        return 0;
    }
    this->nChannelload_ = tmp_channelload;

    // There should be no more Experiment duration Parameters left, otherwise exit
    if (givenParameter.find("-mT_l|") != string::npos) {
        log_ERROR("MEME| Error: Start channel load parameter malformed or more then one mT_l.");
        return -1;
    }

    log_INFORMATIONAL("MEME| -> Found channel load = ", integer(tmp_channelload));
    return 0; // parameter found
}

// Helper for Parsing

/**
 * Parses an integer value out of the input string.
 *
 * @param input, reference to the parameter string
 * @param retval, pointer to integer return value
 *
 * @return 0, if everything is ok
 * @return -1, if an error occured
 */
int32_t measureTBUS::parse_integer(const string input, int &retval) {
    const char *input_cstr = input.c_str();
    int in_length = input.length();
    int i = 0;
    while (isdigit(input_cstr[i]) && (i < in_length)) {
        ++i;
    }

    if (i != in_length) {
        log_ERROR("MEME| ERROR: Not an integer value.");
        return -1;
    }

    retval = (uint32_t) static_cast<uint32_t> (atoi(input_cstr));
    return 0;
}

/**
 * Parses an integer value out of the input string.
 *
 * @param input, reference to the parameter string
 * @param retval, pointer to integer return value
 *
 * @return 0, if everything is ok
 * @return -1, if an error occured
 */
int32_t measureTBUS::parse_long(const string input, int64_t &retval) {
    const char *input_cstr = input.c_str();
    int in_length = input.length();
    int i = 0;
    while (isdigit(input_cstr[i]) && (i < in_length)) {
        ++i;
    }

    if (i != in_length) {
        log_ERROR("MEME| ERROR: Not an integer value.");
        return -1;
    }

    retval = (int64_t) static_cast<int64_t> (atol(input_cstr));
    return 0;
}

/**
 * Parses an boolean value out of the input string.
 * Boolean is decoded as integer
 *
 * @param input, reference to the parameter string
 * @param retval, pointer to boolean return value
 *
 * @return 0, if everything is ok
 * @return -1, if an error occured
 */
int32_t measureTBUS::parse_boolean(const string input, bool &retval) {
    int value = 0;
    if (parse_integer(input, value) == 0) {
        retval = value == 1;
        return 0;
    }
    return -1;
}

/**
 * Parses a double value out of the input string.
 *
 * @param input, reference to parameter string
 * @param retval, pointer to double return value
 *
 * @return 0, if everything is ok
 * @return -1, if an error occured
 */
int32_t measureTBUS::parse_double(const string input, double &retval) {
    int in_length = input.length();
    int i = 0;
    while ((isdigit(input[i]) || input[i] == '.' || input[i] == ',') && i < in_length) {
        ++i;
    }

    if (i != in_length) {
        log_ERROR("MEME| ERROR: Not a double value.");
        return -1;
    }

    retval = atof(input.c_str());
    return 0;
}

/* --------------------------------------------------------------------------------
 * A L G O  -  M E A S U R I N G
 * -------------------------------------------------------------------------------- */

/**
 * Initialize Measurement, so open logfiles, which links
 * should be measured. After this the results will be calculated.
 *
 * @return 0 on success, -1 when an error occured
 */
int32_t measureTBUS::initMeasurement() {
    log_DEBUG("MEME| Entering initMeasurement()");

    class timeHandler timing(inputH.get_timingMethod());
    string parameter_string = inputH.get_parameterString();

    // Reset all values. If we are server we have to do this since we don't spawn a new class, we reuse the global
    initValues();

    // Parse parameter
    if (inputH.get_nodeRole() == 0) {
        if (parse_parameter(parameter_string) == -1) {
            log_ERROR("MEME| Leaving initMeasurement(Parse parameter)");
            closeEveryFile();
            stopRunningThreads();
            return -1;
        }
    }

    // Try to synchronize parameter
    if (syncMeasurementParemeter() != 0) {
        log_ERROR("MEME| Leaving initMeasurement(Sync failed)");
        closeEveryFile();
        stopRunningThreads();
        return -1;
    } else {
        syncingDone_ = true;
    }

    //resize the queues after we received the the maxchirpsizes
    network.getReceiveQueue().resize(getReceiveQueueSize());
    network.getSendingQueue().resize(getSendingQueueSize());
    network.getSentQueue().resize(getSentQueueSize());
    network.getSafeguardReceivingQueue().resize(getSafeguardReceivingQueueSize());
    network.getSafeguardSendQueue().resize(getSafeguardSendingQueueSize());

    // open Logfiles for Sender and Receiver
    if (openLogfiles() == -1) {
        return -1;
    } else {
        fastDatarateCalculator_.setmTBUS(this);
    }

    // start initializing and so on only iff we dont want to calcualte data only
    initHelperClassesAndObjects();

    // TODO NG: make this adjustable via command line parameter
    if (inputH.amIServer()) {
        network.setMeasurementChannelSendingRateThrottle(nMaximalDatarateDownlink_, true);
    } else {
        network.setMeasurementChannelSendingRateThrottle(nMaximalDatarateUplink_, true);
    }

    // start measurement method
    if (startMeasurement(timing) != 0) {
        return -1;
    }

    // Wait for the done message, but only if we expect one
    // ending all threads, beacause sending has finished
    stopRunningThreads();

    // and now it is over
    if (endMeasurement(timing) != 0) {
        return -1;
    }

//we do no longer calculate results in the rmf
//    // Calculate Results but only if we some data for it
//    if ((inputH.get_measuredLink() == 0 && inputH.get_nodeRole() == 1) || // Uplink clients view 	 | Server has the data
//            (inputH.get_measuredLink() == 1 && inputH.get_nodeRole() == 0) || // Downlink clients view | Client has the data
//            (inputH.get_measuredLink() == 2)) { // Boths links
//        log_INFORMATIONAL("MEME| -> Logging done. Processing collected Data.");
//        timing.probeTime();
//        timing.sleepUntil(timing.getLastProbedTimeNs() + 5000000000LL); //sleep 5 seconds
//
//        uint32_t maxChirpLength = inputH.get_nodeRole() == 0 ? nMaximalChirpLengthDownlink_ : nMaximalChirpLengthUplink_;
//        ResultCalculator resCalc(maxChirpLength, inputH.get_nodeRole(), nCountSmoothening_, inputH.get_logfilePath());
//        resCalc.set_createLossPlot(true);
//        resCalc.set_createCongestionPlot(true);
//        resCalc.set_createLossPlot(true);
//        resCalc.set_createChirpAndPacketSizePlot(true);
//
//        //TODO: rewrite the result calculator - atm we just remove it!
//        //resCalc.calculateResults();
//    }

    // close all files
    closeEveryFile();

    log_DEBUG("MEME| Leaving initMeasurement()");

    return 0;
}

/**
 * Initialize all kind of array, vectors, helper classes for measurement
 * @return 0 when no error occures
 */
int32_t measureTBUS::initHelperClassesAndObjects() {
    // Initialize and all kind of instances with custom parameters

    //DEPRECATED: I see no use for this, restarting ntp during a measurement is a no go
    //    // start NTP Helper
    //    ntpHelper_.set_callbackClass(this);
    //    if (ntpHelper_.start() == 0) {
    //        ntpHelper_.setClient(inputH.get_nodeRole() == 0);
    //        log_NOTICE("MEME| NTP Thread is running fine!");
    //    } else
    //        log_ERROR("MEME| Error while starting ntp thread!");

    backChannel_send_ = TBUSDynamicPayload{0L, 0L, nInitialDatarate_, 0L, -1LL, 0L, 0L, 100000000L};
    log_DEBUG("MEME| created dynpay with size ", integer(backChannel_send_.getPayloadSize()));
    network.setDynamicPayload(backChannel_send_);
    backChannel_received_ = backChannel_send_;

    return 0;
}

/**
 * Inform the measurement opptonent that we stopped measuring
 * @return 
 */
int measureTBUS::sendFinishMesurementMessage() {
    string controlMessage = "MEASURE_TBUS_DONE";
    int ret = network.writeToControlChannel(inputH.get_nodeRole(), controlMessage, 0, "MEME");
    log_NOTICE("MEME| Sending FinishMeasurementMessage. Ret=", integer(ret));
    //    static timeHandler timing(inputH.get_timingMethod());
    //    timing.sleepFor(1000000000LL); //1s sleep
    return ret;
}

/**
 * Start measuring between client and server, is part of init initMeasurement()
 * @param timing current timeHandler
 * @param resultCalculationOnly boolean
 * @return 0 when no error occures
 */
int32_t measureTBUS::startMeasurement(timeHandler& timing) {
    // Check which links we want to measure
    if ((inputH.get_measuredLink() == 0 && inputH.get_nodeRole() == 0) || // Uplink clients view   | Client sends packets
            (inputH.get_measuredLink() == 1 && inputH.get_nodeRole() == 1) || // Downlink clients view | server sends packets
            (inputH.get_measuredLink() == 2)) // Boths links
    {
        // Signal threadedMeasurementReceiver that receiving his jobs is done
        if (inputH.get_measuredLink() != 2) network.set_measureReceivingDone(true);

        int errorValue = 0;

        // notice start time
        int64_t now = timing.getCurrentTimeNs();
        lastSafeguardPacketBecauseTimeoutSent_ = now;
        ExpStarttime_ = now;

        log_NOTICE("MEME| Logging to console and this log file is throttled to 1 log every ", integer(CONSOLE_LOG_THROTTLE), " ns per function.");
        // Start enqueuing packets
        log_INFORMATIONAL("MEME| -> Starting to enqueue packets for sending (",
                integer(inputH.get_experimentDuration()), " seconds).");
        errorValue = fillSendingQueue();
        if (errorValue > 0) {
            log_ERROR("MEME| Leaving initMeasurement(deadThread) while fillSendingQueue()");
            sendFinishMesurementMessage(); //try to inform the other side that we shut down our measurements
            closeEveryFile();
            stopRunningThreads();
            return (errorValue);
        }

        log_INFORMATIONAL("MEME| -> Done to enqueue packets for sending, waiting for sending to be done.");

        network.set_sendingDone(true);
        network.set_finishedFillingSendingQueue(true);
        network.set_safeguardReceivingDone(true);

        // Wait for fillSendingQueue() to be done
        while (!network.get_sendingDone() && !network.get_emergencySTOP()) {
            timing.sleepFor(1000000000LL); //sleep 1 second
        }

        // Check if we have to stop right now
        if (network.get_emergencySTOP()) {
            sendFinishMesurementMessage(); //try to inform the other side that we shut down our measurements
            closeEveryFile();
            stopRunningThreads();
            log_ERROR("MEME| Leaving initMeasurement(deadThread) because network.get_emergencySTOP()");
            if (errorValue == 0) errorValue = -1;

            return (errorValue);
        }

        //        // Send message that we are done, try 6 time every 2 seconds
        //        int attempts = 0;
        //int retVal = 
        sendFinishMesurementMessage(); //try to inform the other side that we shut down our measurements

        //        while (retVal != 0) {
        //            if (attempts >= 6 || network.get_emergencySTOP() || retVal < 0) {
        //                closeEveryFile();
        //                stopRunningThreads();
        //                log_ERROR("MEME| Leaving initMeasurement() because no \"", controlMessage, "\"");
        //                return -1;
        //            }
        //
        //            log_DEBUG("MEME| -> Trying to send for ", controlMessage, " message, retrying in 2 seconds.");
        //
        //            timing.probeTime();
        //            timing.sleepFor(2000000000LL); //sleep for 2 seconds
        //
        //            attempts++;
        //        }
    }
    return 0;
}

/**
 * Ends measuring between client and server, is part of init initMeasurement()
 * @param timing current timeHandler
 * @return 0 when no error occures
 */
int32_t measureTBUS::endMeasurement(timeHandler& timing) {
    if ((inputH.get_measuredLink() == 0 && inputH.get_nodeRole() == 1) || // Uplink clients view 	 | Server waits for message
            (inputH.get_measuredLink() == 1 && inputH.get_nodeRole() == 0) || // Downlink clients view | Client waits for message
            (inputH.get_measuredLink() == 2)) // Boths links
    {
        // Set Sending to done since we wont send anything
        if (inputH.get_measuredLink() != 2) network.set_finishedFillingSendingQueue(true);

        log_INFORMATIONAL("MEME| -> Waiting for receiving to be done.");

        // wait for the other side to be done
        if (inputH.get_measuredLink() != 2) timing.sleepUntil(ExpEndtime_ + 2000000000LL); //sleep 2 second longer than ExpEndtime

        // Try to receive the message from counterpart, retry 6 times every 2 seconds
        string controlMessage = "MEASURE_TBUS_DONE";
        string receivedMessage = network.readFromControlChannel(inputH.get_nodeRole(), 0, "MEME");

        int attempts = 0;
        while (receivedMessage.compare(controlMessage) != 0) {
            if (attempts >= 5 || network.get_emergencySTOP() || receivedMessage.compare("error_read_EOF") == 0) {
                log_ERROR("MEME| -> ", controlMessage, " message not received after 10 seconds. Beginning calculation.");
                break;
            }

            log_NOTICE("MEME| -> Waiting for ", controlMessage, " message, retrying in 2 seconds.");

            timing.sleepFor(2000000000LL); //sleep 2 s

            receivedMessage = network.readFromControlChannel(inputH.get_nodeRole(), 0, "MEME");
            attempts++;
        }

        // Signal threadedMeasurementReceiver that receiving his jobs is done
        network.set_measureReceivingDone(true);

        while (!isLoggingDone_ && !network.get_emergencySTOP()) {
            timing.sleepFor(10000000LL); //sleep for 10 ms
        }
    }
    return 0;
}

/* --------------------------------------------------------------------------------
 * A L G O  -  S Y N C I N G
 * -------------------------------------------------------------------------------- */

/**
 * Syncs measurement parameters between server and client.
 *
 * @return -1 on failure, 0 on success
 */
int32_t measureTBUS::syncMeasurementParemeter() {
    log_DEBUG("MEME| Entering syncMeasurementParemeter()");
    int retval = 0;

    if (inputH.get_nodeRole() == 0) {
        retval = sendSyncParameter();
    } else if (inputH.get_nodeRole() == 1) {
        retval = receiveSyncParameter();
    }

    log_DEBUG("MEME| Leaving syncMeasurementParemeter()");

    return (retval);
}

/**
 * Sends the synchronization data
 *
 * @return -1 on failure, 0 on success
 */
int32_t measureTBUS::sendSyncParameter() {
    class timeHandler timing;
    timing.setTimingMethod(inputH.get_timingMethod());
    stringstream parameter;

    parameter << "MEASURE_TBUS_SYNC"
            << '|' << isAdaptive_
            << '|' << isUseOfCongestionDetector_
            << '|' << nPacketHeaderSizeEthernetIpUdp_
            << '|' << nChannelload_
            << '|' << nInitialDatarate_
            << '|' << nMaximalChirpLengthDownlink_
            << '|' << nMaximalChirpLengthUplink_
            << '|' << nMaximalDatarateDownlink_
            << '|' << nMaximalDatarateUplink_
            << '|';

    // Send message that we are done, try 5 time every 3 seconds
    int attempts = 0;
    while (network.writeToControlChannel(0, parameter.str(), 0, "MEME") != 0) {
        if (attempts >= 5) return -1;

        if (network.get_emergencySTOP()) {
            return -1;
        }

        log_DEBUG("MEME| -> trying to send MEASURE_SYNC message, retrying in 3 seconds.");

        timing.sleepFor(3000000000LL); //sleep for 3 s
        attempts++;
    }

    return 0;
}

/**
 * Receives the synchronization data
 *
 * @return -1 on failure, 0 on success
 */
int32_t measureTBUS::receiveSyncParameter() {
    class timeHandler timing;
    timing.setTimingMethod(inputH.get_timingMethod());
    string receivedMessage;

    receivedMessage = network.readFromControlChannel(1, 3, "MEME");

    // Check if we got a Sync Message and not an error, try 5 times every 3 seconds.
    int attempts = 0;
    string tag = "MEASURE_TBUS_SYNC|";
    while (receivedMessage.substr(0, tag.length()).compare(tag) != 0) {
        if (attempts >= 5) {
            return -1; // failed to receive data
        }

        log_DEBUG("MEME| -> Waiting for MEASURE_TBUS_SYNC message, retrying in 3 seconds.");

        timing.sleepFor(3000000000LL); //sleep 3 s

        receivedMessage = network.readFromControlChannel(1, 0, "MEME");
        attempts++;
    }

    // Parse received String, start with chirpsize
    if (receivedMessage.substr(tag.length(), receivedMessage.length()).find("|") == string::npos) {
        log_ERROR("MEME| ERROR: No Parameter(s) in MEASURE_TBUS_SYNC message");
        return -1;
    }
    receivedMessage.erase(0, tag.length()); // remove header

    string sync_parameter = "";

    // all values to 0 or false, because they will be corrected

    // getting boolean for adaption
    bool tmp_adaptive = false;
    if (extractSyncParameter(receivedMessage, sync_parameter)) return -1; // failed to read parameter
    if (parse_boolean(sync_parameter, tmp_adaptive)) return -1; // not an boolean value

    // getting boolean for detector use
    bool tmp_detector = false;
    if (extractSyncParameter(receivedMessage, sync_parameter)) return -1; // failed to read parameter
    if (parse_boolean(sync_parameter, tmp_detector)) return -1; // not an boolean value

    // getting packet header size
    int tmp_packetheadersize = 0;
    if (extractSyncParameter(receivedMessage, sync_parameter)) return -1; // failed to read parameter
    if (parse_integer(sync_parameter, tmp_packetheadersize)) return -1; // not a integer value

    // getting threshold
    int tmp_channelload = 50;
    if (extractSyncParameter(receivedMessage, sync_parameter)) return -1; // failed to read parameter
    if (parse_integer(sync_parameter, tmp_channelload)) return -1; // not an double value

    // getting decimal
    double tmp_datarate = 16 * 1000;
    if (extractSyncParameter(receivedMessage, sync_parameter)) return -1; // failed to read parameter
    if (parse_double(sync_parameter, tmp_datarate)) return -1; // not an integer value

    // getting chirp length for download
    int tmp_lengthdown = 1000;
    if (extractSyncParameter(receivedMessage, sync_parameter)) return -1; // failed to read parameter
    if (parse_integer(sync_parameter, tmp_lengthdown)) return -1; // not an integer value

    // getting chirp length for upload
    int tmp_lengthup = 500;
    if (extractSyncParameter(receivedMessage, sync_parameter)) return -1; // failed to read parameter
    if (parse_integer(sync_parameter, tmp_lengthup)) return -1; // not an integer value

    // getting max datarate down
    int tmp_maxdataratedown = 1500 * 1000;
    if (extractSyncParameter(receivedMessage, sync_parameter)) return -1; // failed to read parameter
    if (parse_integer(sync_parameter, tmp_maxdataratedown)) return -1; // not an integer value

    // getting max datarate up
    int tmp_maxdatarateup = 1500 * 1000;
    if (extractSyncParameter(receivedMessage, sync_parameter)) return -1; // failed to read parameter
    if (parse_integer(sync_parameter, tmp_maxdatarateup)) return -1; // not an integer value

    // are there any parameters left?
    if (!receivedMessage.empty()) {
        log_ERROR("MEME| ERROR: Unknown Parameter(s) found: ", receivedMessage);
        return -1;
    }

    // save extracted values
    isAdaptive_ = tmp_adaptive;
    isUseOfCongestionDetector_ = tmp_detector;
    nPacketHeaderSizeEthernetIpUdp_ = tmp_packetheadersize;
    nChannelload_ = tmp_channelload;
    nMaximalChirpLengthDownlink_ = tmp_lengthdown;
    nMaximalChirpLengthUplink_ = tmp_lengthup;
    nMaximalDatarateDownlink_ = tmp_maxdataratedown;
    nMaximalDatarateUplink_ = tmp_maxdatarateup;
    nInitialDatarate_ = tmp_datarate;
    this->nCurrentDatarate_ = tmp_datarate;

    // output
    log_INFORMATIONAL("MEME| Received values:");
    log_INFORMATIONAL("MEME| -> TBUS uses adaptive chirp sizes: ", (isAdaptive_ ? "true" : "false"));
    log_INFORMATIONAL("MEME| -> TBUS uses SICD: ", (isUseOfCongestionDetector_ ? "true" : "false"));
    log_INFORMATIONAL("MEME| -> Header Size: ", integer(nPacketHeaderSizeEthernetIpUdp_));
    log_INFORMATIONAL("MEME| -> Channelload: ", integer(nChannelload_));
    log_INFORMATIONAL("MEME| -> Maximal chirp length for the download: ", integer(nMaximalChirpLengthDownlink_));
    log_INFORMATIONAL("MEME| -> Maximal chirp length for the upload: ", integer(nMaximalChirpLengthUplink_));
    log_INFORMATIONAL("MEME| -> Maximal data rate downlink: ", integer(nMaximalDatarateDownlink_));
    log_INFORMATIONAL("MEME| -> Maximal data rate uplink: ", integer(nMaximalDatarateUplink_));
    log_INFORMATIONAL("MEME| -> Initial datarate: ", integer(nInitialDatarate_));

    return 0;
}

/**
 * Extracts sync parameters from received string.
 *
 * @param received, pointer to parameter string
 * @param retval, pointer to return string
 *
 * @return 0 on success, -1 on error
 */
int32_t measureTBUS::extractSyncParameter(string& received, string &retval) {
    size_t beforeLastSeparator = received.substr(0, received.length()).find("|");
    if (beforeLastSeparator == std::string::npos) {
        log_ERROR("ERROR: Parameter in sync String is missing!");
        return -1; // no parameter left
    }

    retval = received.substr(0, beforeLastSeparator);
    received.erase(0, beforeLastSeparator + 1);
    return 0; // parameter found and written to retval
}

/* --------------------------------------------------------------------------------------------
 * A L G O  -  R E C E I V I N G   M E A S U R  M E N T
 * -------------------------------------------------------------------------------------------- */

/**
 * handles incoming dynamic payload (currently only from backchannel but maybe it is a good idea
 * to merge it with the dynpay from the safeguardchannel
 * checks if the incoming information is newer than already existend information in backChannel_received_
 * and replaces it.
 * It also checks for misconfigured maximalDatarate for the sending correction and raises the bar
 * if many consecutive icoming packets suggest a much higher maximal sending datarate
 * @param dynPay
 */
void measureTBUS::handleIncomingDynamicPayload(const TBUSDynamicPayload & dynPay, bool arrivedViaSafeguard) {
    static uint64_t maxDatarateIncoming = inputH.get_nodeRole() == 1 ? nMaximalDatarateDownlink_ : nMaximalDatarateUplink_; //0 = client
    static int countWrongDatarate = 0;
    static uint64_t averageWrongDatarate = 0LL;
    static uint32_t wrongDatarateStartChirp = 0;
    static uint32_t lastChirpnr = backChannel_received_.getBasedOnChirpnr();
    static uint32_t lastPacketnr = backChannel_received_.getBasedOnPacketnr();

    log_DEBUG("MEME| handling incoming dynpay. ", dynPay.getStringHeaderReceived());
    log_DEBUG("MEME| handling incoming dynpay. ", dynPay.toStringReceived());

    uint32_t currentChirpnr = dynPay.getBasedOnChirpnr();
    uint32_t currentPacketnr = dynPay.getBasedOnPacketnr();

    // set received backchannel if the data is newer and the data rate seems valid
    if ((currentChirpnr > lastChirpnr) || (currentChirpnr == lastChirpnr && currentPacketnr > lastPacketnr)) { //new backchannel info
        nLastSafeguardChannelOWDUpstream_ = dynPay.getLastValidSafeguardChannelDelay_(); //save OWD of safeguardchannel
        if (dynPay.getDatarate() <= maxDatarateIncoming) {
            pthread_mutex_lock(&measureTBUS_backchannel_mutex);
            backChannel_received_ = dynPay;
            pthread_mutex_unlock(&measureTBUS_backchannel_mutex);
            lastChirpnr = currentChirpnr;
            lastPacketnr = currentPacketnr;
            countWrongDatarate = 0;
            averageWrongDatarate = 0LL;
        } else if (adjustOnWrongInitialDatarate_) { //only adjust maxDatarateIncoming if we really want it - might render parts of cellular network measurements useless
            if (countWrongDatarate == 0) {
                wrongDatarateStartChirp = currentChirpnr;
            }
            assert(countWrongDatarate >= 0);
            averageWrongDatarate = (averageWrongDatarate * countWrongDatarate + dynPay.getDatarate()) / (countWrongDatarate + 1);
            countWrongDatarate++;
            //raise the max datarate after 10 consecutive data rate measurements from at least two adjacent chirps  indicating a wrong guard
            if (currentChirpnr > wrongDatarateStartChirp && countWrongDatarate > 10) {
                log_NOTICE("MEME| Detected wrong maxIncomingDatarate, adjusting from ", integer(maxDatarateIncoming), " to ", integer(averageWrongDatarate));
                maxDatarateIncoming = averageWrongDatarate; //raise to the average wrong datarate
                countWrongDatarate = 0;
                averageWrongDatarate = 0LL;
            }
        }
        //handle nextTTX in dynPay (only for newer (chirp:packet) than already handled)
        setNewTtxForNextChirp(dynPay);
    }

    //save the last OWD for the safeguarChannel
    if (arrivedViaSafeguard) {
        if (dynPay.getPacketDelay() > 0) {
            nLastSafeguardChannelOWDDownstream_ = dynPay.getPacketDelay();
            log_DEBUG("MEME| Delay of last safeguardPacket ", integer(dynPay.getBasedOnChirpnr()), ":", integer(dynPay.getBasedOnPacketnr()),
                    " is ", integer(dynPay.getPacketDelay()));
        } else {
            log_DEBUG("MEME| Delay of last safeguardPacket ", integer(dynPay.getBasedOnChirpnr()), ":", integer(dynPay.getBasedOnPacketnr()), " is negative ", integer(dynPay.getPacketDelay()));
        }
    }

}

/**
 * checks nexttx value in incoming dynamic payload and sets the ttx for our next chirp
 * occordingly (after some checks) and probably wakes our sending thread because it needs 
 * to send earlier than expected
 * @param dynPay
 */
void measureTBUS::setNewTtxForNextChirp(const TBUSDynamicPayload & dynPay) {
    static timeHandler timing(inputH.get_timingMethod());
    uint32_t basedOnChirp = dynPay.getNextTtxBasedOnChirpnr();
    int64_t nextTtx = dynPay.getNextTtx();
    pthread_mutex_lock(&measureTBUS_nextttx_mutex);
    int64_t oldNextTtx = nextTtxUpstream_;
    int64_t adjustedNextTtx;
    if (basedOnChirp + 1 <= nextChirpnr_) { //only handle ttx which are sane
        adjustedNextTtx = nextTtx + (nextChirpnr_ - basedOnChirp - 1) * MEASUREMENT_INTERVALL;
        if (adjustedNextTtx >= nextTtxUpstreamNormal_) {
            //even if the receivded a nextttx for previous chirp (due to large delay) we can
            //at least raise the nexttx for our next chirp masurements we can change the nextTtx 

            if (adjustedNextTtx > nextTtxUpstream_) {
                //just set the new ttx, no emergency signal needed
                timing.probeTime();
                nextTtxUpstream_ = adjustedNextTtx;
            } else if (adjustedNextTtx < nextTtxUpstream_) {
                //the newttx is lower than previous we stored - wakeup the fillsendingqueuethread 
                //as it might need to send erlier than it's timer suggests
                nextTtxUpstream_ = adjustedNextTtx;
                int rc = pthread_cond_signal(&measureTBUS_nextTtx_Emergency_cond);
                if (rc == EINVAL) {
                    log_DEBUG("MEME| setNewTtxForNextChirp(): Could not send broadcast signal for sleeping, because cond argument is invalid!");
                } else if (rc == 0) {
                    log_DEBUG("MEME| setNewTtxForNextChirp(): Wake up signal send to fillsendingQueue - lowered nextTtx from ",
                            integer(oldNextTtx), " to ", integer(nextTtxUpstream_));
                } else {
                    log_DEBUG("MEME| setNewTtxForNextChirp(): Could not send broadcast signal because some unknown error!");
                }
            } else { //same nextttx, so do nothing
            }
        }
    }
    pthread_mutex_unlock(&measureTBUS_nextttx_mutex);
}

/**
 * handles incoming dynamic payload (currently only from backchannel but maybe it is a good idea
 * to merge it with the dynpay from the safeguardchannel
 * @param dynPay
 */
void measureTBUS::handleIncomingFixedPayload(const TBUSFixedPayload & fixedPay) {
    static timeHandler timing(inputH.get_timingMethod());
    static int64_t nextTtx = 0LL;
    static int64_t lastEmergencyNextTtx = 0LL;
    static uint32_t nextTtxBasedOnChirpnr = 0L;
    static uint32_t nextTtxBasedOnPacketnr = 0L;
    static bool newTtx = false;
    static bool emergencySend = false;

    int64_t datarate = 0LL;
    uint32_t loss = 0L;
    int ret;
    //calculate the datarate and perfom icsic detection and calculate nextTtx
    ret = fastDatarateCalculator_.calculateDatarateAndNextTtx(fixedPay, datarate, loss, nextTtx, nextTtxBasedOnChirpnr, nextTtxBasedOnPacketnr, newTtx);
    if (ret != 0 && !newTtx) { //if neither new data rate nor new ttx was calculated
        if (DEBUG_TRDD) log_NOTICE("TRDD| Could not calculate data rate. ret=", integer(ret), " fixedPay=", fixedPay.toStringSend());
    } else { // send everything via backchannel
        TBUSDynamicPayload sendingDynpayPay(fixedPay.getChirpnr(), fixedPay.getPacketnr(), datarate, loss,
                nextTtx, nextTtxBasedOnChirpnr, nextTtxBasedOnPacketnr, nLastSafeguardChannelOWDDownstream_);

        pthread_mutex_lock(&measureTBUS_backchannel_mutex);
        backChannel_send_ = sendingDynpayPay;
        pthread_mutex_unlock(&measureTBUS_backchannel_mutex);

        //check if we need an emergency send of the dynamic payload via safeguarchannel
        timing.probeTime();
        if (newTtx
                && timing.getLastProbedTimeNs() + 3 * nLastSafeguardChannelOWDDownstream_ + ProcessTimer_ > nextTtx
                && timing.getLastProbedTimeNs() + nLastSafeguardChannelOWDDownstream_ + ProcessTimer_ < nextTtx
                && timing.getLastProbedTimeNs() >= network.getLastSafeguardChannelSendNs_() + networkHandler::SAFEGUARDCHANNEL_SENDING_INTEVAL_MINIMUM) {
            if (llabs(nextTtx - lastEmergencyNextTtx) > ProcessTimer_) { //only send a new emergency message if the ttx difffers minimum 5ms (ProcessTimer_) from last emergency message
                emergencySend = true;
                lastEmergencyNextTtx = nextTtx;
            } else {
                emergencySend = false;
            }
        } else {
            emergencySend = false;
        }
        network.setDynamicPayload(sendingDynpayPay, emergencySend);

        // assign new calculated data rate
        nFastDataCalcDatarate_ = datarate;
        //reset newTtx to false
        newTtx = false;
    }
}

/**
 * Will handle all incoming packets which includes:
 * - datarate and loss rate calculation 
 * - inter-chirp self-induced congestion detection
 * - preparing data for the backchannel and the safeguardchannel
 * 
 * @param payload the complete packet payload from the measurement packet
 * @return true
 */
int32_t measureTBUS::fastDataCalculator(payloadStruct & payload) {
    if (DEBUG_FASTDATACALCULATOR) log_DEBUG("TRDD| Entering fastDataCalculator()");

    //handle incoming dynamic Payload
    handleIncomingDynamicPayload(payload.dynPay, false);

    //handle incoming fixed Payload
    handleIncomingFixedPayload(payload.fixPay);
    if (DEBUG_FASTDATACALCULATOR) log_DEBUG("TRDD| Leaving fastDataCalculator()");
    return 0;
}

/* --------------------------------------------------------------------------------------------
 * A L G O  -  S A F E G U A R D   P A C K E T S
 * -------------------------------------------------------------------------------------------- */

/**
 * handles all incoming safeguardchannel packets
 * @param dynPay
 * @return 
 */
int32_t measureTBUS::processSafeguardPacket(const TBUSDynamicPayload & dynPay) {
    static int64_t lastConsoleLog = 0LL;
    log_DEBUG("TSRE| Entering ProcessSafeguardPacket()");
    handleIncomingDynamicPayload(dynPay, true);
    //log the packet
    dataFileWriterSafeguardReceiver(dynPay);

    if (dynPay.getReceivetimeNs() - lastConsoleLog >= CONSOLE_LOG_THROTTLE) {
        lastConsoleLog = dynPay.getReceivetimeNs();
        log_NOTICE("TSRE| Safeguard packet received ", dynPay.toLogSmall());
    }
    log_DEBUG("TSRE| Leaving ProcessSafeguardPacket()");
    return 0;
};

/* --------------------------------------------------------------------------------
 * N E E D E D   F O R   m e a s u r e m e n t M e t h o d C l a s s
 * -------------------------------------------------------------------------------- */

/**
 * Returns name for logfile with time, date, client address, server adress
 * @return string
 */
string measureTBUS::getLogFileName() {
    time_t result = time(NULL);
    char date[20];
    if (strftime(date, 20, "%Y%m%d-%H%M%S", localtime(&result)) == 0) { // This should never happen...

        log_ERROR("Error: An error occurred when the program tried to get the local date&time of the system. This is bad :(");
    }
    return inputH.get_inputLogfilePath() + "mTBUS/" + inputH.get_client_cAddress() + "_to_" + inputH.get_server_cAddress() + "/" + date + "/";
}

/**
 * Sets the loggingDone flag to input
 * @param input, bool
 * @param caller, string
 * @return 0
 */
int32_t measureTBUS::setLoggingDone(bool input, string caller) {
    log_DEBUG(caller, "| Entering set_loggingDone()");
    isLoggingDone_ = input;
    log_DEBUG(caller, "| Leaving set_loggingDone()");
    return 0;
}

int measureTBUS::getReceiveQueueSize() {
    //at least one chirp should fit
    int maxchirplength = inputH.amIServer() ? nMaximalChirpLengthUplink_ : nMaximalChirpLengthDownlink_;
    int sizeOfOneElement = sizeof (TBUSFixedPayload) + sizeof (TBUSDynamicPayload) + sizeof (payloadStruct);

    int ret = 1 * maxchirplength * sizeOfOneElement;
    //log_NOTICE("MEME| getReceiveQueueSize() returned ", integer(ret), ", ", integer(maxchirplength), " * ", integer(sizeOfOneElement));
    return ret;
}

int measureTBUS::getSentQueueSize() {
    //at least one chirp should fit
    int maxchirplength = inputH.amIServer() ? nMaximalChirpLengthDownlink_ : nMaximalChirpLengthUplink_;
    int sizeOfOneElement = sizeof (TBUSFixedPayload) + sizeof (TBUSDynamicPayload) + sizeof (payloadStruct);

    int ret = 1 * maxchirplength * sizeOfOneElement;
    //log_NOTICE("MEME| getSentQueueSize() returned ", integer(ret), ", ", integer(maxchirplength), " * ", integer(sizeOfOneElement));
    return ret;
}

int measureTBUS::getSendingQueueSize() {
    //at least one chirp should fit
    int maxchirplength = inputH.amIServer() ? nMaximalChirpLengthDownlink_ : nMaximalChirpLengthUplink_;
    int sizeOfOneElement = sizeof (TBUSFixedPayload);
    int ret = 1 * maxchirplength * sizeOfOneElement;
    //log_NOTICE("MEME| getSendingQueueSize() returned ", integer(ret), ", ", integer(maxchirplength), " * ", integer(sizeOfOneElement));
    return ret;
}

int measureTBUS::getSafeguardReceivingQueueSize() {
    return 0;
}//DEPRECATED

int measureTBUS::getSafeguardSendingQueueSize() {
    return 0;
} //DEPRECATED
