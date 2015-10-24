/* networkThreadStructs.cpp */
#include "networkThreadStructs.h"
#include <string.h> //memcpy
#include <stdlib.h> //malloc
#include "rmfMakros.h"
#include <netdb.h> //ntohl
#include "packetPayloads/packetPayload.h"

void udp_measure_sendQueue::setSendtimeNs(const uint32_t sec, const uint32_t nsec) {
    sendtimeNs = sec * 1000000000LL + nsec;
};

//void udp_measure_sentQueue::setSendtimeNs(uint32_t sec, uint32_t nsec) {
//    sendtimeNs = sec * BILLION + nsec;
//};

void udp_measure_receiveQueue::setSendtimeNs(uint32_t sec, uint32_t nsec) {
    sendtimeNs = sec * BILLION + nsec;
};

void udp_measure_receiveQueue::setReceivetimeNs(uint32_t sec, uint32_t nsec) {
    receivetimeNs = sec * BILLION + nsec;
};

    void udp_safeguard_receiveQueue::setReceivetimeNs(uint32_t sec, uint32_t nsec) {
        receivetimeNs = sec * BILLION + nsec;
    };

udp_measure_receiveQueue::udp_measure_receiveQueue(int readChars, char* buf, int64_t receiveTimeNs) {

    static int sizeof_packet_header = 5 * sizeof (uint32_t);
    static int payload_size = readChars - sizeof_packet_header;

    this->complete_payload = (char *) malloc(payload_size);
    id = GET_NETWORK_UINT32(buf, 0);
    setSendtimeNs(GET_NETWORK_UINT32(buf, 1), GET_NETWORK_UINT32(buf, 2));
    fixed_payload_length = GET_NETWORK_UINT32(buf, 3);
    dynamic_payload_length = GET_NETWORK_UINT32(buf, 4);

    memcpy(this->complete_payload, &buf[sizeof_packet_header], payload_size);
    //        
    //        memcpy(&net_id, &buffer[ 0], sizeof_uint32_t);
    //        memcpy(&net_send_seconds, &buffer[ sizeof_uint32_t], sizeof_uint32_t);
    //        memcpy(&net_send_nanoseconds, &buffer[ 2 * sizeof_uint32_t], sizeof_uint32_t);
    //        memcpy(&net_fixed_payload_length, &buffer[ 3 * sizeof_uint32_t], sizeof_uint32_t);
    //        memcpy(&net_dynamic_payload_length, &buffer[ 4 * sizeof_uint32_t], sizeof_uint32_t);
    //        memcpy(received.complete_payload, &buffer[sizeof_packet_header], payload_size);
    //        received.id = ntohl(net_id);
    //        received.setSendtimeNs(ntohl(net_send_seconds), ntohl(net_send_nanoseconds));
    //        received.fixed_payload_length = ntohl(net_fixed_payload_length);
    //        received.dynamic_payload_length = ntohl(net_dynamic_payload_length);
    //        received.receivetimeNs = timing.getLastProbedTimeNs();
    //        received.complete_payload_length = payload_size;
};