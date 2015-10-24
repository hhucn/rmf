/**
 * @authors Tobias Krauthoff <tobias.krauthoff@hhu.de, krauthoff@cs.uni-duesseldorf.de>
 * @date 23. June 2014
 * adaptive measurment method with self induced congestion detection
 * INFORMATION: The countdown timer is currently not used!
 */

#ifndef measureTBUS_H_
#define measureTBUS_H_

#define DEBUG_DATAFILEWRITER (false)
#define DEBUB_FILLSENDINGQUEUE (false)
#define DEBUG_FASTDATACALCULATOR (false)
#define DEBUG_ENQUEUE_CHIRP (true)
#define DEBUG_TRDD (false)

// Namespace
//using namespace std;

#include "../countdownTimer.h"
#include "../timeHandler.h"
#include "TBUS/TBUS_FastDatarateCalculator.h"
#include "../networkHandler.h"
#include "../packetPayloads/TBUSDynamicPayload.h"
#include "../packetPayloads/TBUSFixedPayload.h"
#include "../packetPayloads/packetPayload.h"
#include <atomic> //std::atomic
#include "measurementMethodClass.h"

// Include C++ header files
#include <stdint.h> //uint32_t

#include "../rmfMakros.h"

// Forward declaration
extern class inputHandler inputH;
extern class networkHandler network;

// the real measurement class

class measureTBUS : public measurementMethodClass {
private:
    // DEPRECATED NtpHelper ntpHelper_;
    TBUS_FastDatarateCalculator fastDatarateCalculator_;
    TBUSDynamicPayload backChannel_send_;
    TBUSDynamicPayload backChannel_received_;
//    CountdownTimer countdownTimerFeedback_;
//    CountdownTimer countdownTimerTimeout_;

    udp_measure_receiveQueue lastReceivedPacket_;

    uint32_t nChannelload_;
    uint32_t nCheckSicEveryKthPacket_;
    uint32_t nChirpLength_;
    uint32_t nCountdownTimerSec_;
    uint32_t nCountdownTimerNSec_;
    uint32_t nCountSmoothening_;
    int64_t ExpEndtime_;
    int64_t ExpStarttime_;
    uint32_t nPacketSize_;
    uint32_t nPacketHeaderSizeEthernetIpUdp_;
    uint32_t nMinimalPacketSize_;
    uint32_t nMaximalChirpLengthUplink_;
    uint32_t nMaximalChirpLengthDownlink_;
    int64_t nMaximalDatarateUplink_; //byte/s
    int64_t nMaximalDatarateDownlink_; //byte/s
    uint32_t nPacktIdOfCurrentCongestedPacket_;

    int64_t nChirpDuration_;
    uint64_t nCurrentDatarate_; //byte/s
    uint64_t nLastValidDatarate_; //byte/s
    uint64_t nInitialDatarate_; //byte/s
    uint64_t nFastDataCalcDatarate_; //byte/s
    std::atomic<uint32_t> nLastSafeguardChannelOWDDownstream_;
    std::atomic<uint32_t> nLastSafeguardChannelOWDUpstream_;
    std::atomic<bool> syncingDone_;

    int64_t ProcessTimer_;
    int64_t lastChirpStart_;
    int64_t congestionSleepUntil_;
    int64_t lastSafeguardPacketBecauseCongestionSent_;
    int64_t lastSafeguardPacketBecauseTimeoutSent_;
    int64_t nextTimeToSleep_;
    int64_t lastTimeToSleep_;
    int64_t lastAbsoluteTimeToSleep_;

    //guarded by measureTBUS_nextttx_mutex
    int64_t nextTtxUpstream_; //the transmit time for our next chirp if congestion occurs (received via backchannel or safeguard)
    int64_t nextTtxUpstreamNormal_; // the transmit time for our next chirp if no congestion occurs
    uint32_t nextChirpnr_;

    ofstream outputFileReceiverRaw_;
    ofstream outputFileReceiverDelay_;
    ofstream outputFileReceiverDatarate_;
    ofstream outputFileReceiverNewTtx_;
    ofstream outputFileReceiverGaps_;
    ofstream outputFileSenderNewTtx_;
    ofstream outputFileSenderRaw_;
    ofstream safeguardFileReceiver_;
    ofstream safeguardFileSender_;

    static const bool debugDataFileWriter = false;
    bool isAdaptive_;
    bool isLoggingDone_;
    bool isUseOfCongestionDetector_;
    bool adjustOnWrongInitialDatarate_;

    // open all logfiles
    int32_t openLogfile_measureSender();
    int32_t openLogfile_measureReceiver();
    int32_t openLogfile_measureReceiverDatarate();
    int32_t openLogfile_measureReceiverNewTtx();
    int32_t openLogfile_measureReceiverGaps();
    int32_t openLogfile_measureSenderNewTtx();
    int32_t openLogfile_measureReceiverDelay();
    int32_t openLogfile_safeguardSender();
    int32_t openLogfile_safeguardReceiver();
    int32_t openLogfiles();
    int32_t closeEveryFile();
    int32_t stopRunningThreads();

    int32_t initValues();

    // parse input
    int32_t parse_parameter(string &);
    int32_t search_parameter(const string, string &, string &);
    int32_t parse_adaptSizes(string &);
    int32_t parse_startRate(string &);
    int32_t parse_channelload(string &);
    int32_t parse_congestionCheckPacket(string &);
    int32_t parse_chirpLengthDownlink(string &);
    int32_t parse_chirpLengthUplink(string &);
    int32_t parse_maximalDatarateUplink(string &);
    int32_t parse_maximalDatarateDownlink(string &);
    int32_t parse_useOfCongestionDector(string &);
    
    int32_t parse_integer(const string, int32_t &);
    int32_t parse_long(const string input, int64_t &retval);
    int32_t parse_double(const string, double &);
    int32_t parse_boolean(const string, bool &);
    int32_t extractSyncParameter(string&, string &);

    // check for killed threads
    int32_t get_countOfDeadThreads();

    // sync with server <-> client
    int32_t syncMeasurementParemeter();
    int32_t sendSyncParameter();
    int32_t receiveSyncParameter();
    int sendFinishMesurementMessage();

    // put some packet in the sending queue
    int32_t fillSendingQueue();
    int32_t getLatestInformationOutOfChannels(uint32_t &basedOnChirp, uint32_t & basedOnPacket, int64_t & datarate);
    int32_t enqueueChirp(const int64_t nextTtx);

    // fastDatacalculator helperfunctions
    void handleIncomingDynamicPayload(const TBUSDynamicPayload & dynpay, bool arrivedViaSafeguard = false);
    void handleIncomingFixedPayload(const TBUSFixedPayload & fixedPay);
    void setNewTtxForNextChirp(const TBUSDynamicPayload & dynPay);

    //open logfiles and paste headers
    int32_t openLogFile(const string fileNameSuffix, ofstream & file, const string & header, const int precision);

    // and some calculations
    int64_t get_timeSinceLastChirpWasEnqueued();
    int32_t calculateChirpAndChannelProperties(uint32_t chirpnr, uint32_t packetnr);

    // other methods
    int32_t startMeasurement(timeHandler&);
    int32_t endMeasurement(timeHandler&);
    int32_t initHelperClassesAndObjects();

public:
    static const uint32_t ETHERNET_MTU_SIZE;
    static const int64_t MEASUREMENT_INTERVALL;
    static const uint32_t ID = 4;
    static const uint32_t kSERIALIZED_FIXED_PAYLOAD_LENGTH;

    measureTBUS();
    virtual ~measureTBUS();
    int32_t initMeasurement();

    struct udp_measure_receiveQueue recPackets[MAX_CHIRP_LENGTH]; // new and delete are not threadsafe, we have to use static "allocation"


    // writing logfiles
    virtual int32_t dataFileWriterMeasureReceiver(const payloadStruct & payload) override;
    virtual int32_t dataFileWriterMeasureSender(const payloadStruct & payload) override;
    virtual int32_t dataFileWriterSafeguardSender(const TBUSDynamicPayload & dynpay) override;
    virtual int32_t dataFileWriterSafeguardReceiver(const TBUSDynamicPayload & dynpay) override;

    //only valid for measureTBUS and TBUS_FastDatarateCalculator
    int32_t dataFileWriterMeasureReceiverDelay(const TBUSFixedPayload & fixedpay);
    int32_t dataFileWriterMeasureReceiverDatarate(const TBUSFixedPayload & fixedpay, const int64_t datarate, const uint32_t lossrate,const bool datarateUsed);
    int32_t dataFileWriterMeasureReceiverNewTtx(const TBUSFixedPayload & fixedpay, const int64_t newTtx, const int64_t normalTtx, const int64_t diff, const bool sendLater);
    int32_t dataFileWriterMeasureReceiverGaps(const TBUSFixedPayload & back, const TBUSFixedPayload & next,const int64_t  t_gap_i,const int64_t delta_trx_i,const int64_t trxdiff,const int64_t ttxdiff);

    // called by network handler
    int32_t setLoggingDone(bool, string);

    uint32_t get_chirpsize() {
        return (this->nChirpLength_);
    }

    virtual int32_t processSafeguardPacket(const TBUSDynamicPayload & dynPay) override;

    virtual int32_t fastDataCalculator(payloadStruct&) override;

    virtual bool syncingDone() override {
        return syncingDone_;
    }

    // Needed for measurementMethodClass
    string getLogFileName();

    int32_t get_Id() {
        return ID;
    };

    string get_Name() {
        const string name = "TBUS";
        return name;
    }

    virtual int getSendingQueueSize() override;

    virtual int getSentQueueSize() override;

    virtual int getReceiveQueueSize() override;

    virtual int getSafeguardSendingQueueSize() override;

    virtual int getSafeguardReceivingQueueSize() override;

    int getMaxFixedPayloadSize() {
        return 1444 * 3; // algo tries to send to large packets. testing it with * 3
    };

    virtual packetPayload* getFixedPacketPayloadClass()override {

        static TBUSFixedPayload* fp = new (TBUSFixedPayload);
        return fp;
    }; //returns the class of the fixed payload packets of the measurement method

    virtual packetPayload* getDynamicPacketPayloadClass() override {
        static TBUSDynamicPayload* dp = new (TBUSDynamicPayload);
        return dp;
    }; //returns the class of the fixed payload packets of the measurement method;

};


#endif /* measureTBUS_H_ */
