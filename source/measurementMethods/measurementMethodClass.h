#ifndef _measurementMethodeClass_
#define _measurementMethodeClass_
#include "../networkThreadStructs.h"

#include "../gpsDate.h"
#include <stdlib.h>
#include <string.h>

class measurementMethodClass {
public:

    virtual ~measurementMethodClass();
    virtual int initMeasurement() = 0; // will be called during the initialisation of a measurement run

    virtual int fastDataCalculator(payloadStruct&) = 0; // will be called if an element is received
    virtual int dataFileWriterMeasureReceiver(const payloadStruct &) = 0; // will be called if an element is received!
    virtual int32_t dataFileWriterSafeguardSender(const TBUSDynamicPayload & dynpay)=0;
    virtual int32_t dataFileWriterSafeguardReceiver(const TBUSDynamicPayload & dynpay)=0;
    virtual int dataFileWriterMeasureSender(const payloadStruct&);
    virtual int setLoggingDone(bool, string) = 0; // will be called from various methods

    //  process incoming safeguardpackets
    virtual int processSafeguardPacket(const TBUSDynamicPayload&) = 0;

    virtual string getLogFileName() = 0; // returns a logfilename for this measurement class
    virtual int get_Id() = 0; // returns a unique id for the measurement class
    virtual string get_Name() = 0; // returns the name for the measurement class. This name will be used for command line option parsing.
    virtual int getSendingQueueSize();
    virtual int getSentQueueSize();
    virtual int getReceiveQueueSize();
    virtual int getSafeguardSendingQueueSize();
    virtual int getSafeguardReceivingQueueSize();
    virtual int getMaxFixedPayloadSize();
    virtual packetPayload* getFixedPacketPayloadClass() = 0;
    virtual packetPayload* getDynamicPacketPayloadClass() = 0;
    virtual bool syncingDone() = 0;
  
};
#endif

