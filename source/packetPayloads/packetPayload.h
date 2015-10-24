/* 
 * File:   packetPayload.h
 * Author: Norbert Goebel
 *
 * Created on 2. April 2015, 15:30
 */

#if defined(__linux__)
#include <endian.h>
#elif defined(__FreeBSD__) || defined(__NetBSD__)
#include <sys/endian.h>
#elif defined(__OpenBSD__)
#include <sys/types.h>
#define be16toh(x) betoh16(x)
#define be32toh(x) betoh32(x)
#define be64toh(x) betoh64(x)
#endif

#define GET_NETWORK_UINT64(buf,pos) (be64toh(((uint64_t*) buf)[pos]))
#define GET_NETWORK_UINT32(buf,pos) (be32toh(((uint32_t*) buf)[pos]))
#define SET_NETWORK_UINT64(var,buf,pos) (((uint64_t*) buf)[pos]=htobe64(var))
#define SET_NETWORK_UINT32(var,buf,pos) (((uint32_t*) buf)[pos]=htobe32(var))

#ifndef PACKETPAYLOAD_H
#define	PACKETPAYLOAD_H

#include <stdint.h>
#include <time.h>
#include <string>
#include "../timeHandler.h"
#include <sstream> //stringstream

/**
 * All paylouds should inherit from this abstract class
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
class packetPayload {
protected:
    int64_t sendtimeNs_; //send time of this packet    (-1 if not set)
    int64_t receivetimeNs_; //the receive time of this payload (set at the receiver and not included in network packets) (-1 if not set)

    packetPayload(int64_t sendtimeNs, int64_t receivetimeNs)
    : sendtimeNs_(sendtimeNs), receivetimeNs_(receivetimeNs) {
    };
public:

    packetPayload() : sendtimeNs_(-1LL), receivetimeNs_(-1LL) {
    };

    virtual int insertPayloadInPacket(char* buf) const {
        return 0;
    } //insert the data of the struct into the packet

    virtual uint32_t getPayloadSize() const {
        return 0;
    }; //return the size of the payload defined in the class

    virtual int64_t getSendtimeNs() const {
        return sendtimeNs_;
    }; //get the sending time of the payload

    virtual int64_t getReceivetimeNs() const {
        return receivetimeNs_;
    }; //get the receiving time of the packet (at receiver side) 

    virtual void setSendtimeNs(timespec time) {//allows altering the sendtime prior fetching the packetpayload
        sendtimeNs_ = timeHandler::timespec_to_ns(time);
    }

    virtual void setSendtimeNs(int64_t time) {//allows altering the sendtime prior fetching the packetpayload
        sendtimeNs_ = time;
    }

    virtual void setReceivetimeNs(timespec time) {//allows altering the sendtime prior fetching the packetpayload
        receivetimeNs_ = timeHandler::timespec_to_ns(time);
    }

    virtual void setReceiveimeNs(int64_t time) {//allows altering the sendtime prior fetching the packetpayload
        receivetimeNs_ = time;
    }

    /**
     * get the Delay of this Packet
     * @return  -1LL if either receivetimeNs or sendtimeNS is not set
     *          receivetimeNs - sendtimeNs
     */
    virtual int64_t getPacketDelay() const {
        if (receivetimeNs_ <= 0 || sendtimeNs_ <= 0) {
            return -1LL;
        } else {
            return receivetimeNs_ - sendtimeNs_;
        }
    }

    /*
     * used for logging - headers for the logfile go here
     */
    virtual std::string toStringSend(string separator = ",") const {
        std::stringstream stream;
        stream << sendtimeNs_;
        return stream.str();
    }

    virtual std::string getStringHeaderSend(string separator = ",") const {
        std::stringstream stream;
        stream << "SendTime[ns]";
        return stream.str();
    }

    virtual std::string toStringReceived(string separator = ",") const {
        std::stringstream stream;
        stream << sendtimeNs_ << separator << receivetimeNs_;
        return stream.str();
    }

    virtual std::string getStringHeaderReceived(string separator = ",") const {
        std::stringstream stream;
        stream << "SendTime[ns]" << separator << "ReceiveTie[ns]";
        return stream.str();
    }

};

#endif	/* PACKETPAYLOAD_H */

