/* 
 * File:   TBUS_FastDatarateCalculator.h
 * Author: goebel
 *
 * Created on 31. März 2015, 13:23
 */

#ifndef TBUS_FASTDATARATECALCULATOR_H
#define	TBUS_FASTDATARATECALCULATOR_H

#define LOSSFACTOR 1000000000LL

#include <stdint.h>
#include "../../networkThreadStructs.h"
#include "../../packetPayloads/fixedPayload.h"
#include <vector>
#include <fstream>   

class measureTBUS;

// class for calculating datarate
class TBUS_FastDatarateCalculator {
private:

    uint32_t nRecBytes_;
    uint32_t nChirpLength_;
    uint32_t nPacketCounter_;
    int64_t nCurrReceiveFirstNs_;
    int64_t nDatarateLastChirp_;
    int64_t nDatarateCurrentChirp_;
    uint32_t nLossLastChirp_; // loss = (1 - received/sent) * LOSSFACTOR
    uint32_t nLossCurrentChirp_;
    uint32_t nLastChirpNr_;
    uint32_t nLastPacketNr_;
    uint32_t sp_i;
    uint32_t sp_0;
    TBUSFixedPayload currentChirpFront_;
    TBUSFixedPayload currentChirpBack_;
    uint32_t currentChirpReceivedPackets_;
    static const int64_t t_guard = 50 * 1000L * 1000L; //50ms guardtime between chirps
    int64_t lastValidDelay;

    //log some calculation information
    measureTBUS * mTBUS_;


public:
    TBUS_FastDatarateCalculator(const uint32_t minimalPacketSize = 74, const uint32_t maxChirpLength = 6473800L); //please fill with something much more useful!
    ~TBUS_FastDatarateCalculator();
    int32_t calculateDatarate(const TBUSFixedPayload &, int64_t & datarate, uint32_t & loss);
    void calculateNextTtxAndSetIt(const TBUSFixedPayload & fixedPay, const int64_t & a_i,
            int64_t & nextTtx, uint32_t & nextTtxBasedOnChirpnr, uint32_t & nextTtxBasedOnPacketnr, bool & newTtx);
    void checkForICSICBetweenChirps(const TBUSFixedPayload & fixedPay, const int64_t & datarate, int64_t & nextTtx,
            uint32_t & nextTtxBasedOnChirpnr, uint32_t & nextTtxBasedOnPacketnr, bool & newTtx);
    int32_t calculateDatarateAndNextTtx(const TBUSFixedPayload & fixPay, int64_t & datarate, uint32_t & loss,
            int64_t & nextTtx, uint32_t & nextTtxBasedOnChirpnr, uint32_t & nextTtxBasedOnPacketnr, bool & newTtx);
    inline uint32_t calculateLossRate(uint32_t received, uint32_t send);
    inline int64_t calculateDatarate(uint32_t bytesReceived, int64_t timespanNs);
    void setmTBUS(measureTBUS * tbusPointer);

    int64_t getDatarateCurrentChirp() {
        return nDatarateCurrentChirp_;
    };

    int64_t getDatarateLastChirp() {
        return nDatarateLastChirp_;
    };

    uint32_t get_lossCurrentChirp() {
        return nLossCurrentChirp_;
    };

    uint32_t get_lossLastChirp() {
        return nLossLastChirp_;
    };
};

#endif	/* TBUS_FASTDATARATECALCULATOR_H */

