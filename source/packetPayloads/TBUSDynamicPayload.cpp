/* 
 * File:   TBUSDynamicPayload.cpp
 * Author: goebel
 * 
 * Created on 2. April 2015, 17:33
 */

#include "TBUSDynamicPayload.h"
#include "../measurementMethods/TBUS/TBUS_FastDatarateCalculator.h"
#include "../timeHandler.h"
#include "packetPayload.h"
#include <iomanip>

TBUSDynamicPayload::TBUSDynamicPayload(const uint32_t basedOnChirpnr, const int32_t basedOnPacketnr, const uint64_t datarate,
        const uint32_t loss, const int64_t nextTtx, const uint32_t nextTtxBasedOnChirpnr, const uint32_t nextTtxBasedOnPacketnr,
        const uint32_t lastValidSafeguardChannelDelay)
: datarate_(datarate), nextTtx_(nextTtx), nextTtxBasedOnChirpnr_(nextTtxBasedOnChirpnr), nextTtxBasedOnPacketnr_(nextTtxBasedOnPacketnr),
basedOnChirpNr_(basedOnChirpnr), basedOnPacketNr_(basedOnPacketnr), loss_(loss),
lastValidSafeguardChannelDelay_(lastValidSafeguardChannelDelay) {
}

TBUSDynamicPayload::TBUSDynamicPayload(const TBUSDynamicPayload & dynPay, const uint32_t basedOnChirpnr, const uint32_t basedOnPacketnr,
        const uint64_t datarate, const uint32_t loss, const uint32_t lastValidSafeguardChannelDelay)
: datarate_(datarate), basedOnChirpNr_(basedOnChirpnr), basedOnPacketNr_(basedOnPacketnr), loss_(loss), lastValidSafeguardChannelDelay_(lastValidSafeguardChannelDelay) {
    nextTtx_ = dynPay.getNextTtx();
    nextTtxBasedOnChirpnr_ = dynPay.getNextTtxBasedOnChirpnr();
    nextTtxBasedOnPacketnr_ = dynPay.getNextTtxBasedOnPacketnr();
}

/**
 * create object from char buffer (which can be discarded after the object
 * it created)
 * this converts from network to host order
 * 
 * @param buf
 */
TBUSDynamicPayload::TBUSDynamicPayload(char * buf, int64_t receivetimeNs) {
    datarate_ = GET_NETWORK_UINT64(buf, 0); //warning, pos here is in 8 bytes!
    nextTtx_ = GET_NETWORK_UINT64(buf, 1); //warning, pos here is in 8 bytes!
    sendtimeNs_ = GET_NETWORK_UINT64(buf, 2); //warning, pos here is in 8 bytes!
    nextTtxBasedOnChirpnr_ = GET_NETWORK_UINT32(buf, 6); //pos is going in 4 byte steps!
    nextTtxBasedOnPacketnr_ = GET_NETWORK_UINT32(buf, 7); //pos is going in 4 byte steps!
    basedOnChirpNr_ = GET_NETWORK_UINT32(buf, 8); //pos is going in 4 byte steps!
    basedOnPacketNr_ = GET_NETWORK_UINT32(buf, 9); //pos is going in 4 byte steps!
    loss_ = GET_NETWORK_UINT32(buf, 10); //pos is going in 4 byte steps!
    lastValidSafeguardChannelDelay_ = GET_NETWORK_UINT32(buf, 11); //pos is going in 4 byte steps!
    receivetimeNs_ = receivetimeNs;
}

/**
 * Insert the payload into the given buffer (which hopefully is long enogh
 * Remember to implement the same conversion in the char*buf constructor!
 * 
 * @param buf
 * @return -1 if something went wrong, else it returns 0
 */
int TBUSDynamicPayload::insertPayloadInPacket(char* buf) const {
    SET_NETWORK_UINT64(datarate_, buf, 0);
    SET_NETWORK_UINT64(nextTtx_, buf, 1);
    SET_NETWORK_UINT64(sendtimeNs_, buf, 2);
    SET_NETWORK_UINT32(nextTtxBasedOnChirpnr_, buf, 6);
    SET_NETWORK_UINT32(nextTtxBasedOnPacketnr_, buf, 7);
    SET_NETWORK_UINT32(basedOnChirpNr_, buf, 8);
    SET_NETWORK_UINT32(basedOnPacketNr_, buf, 9);
    SET_NETWORK_UINT32(loss_, buf, 10);
    SET_NETWORK_UINT32(lastValidSafeguardChannelDelay_, buf, 11);
    return 0;
}

uint32_t TBUSDynamicPayload::getPayloadSize() const {
    return payloadsize_;
}

std::string TBUSDynamicPayload::getStringHeaderReceived(string separator) const {
    std::stringstream stream;
    stream << " ttx (dynPay)[ns] " << separator << " trx (dynPay)[ns] " << separator << "bChirp" <<
            separator << "bPack" << separator << "Rate[Byte/s]" << separator << " Loss% " <<
            separator << " SC_OWD[ns] " <<
            separator << "    nextTtx[ns]    " << separator << "bChirp" << separator << "bPacket";
    return stream.str();
}

std::string TBUSDynamicPayload::getStringHeaderSend(string separator) const {
    std::stringstream stream;
    stream << "ttx (dynPay)[ns] " << separator << "bChirp" <<
            separator << "bPack" << separator << "Rate[Byte/s]" << separator << " Loss% " <<
            separator << " SC_OWD[ns] " <<
            separator << "    nextTtx[ns]    " << separator << "bChirp" << separator << "bPacket";
    return stream.str();
}

std::string TBUSDynamicPayload::toStringReceived(string separator) const {
    std::stringstream stream;
    stream << setw(19) << sendtimeNs_ << separator << setw(19) << receivetimeNs_ << separator << setw(5) << basedOnChirpNr_ <<
            separator << setw(5) << basedOnPacketNr_ << separator << setw(11) << datarate_ << separator << setw(9) << loss_ * 100.0 / LOSSFACTOR <<
            separator << setw(12) << lastValidSafeguardChannelDelay_ <<
            separator << setw(19) << nextTtx_ << separator << setw(5) << nextTtxBasedOnChirpnr_ << separator << setw(5) << nextTtxBasedOnPacketnr_;
    return stream.str();
}

std::string TBUSDynamicPayload::toLogSmall() const {
    std::stringstream stream;
    stream << "Based on (" << setw(5) << basedOnChirpNr_ << ":" << setw(5) << basedOnPacketNr_ << ")  datarate = " << setw(11) << datarate_
            << "  Loss= " << loss_ * 100.0 / LOSSFACTOR;
    return stream.str();
}

std::string TBUSDynamicPayload::toStringSend(string separator) const {
    std::stringstream stream;
    stream << setw(19) << sendtimeNs_ << separator << setw(5) << basedOnChirpNr_ <<
            separator << setw(5) << basedOnPacketNr_ << separator << setw(11) << datarate_ << separator << setw(9) << loss_ * 100.0 / LOSSFACTOR <<
            separator << setw(12) << lastValidSafeguardChannelDelay_ <<
            separator << setw(19) << nextTtx_ << separator << setw(5) << nextTtxBasedOnChirpnr_ << separator << setw(5) << nextTtxBasedOnPacketnr_;
    return stream.str();
}

uint32_t TBUSDynamicPayload::getBasedOnChirpnr() const {
    return basedOnChirpNr_;
}

uint32_t TBUSDynamicPayload::getBasedOnPacketnr() const {
    return basedOnPacketNr_;
}

uint64_t TBUSDynamicPayload::getDatarate() const {
    return datarate_;
}

uint32_t TBUSDynamicPayload::getLoss() const {
    return loss_;
}

uint32_t TBUSDynamicPayload::getNextTtxBasedOnChirpnr() const {
    return nextTtxBasedOnChirpnr_;
}

uint32_t TBUSDynamicPayload::getNextTtxBasedOnPacketnr() const {
    return nextTtxBasedOnPacketnr_;
}

int64_t TBUSDynamicPayload::getNextTtx() const {
    return nextTtx_;
}

bool TBUSDynamicPayload::isNewerThan(const TBUSDynamicPayload& dynPay) const {
    if ((basedOnChirpNr_ > dynPay.basedOnChirpNr_) ||
            (basedOnChirpNr_ == dynPay.basedOnChirpNr_ && basedOnPacketNr_ > dynPay.basedOnPacketNr_) ||
            (nextTtxBasedOnChirpnr_ > dynPay.nextTtxBasedOnChirpnr_) ||
            (nextTtxBasedOnChirpnr_ == dynPay.nextTtxBasedOnChirpnr_ && nextTtxBasedOnPacketnr_ > dynPay.basedOnPacketNr_)) {
        return true;
    } else
        return false;
}

uint32_t TBUSDynamicPayload::getLastValidSafeguardChannelDelay_() const {
    return lastValidSafeguardChannelDelay_;
}

