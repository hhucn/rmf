/* networkHandler.h */
#ifndef _networkHandler_
#define _networkHandler_

#define DEBUG_CHECK_IF_SOCKET_IS_READY (false)
#define DEBUG_RECEIVING_QUEUE_LOCKS (false)
#define DEBUG_SENDING_QUEUE_LOCKS (true)
#define DEBUG_SET_DYNAMIC_PAYLOAD (false)
#define DEBUG_SAFEGUARD_SENDING_QUEUE (true)

// Namespace
using namespace std;

// Includes
#include "inputHandler.h"
#include "circularQueue.h"
#include "routingHandler.h"
#include "networkThreadStructs.h"
#include "packetPayloads/fixedPayload.h"
#include "packetPayloads/packetPayload.h"
#include "packetPayloads//TBUSFixedPayload.h"
#include "packetPayloads//TBUSDynamicPayload.h"
#include <atomic> //std::atomic

//define some constants
#define HEADERSIZE_ETHERNET 14
#define HEADERSIZE_UDP 8
#define HEADERSIZE_IP4 20
#define HEADERSIZE_IP6 40

extern class inputHandler inputH;
extern class measureAssolo mAssolo;
extern class measureTBUS mTBUS;
extern class MeasureAlgo mAlgo;
extern class measureBasic mBasic;
extern class measureNew mNew;
extern class MeasureMobile mMobile;

/**
 * The networkHandler class manages all network related work and contains the threaded functions
 */
class networkHandler {
private:
    TBUSDynamicPayload dynamicPayload;
    TBUSDynamicPayload safeguardDynamicPayload;


    //char dynamic_payload[MAX_MEASUREMENT_CHANNEL_MESSAGE_LENGTH];
    //uint32_t dynamic_payload_length;

    // Read/Write Errorcounter
    uint32_t readErrorCounter_safeguardUDP, writeErrorCounter_safeguardUDP;

    uint32_t maxWaitSecondsRead;
    uint32_t maxWaitSecondsWrite;

    // Thread emergency Stop Flag
    std::atomic<bool> flag_emergencySTOP;

    //DEPRECATED
    //uint32_t flag_sentQueueToUse;
    //uint32_t flag_receivingQueueToUse;

    std::atomic<bool> flag_mSendingThreadCurrentlyWaitingForQueue;

    // Thread Ready Flags
    bool flag_mSendingThreadReady;
    std::atomic<bool> flag_mReceivingThreadReady;
    bool flag_receiverDataDistributerReady;
    bool flag_senderDataDistributerReady;
    bool flag_sSendingThreadReady;
    bool flag_sReceivingThreadReady;

    // Thread Done Flags
    std::atomic<bool> flag_mReceivingThreadDone;
    std::atomic<bool> flag_mSendingThreadDone;
    std::atomic<bool> flag_receiverDataDistributerDone;
    std::atomic<bool> flag_senderDataDistributerDone;
    std::atomic<bool> flag_sSendingThreadDone;
    std::atomic<bool> flag_sReceivingThreadDone;

    // Thread control flags guarded with mutex
    std::atomic<bool> flag_finishedFillingSendingQueue;
    std::atomic<bool> flag_sendingDone;
    std::atomic<bool> flag_measureReceivingDone;
    std::atomic<bool> flag_safeguardReceivingDone;

    // Queues to let Threads run independent
    circularQueue<TBUSFixedPayload> mSendingQueue{"SEND"};
    circularQueue<payloadStruct> mSentQueue{"SENT"};
    circularQueue<payloadStruct> mReceivingQueue{"RECV"};
    circularQueue<TBUSDynamicPayload> sSendingQueue{"SGCS"};
    circularQueue<TBUSDynamicPayload> sReceivingQueue{"SGCR"};

    // Sockets to connect to a server
    int clientSocketFD_Control_TCP;
    int clientSocketFD_Measure_UDP;
    int clientSocketFD_Safe_UDP;

    // Socket to accept an incoming tcp connection for the control channel
    int serverListenSocketFD_Control_TCP;
    // Sockets to listen for incoming connection requests
    int serverSocketFD_Control_TCP;
    int serverListenSocketFD_Measure_UDP;
    int serverListenSocketFD_Safe_UDP;

    // devicequeue
    int mSendingMaxQueueSize;

    std::atomic<int64_t> maxSendingRateMeasurementChannel;
    std::atomic<bool> throttleSendingRateMeasurementChannel;

    //routinghandler
    routingHandler::routingHandler * routingControl;
    routingHandler::routingHandler * routingMeasurement;
    routingHandler::routingHandler * routingSafeguard;

    //sendtime of next safeguardchannel packet
    std::atomic<int64_t> lastSafeguardChannelSendNs_;

    // remember to which client ip we are connected 
    string MeasurementChannelForeignIP;
    string SafeguadChannelForeignIP;

    // Create or connect a Socket/Connection
    int createSocket(string, uint32_t, int*, uint32_t, uint32_t, string);
    int createUdpChannel(int*, int*, string & foreignIP);
    int createTcpChannel(int*, int*);
    int connectToUdpChannel(string, uint32_t, int*, int*, uint32_t);
    int connectToTcpChannel(string, uint32_t, int*, uint32_t);

    int checkIfSocketIsReadyToRead(int, uint32_t, string);
    int checkIfSocketIsReadyToWrite(int, uint32_t, string);

    int ParseInteger(const std::string &, int *);
    int ParseServerIntegerSize(const std::string &);
    int SendSyncMessage(const uint32_t &, const uint32_t &,
            const uint32_t &, const uint32_t &, const uint32_t &,
            const uint32_t &, const uint32_t &, const uint32_t &,
            const string &, const string &, const string &, const string &,
            const string &);
    int WaitForSyncConfirmation(std::string *);

    int ParseMethod(const std::string &);
    int ParseMeasuredLink(const string &);
    int ParseExperimentDuration(std::string *);
    int ParseSafeguardStatus(std::string *);
    int ParseClientMeasureChannelPort(std::string *);
    int ParseClientSafeguardChannelPort(std::string *);
    int ParseServerMeasureChannelPort(std::string *);
    int ParseServerSafeguardChannelPort(std::string *);
    int ParseClientIntegerSize(std::string *);
    int ParseClientMeasureChannelAddress(std::string *);
    int ParseClientSafeguardChannelAddress(std::string *);
    int ParseMeasurementMagic(std::string *);
    int ParseSafeguardMagic(std::string *);
    int ParseFilenameFormat(std::string *);

    int WaitForSyncMessage(std::string *);
    int ParseSyncMessage(const std::string &);
    int SendSyncConfirmation();

public:
    static const int64_t SAFEGUARDCHANNEL_SENDING_INTERVAL_NORMAL = 100000000LL; //send a safeguard packet at most every 100ms
    static const int64_t SAFEGUARDCHANNEL_SENDING_INTEVAL_MINIMUM = 5000000LL; //send every 5ms if dramatic changes occur
    networkHandler();
    ~networkHandler();

    int initValues();

    int establishControlChannel(uint32_t, string, string, uint32_t, uint32_t, string, string, uint32_t);
    int establishMeasurementChannel(uint32_t, string, string, uint32_t, uint32_t, string, string, uint32_t);
    int establishSafeguardChannel(uint32_t, string, string, uint32_t, uint32_t, string, string, string, uint32_t);

    int writeToControlChannel(const uint32_t&, const string&, const uint32_t&, const string&);
    string readFromControlChannel(const uint32_t&, const uint32_t&, const string&, const bool logerror = false);

    int syncBasicParameter(const uint32_t &, const uint32_t &,
            const uint32_t &, const uint32_t &, const uint32_t &,
            const uint32_t &, const uint32_t &, const uint32_t &,
            const uint32_t &, const string &, const string &, const string &,
            const string &, const string &);
    int finishInit(uint32_t);
    void closeSockets();
    int getThreadStatus(string);

    // Measurement Sending Threads
    int threadedMeasurmentSender();
    int threadedSenderDataDistributer();

    // Measurement Receiving Threads
    int threadedMeasurmentReceiver();
    int threadedReceiverDataDistributer();

    // Safeguard Threads
    int threadedSafeguardSender();
    int threadedSafeguardReceiver();

    // Measurement Sending Queue Access
    int waitForEmptySendingQueue();
    int mSendingQueue_enqueue(TBUSFixedPayload & fixedPay);
    int getCurrentmSendingQueueSize();

    // Safeguard Queue Access
    int sSendingQueue_enqueue(TBUSDynamicPayload & dynPay);
    int sSendingQueue_enqueue(udp_safeguard_sendQueue);
    int sSendingQueue_enqueue(uint32_t id, uint32_t based_on_id, uint32_t payload_length, char* payload);
    int sReceivingQueue_clean();
    int sReceivingQueue_size();
    bool sReceivingQueue_isEmpty();
    int sReceivingQueue_getFront(TBUSDynamicPayload&, bool);
    int sReceivingQueue_getBack(TBUSDynamicPayload&, bool);

    // Dynamic Payload Access
    int setDynamicPayload(TBUSDynamicPayload & dynPay, bool safeguardEmergencySend = false);
    packetPayload safelyGetDynamicPayload();

    // nextTtx handling
    void setOWDSafeguardchannel(uint32_t delay);
    int64_t getLastSafeguardChannelSendNs_() const;

    void setMeasurementChannelSendingRateThrottle(int64_t maxRate, bool enable);
    ;

    // Control flag setting
    int set_finishedFillingSendingQueue(bool);
    int set_measureReceivingDone(bool);
    int set_safeguardReceivingDone(bool);
    bool set_sendingDone(bool);

    // Control flag getting
    bool get_finishedFillingSendingQueue();
    bool get_sendingDone();
    bool get_measureReceivingDone();
    bool get_safeguardReceivingDone();


    // The red button
    int set_emergencySTOP(string reason);

    bool get_emergencySTOP();

    // queue Getting

    circularQueue<TBUSFixedPayload> & getSendingQueue();

    circularQueue<payloadStruct> & getSentQueue();

    circularQueue<payloadStruct> & getReceiveQueue();

    circularQueue<TBUSDynamicPayload> & getSafeguardSendQueue();

    circularQueue<TBUSDynamicPayload> & getSafeguardReceivingQueue();
    uint32_t getHeaderSizeUDP();
    uint32_t getHeaderSizeIP();
    uint32_t getHeaderSizeEthernet();
    ;
    uint32_t getHeaderSizeEthernetIpUdp();
    int testMeasurementDeviceQueueSize();
    int unlockMeasurementDeviceSend();
    uint32_t getMeasurementDeviceQueueSize();
    int getMaxDeviceQueueSizeForInterface(const char *);

    string getMeasurementChannelForeignIP() const;
    string getSafeguadChannelForeignIP() const;

    
};

#endif /* _networkHandler_ */
