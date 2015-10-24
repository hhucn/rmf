/*
 * File:   packets.h
 * Author: goebel
 *
 * Created on 24. März 2015, 10:02
 */

#ifndef PACKETS_H
#define	PACKETS_H

#include <stdint.h>

namespace packets {

    class saved_measurement_packet {
    public:
        // basic fields
        uint32_t id;
        int64_t sendtime_;
        int64_t receivetime_;
        // extended version :-)
        uint32_t datarate_;
        uint32_t chirp_nr_;
        uint32_t packet_nr_;
        uint32_t packetSize_;
        uint32_t sizeOfOtherPackets_;
        uint32_t chirp_length_;
        int64_t  sendTimeFirstPacket_;
        
        int64_t getDelay() const {return receivetime_ - sendtime_;}; 
        
        saved_measurement_packet(uint32_t packetId, uint32_t packetLength, int64_t sendtimeNs, int64_t receivetimeNs, 
                char* fixedPayload, double datarate);
    };

    class SavedSafeguardPacket {
    public:
        uint32_t id_;
        uint32_t based_on_id_;
        int64_t sendtime_;
        int64_t receivetime_;
        uint32_t payload_length_;
        char* payload_;
        
        int64_t getDelay() const {return receivetime_ - sendtime_;}; 
        
        SavedSafeguardPacket(uint32_t id, uint32_t based_on_id, int64_t receivetimeNs,uint32_t payload_length, char* payload);
    };
};

#endif	/* PACKETS_H */

