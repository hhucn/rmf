/* 
 * File:   TBUSDynamicPayload.h
 * Author: goebel
 *
 * Created on 2. April 2015, 17:33
 */

#ifndef TBUSDYNAMICPAYLOAD_H
#define	TBUSDYNAMICPAYLOAD_H

#include <stdint.h>
#include "packetPayload.h"
#include <sstream> //stringstream

class TBUSDynamicPayload : public packetPayload {
private:
    uint64_t datarate_; //in Byte/s
    int64_t nextTtx_; //Ttx calculated for nextTtxBasedOnChirpnr+1
    uint32_t nextTtxBasedOnChirpnr_;
    uint32_t nextTtxBasedOnPacketnr_;
    uint32_t basedOnChirpNr_; //the chirpnr loss and datarate are based on
    uint32_t basedOnPacketNr_; //the packetnr loss and datarate are based on
    uint32_t loss_; // arrived / sent * 1000000 
    uint32_t lastValidSafeguardChannelDelay_; //the last valid received one way delay in the safeguardchannel
public:
    static const uint32_t payloadsize_ = 6 * sizeof (uint32_t) + 3 * sizeof (uint64_t);

    TBUSDynamicPayload() {
    };
    TBUSDynamicPayload(char* buf, int64_t receivetimeNs = -1LL);
    explicit TBUSDynamicPayload(const uint32_t basedOnChirpnr, const int32_t basedOnPacketnr, const uint64_t datarate, 
            const uint32_t loss , const int64_t nextTtx, const uint32_t nextTtxBasedOnChirpnr, const uint32_t nextTtxBasedOnPacketnr,  
            const uint32_t lastValidSafeguardChannelDelay);
    explicit TBUSDynamicPayload(const TBUSDynamicPayload & dynPay, const uint32_t basedOnChirpnr, const uint32_t basedOnPacketnr, 
            const uint64_t datarate, const uint32_t loss,  const uint32_t lastValidSafeguardChannelDelay);

    virtual int insertPayloadInPacket(char* buf) const override;
    virtual uint32_t getPayloadSize() const override;
    uint32_t getBasedOnChirpnr() const;
    uint32_t getBasedOnPacketnr() const;
    virtual uint64_t getDatarate() const;
    virtual uint32_t getLoss() const;
    virtual uint32_t getNextTtxBasedOnChirpnr() const;
    virtual uint32_t getNextTtxBasedOnPacketnr() const;
    virtual int64_t getNextTtx() const;
    virtual uint32_t getLastValidSafeguardChannelDelay_() const;
    
    virtual bool isNewerThan(const TBUSDynamicPayload & dynPay) const;

    virtual std::string toStringSend(string separator=",") const override;
    virtual std::string getStringHeaderSend(string separator=",") const override;
    virtual std::string toStringReceived(string separator=",") const override;
    virtual std::string getStringHeaderReceived(string separator=",") const override;
    std::string toLogSmall() const;
};

#endif	/* TBUSDYNAMICPAYLOAD_H */

