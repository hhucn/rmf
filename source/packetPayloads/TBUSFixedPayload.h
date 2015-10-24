/* 
 * File:   TBUSFixedPayload.h
 * Author: goebel
 *
 * Created on 2. April 2015, 16:07
 */

#ifndef TBUSFIXEDPAYLOAD_H
#define	TBUSFIXEDPAYLOAD_H

#include "fixedPayload.h"
#include <stdint.h>

class TBUSFixedPayload : public fixedPayload {
private:
    //int64_t sendtimeNs_; //send time of this packet (already contained in super class)
    //int64_t receivetimeNs_; //the receive time of this packet (receiver side only) -1 indicates not set
    int64_t sendtimeFirstNs_; //the transmit time of the first packet of this chirp
    uint32_t chirpnr_;
    uint32_t packetnr_;
    //uint32_t packetlength_; //already contained in super class
    uint32_t chirplength_;
    uint32_t measurementPacketLength_; //the size of packets 2..n in each chirp
public:
    static const uint32_t payloadsize_ = 5 * sizeof (uint32_t) + 2 * sizeof (uint64_t);

    TBUSFixedPayload() {
    };
    TBUSFixedPayload(char* buf, int64_t receivetimeNs = -1);

    //sendtimeNs_(sendtimeNs), sendtimeFirstNs_(sendtimeFirstNs), chirpnr_(chirpnr), packetnr_(packetnr),packetlength_(packetlength),  chirplength_(chirplength),

    TBUSFixedPayload(int64_t sendtimeNs, int64_t sendtimeFirstNs, uint32_t chirpnr, uint32_t packetnr, uint32_t packetlength, uint32_t chirplength, uint32_t measurementPacketLength) :
    sendtimeFirstNs_(sendtimeFirstNs), chirpnr_(chirpnr), packetnr_(packetnr), chirplength_(chirplength), measurementPacketLength_(measurementPacketLength) {
        sendtimeNs_ = sendtimeNs;
        packetlength_ = packetlength;
        //fixedPayload::fixedPayload(packetlength, sendtimeNs),
    }
    virtual int insertPayloadInPacket(char* buf) const override;
    virtual uint32_t getPayloadSize() const override;
    virtual void setPacketNr(uint32_t packetnr);
    virtual uint32_t getChirplength() const;
    virtual uint32_t getChirpnr() const;
    virtual uint32_t getPacketnr() const;
    virtual int64_t getSendtimeFirstNs() const;
    virtual void setSendtimeFirstNs(timespec time);
    virtual void setSendtimeFirstNs(int64_t time);

    virtual uint32_t getMeasurementPacketLength() const; //this can be used to use "packetlength_" for something else if the receiver can decide the correct length lateron

    virtual std::string toStringSend(string separator = ",") const override;
    virtual std::string getStringHeaderSend(string separator = ",") const override;
    virtual std::string toStringReceived(string separator = ",") const override;
    virtual std::string getStringHeaderReceived(string separator = ",") const override;

};

#endif	/* TBUSFIXEDPAYLOAD_H */

