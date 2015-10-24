/* 
 * File:   packets.cpp
 * Author: goebel
 *
 * Created on 24. März 2015, 10:32
 */

// External logging framework
#include "pantheios/pantheiosHeader.h"
#include "networkThreadStructs.h"

#include "packets.h"
#include "timeHandler.h"
#include <arpa/inet.h> //ntohl
using namespace packets;


/**
 * Initialize an measurement packet for saving with the same paramter as in packet
 * Will use much less memory :-)
 * @param packetId current id
 * @param packetLength current length
 * @param sendtime_seconds timestamp
 * @param sendtime_nanoseconds timestamp
 * @param receivetime_seconds timestamp
 * @param receivetime_nanoseconds timestamp
 * @param fixedPayload char*
 * @param datarate for recent packet
 */
saved_measurement_packet::saved_measurement_packet(uint32_t packetId, uint32_t packetLength, int64_t sendtimeNs, int64_t receivetimeNs, char* fixedPayload, double datarate) {
    this->id = packetId;
    this->chirp_nr_ = packetId >> 16;
    this->packet_nr_ = packetId & 0xFFFF;
    this->sendtime_ = sendtimeNs;
    this->receivetime_ = receivetimeNs;

    // get chirp length and packet size
    uint32_t chirpData;
    memcpy(&chirpData, &fixedPayload[sizeof (uint32_t)], sizeof (uint32_t));
    chirpData = ntohl(chirpData);
    uint32_t chirpLength = chirpData >> 16;
    uint32_t sizeOfOtherPackets = chirpData & 0xFFFF;
    //cout << "\t" << saved_packet_.chirp_nr << " " << saved_packet_.packet_nr << " chirp data: " << chirpData << " " << chirpLength << " " << sizeOfOtherPackets << "\n";

    this->chirp_length_ = chirpLength;
    this->packetSize_ = packetLength;
    this->sizeOfOtherPackets_ = sizeOfOtherPackets;

    // gettin send time of the first packet in this chirp
    int64_t sendTimeFirstPacketNs;
    if (packet_nr_ == 1) {
        sendTimeFirstPacketNs = sendtimeNs;
    } else {
        //TODO NG: check this
        //        memcpy(&sendTimeFirstPacketSec, &fixedPayload[2 * sizeof (uint32_t)], sizeof (uint32_t));
        //        memcpy(&sendTimeFirstPacketNanosec, &fixedPayload[3 * sizeof (uint32_t)], sizeof (uint32_t));
        //        sendTimeFirstPacketSec = ntohl(sendTimeFirstPacketSec);
        //        sendTimeFirstPacketNanosec = ntohl(sendTimeFirstPacketNanosec);
        sendTimeFirstPacketNs = timeHandler::timespec_to_ns(ntohl(((uint32_t*) fixedPayload)[2]), ntohl(((uint32_t*) fixedPayload)[3]));
    }
    sendTimeFirstPacket_ = sendTimeFirstPacketNs;

    this->datarate_ = datarate;

    pantheios::log_DEBUG("SICD| Creating measurement packet for saving with: ", "ID: ", pantheios::integer(packetId), ", chirp: ",
            pantheios::integer(packetId >> 16), ", packet: ", pantheios::integer(packetId & 0xFFFF), ", send time : ",
            pantheios::integer(sendtimeNs), ", receive time : ", pantheios::integer(receivetimeNs), ", chirp length: ",
            pantheios::integer(chirpLength), ", packet length: ", pantheios::integer(packetLength), ", send time first packet: ",
            pantheios::integer(sendTimeFirstPacketNs));
}

/**
 * Initialize an safeguard packet for saving
 * @param id packet id for the backchannel
 * @param based_on_id id where the infos are based on
 * @param receivetime_seconds timestamp
 * @param receivetime_nanoseconds timestamp
 * @param payload_length length of payload
 * @param payload char*
 */
SavedSafeguardPacket::SavedSafeguardPacket(uint32_t id, uint32_t based_on_id, int64_t receivetimeNs,uint32_t payload_length, char* payload) {
    this->id_ = id;
    this->based_on_id_ = based_on_id;
    this->receivetime_ = receivetimeNs;
    this->payload_length_ = payload_length;
    this->payload_ = payload;

    this->sendtime_ = timeHandler::timespec_to_ns(ntohl(((uint32_t*) payload)[4]), ntohl(((uint32_t*) payload)[5]));
}


