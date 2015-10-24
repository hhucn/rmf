/* 
 * File:   TBUSFixedPayload.cpp
 * Author: goebel
 * 
 * Created on 2. April 2015, 16:07
 */

#include "TBUSFixedPayload.h"
#include "../timeHandler.h"
#include "packetPayload.h"
#include <iomanip>

/**
 * create object from char buffer (which can be discarded after the object
 * it created)
 * this converts from network to host order
 * 
 * @param buf
 */
TBUSFixedPayload::TBUSFixedPayload(char * buf, int64_t receivetimeNs) {
    sendtimeNs_ = GET_NETWORK_UINT64(buf, 0); //warning, pos here is in 8 bytes!
    sendtimeFirstNs_ = GET_NETWORK_UINT64(buf, 1); //warning, pos here is in 8 bytes!
    chirpnr_ = GET_NETWORK_UINT32(buf, 4); //pos is going in 4 byte steps!
    packetnr_ = GET_NETWORK_UINT32(buf, 5); //pos is going in 4 byte steps!
    packetlength_ = GET_NETWORK_UINT32(buf, 6); //pos is going in 4 byte steps!
    chirplength_ = GET_NETWORK_UINT32(buf, 7); //pos is going in 4 byte steps!
    measurementPacketLength_ = GET_NETWORK_UINT32(buf, 8); //pos is going in 4 byte steps!
    receivetimeNs_ = receivetimeNs; //not included in buffer but can be set by receiving thread
}

/**
 * Insert the payload into the given buffer (which hopefully is long enogh
 * Remember to implement the same conversion in the char*buf constructor!
 * 
 * @param buf
 * @return -1 if something went wrong, else it returns 0
 */
int TBUSFixedPayload::insertPayloadInPacket(char* buf) const {
    SET_NETWORK_UINT64(sendtimeNs_, buf, 0);
    SET_NETWORK_UINT64(sendtimeFirstNs_, buf, 1);
    SET_NETWORK_UINT32(chirpnr_, buf, 4);
    SET_NETWORK_UINT32(packetnr_, buf, 5);
    SET_NETWORK_UINT32(packetlength_, buf, 6);
    SET_NETWORK_UINT32(chirplength_, buf, 7);
    SET_NETWORK_UINT32(measurementPacketLength_, buf, 8);
    return 0;
}

uint32_t TBUSFixedPayload::getPayloadSize() const {
    return payloadsize_;
    //return 4 * sizeof (uint32_t) + 2 * sizeof (uint64_t);}
}

void TBUSFixedPayload::setPacketNr(uint32_t packetnr) {
    packetnr_ = packetnr;
}

uint32_t TBUSFixedPayload::getPacketnr() const {
    return packetnr_;
}

uint32_t TBUSFixedPayload::getChirplength() const {
    return chirplength_;
}

uint32_t TBUSFixedPayload::getChirpnr() const {
    return chirpnr_;
}

int64_t TBUSFixedPayload::getSendtimeFirstNs() const {
    return sendtimeFirstNs_;
}

void TBUSFixedPayload::setSendtimeFirstNs(int64_t time) {
    this->sendtimeFirstNs_ = time;
}

void TBUSFixedPayload::setSendtimeFirstNs(timespec time) {
    this->sendtimeFirstNs_ = timeHandler::timespec_to_ns(time);
}

uint32_t TBUSFixedPayload::getMeasurementPacketLength() const {
    return measurementPacketLength_;
}

std::string TBUSFixedPayload::getStringHeaderReceived(string separator) const {
    std::stringstream stream;
    stream << "    ttx[ns]     " << separator << "      trx[ns]      " << separator << "Chirp" << separator << "Pack " <<
            separator << "#Pack" << separator << "Byte/Packet" << separator <<
            "ttx_first[ns]";
    return stream.str();
}

std::string TBUSFixedPayload::getStringHeaderSend(string separator) const {
    std::stringstream stream;
    stream << "     ttx[ns]     " << separator << "Chirp" << separator << "Pack " <<
            separator << "#Pack" << separator << "Byte/Packet" << separator <<
            "ttx_first[ns]";
    return stream.str();
}

std::string TBUSFixedPayload::toStringReceived(string separator) const {
    std::stringstream stream;
    stream << sendtimeNs_ << separator << receivetimeNs_ << separator << setw(5) << chirpnr_ << separator << setw(5) << packetnr_ <<
            separator << setw(5) << chirplength_ << separator << setw(5) << packetlength_ << separator << sendtimeFirstNs_;
    return stream.str();
}

std::string TBUSFixedPayload::toStringSend(string separator) const {
    std::stringstream stream;
    stream << sendtimeNs_ << separator << setw(5) << chirpnr_ << separator << setw(5) << packetnr_ <<
            separator << setw(5) << chirplength_ << separator << setw(5) << packetlength_ << separator << sendtimeFirstNs_;
    return stream.str();
}
