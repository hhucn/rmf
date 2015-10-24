/* 
 * File:   fixedPayload.h
 * Author: goebel
 *
 * Created on 3. April 2015, 17:53
 */

#ifndef FIXEDPAYLOAD_H
#define	FIXEDPAYLOAD_H

#include <stdint.h>
#include <time.h>
#include "packetPayload.h"

/**
 * All fixedpaylouds should inherit from this abstract class
 * they should implement a suitable constructor from char*buf
 * and inserPayloadInPacket(char*buf)
 * 
 * serializing and deserializing to network order is implemented in the 
 * inherited class in those functions. you can use the macros GET_NETWORK.. and
 * SET_NETWORK.. from above - pos thereby increments in 8 Byte or 4 Byte 
 * depending on the variable length!
 * Thus first put the 64bit fields into the buffer followed by the 32bit values
 * to allow for correct alignment.
 * 
 * @param buf
 * @return 
 */
class fixedPayload : public packetPayload {
protected:
    uint32_t packetlength_; //the size of the complete measurement packet
    
public:
    fixedPayload(uint32_t packetlength, int64_t sendtimeNs, int64_t receivetimeNs = -1LL)
    : packetlength_(packetlength) {
        sendtimeNs_ = sendtimeNs;
        receivetimeNs_ = receivetimeNs;
    }; //use this in the derived class to initialize the members
    
    fixedPayload(char* buf, int64_t receivetimeNs = -1LL) {
        //do something with the char buffer
        receivetimeNs_ = receivetimeNs;
    }; //create an instance from a char buffer - implement this in derived classes!

    virtual int insertPayloadInPacket(char* buf) const override {
        return -1;
    }; //insert the data of the struct into the packet

    virtual uint32_t getPayloadSize() const {
        return sizeof (uint32_t) + sizeof (int64_t);
    }; //return the size of the payload defined in the class

    fixedPayload() {
    };

    virtual uint32_t getPacketLength() const {
        return packetlength_;
    }; // get the size of the complete measurement packet
    
    
};


#endif	/* FIXEDPAYLOAD_H */

