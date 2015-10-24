/* networkThreadStruct.h */
#ifndef _networkThreadStruct_
#define _networkThreadStruct_

// Defines
#define MAX_CONTROL_CHANNEL_MESSAGE_LENGTH 		3000	// Payloadsize of an TCP Control Message
#define MAX_MEASUREMENT_CHANNEL_MESSAGE_LENGTH 	5000	// Payloadsize of an UDP Measure Packet, max 65535
#define MAX_SAFEGUARD_CHANNEL_MESSAGE_LENGTH 	 500	// Payloadsize of an UDP Safeguard Packet, max 65535
#define MAX_CHIRP_LENGTH 						1000	// maximum Packets per chirp, max 65535

// Includes
#include <netdb.h>
#include "rmfMakros.h"
#include "packetPayloads/TBUSDynamicPayload.h"
#include "packetPayloads/TBUSFixedPayload.h"
#include "packetPayloads/fixedPayload.h"

#include <string>

struct udp_measure_sendQueue {
    uint32_t id;
    int64_t sendtimeNs;
    uint32_t fixed_packet_length;
    uint32_t fixed_payload_length;
    char* fixed_payload;

    void setSendtimeNs(const uint32_t sec, const uint32_t nsec);

    std::string toStringSend(char sep) {
        return "";
    };
};

struct payloadStruct {
    TBUSFixedPayload fixPay;
    TBUSDynamicPayload dynPay;

    std::string toStringSend(string sep = ",") {
        return fixPay.toStringSend(sep) + sep + dynPay.toStringSend(sep);
    };
    
     std::string toStringReceived(string sep = ",") {
        return fixPay.toStringReceived(sep) + sep + dynPay.toStringReceived(sep);
    };

    std::string getStringHeaderSend(string sep = ",") {
        return fixPay.getStringHeaderSend(sep) + sep + dynPay.getStringHeaderSend(sep);
    };
    
    std::string getStringHeaderReceived(string sep = ",") {
        return fixPay.getStringHeaderReceived(sep) + sep + dynPay.getStringHeaderReceived(sep);
    };
};

struct udp_measure_receiveQueue {
    uint32_t id;
    int64_t sendtimeNs;
    uint32_t fixed_payload_length;
    uint32_t dynamic_payload_length;
    int64_t receivetimeNs;
    uint32_t complete_payload_length; // size without the packet header
    char* complete_payload; // payload without the packet header

    udp_measure_receiveQueue() {
    };
    udp_measure_receiveQueue(int readChars, char* buf, int64_t receiveTimeNs);

    void setSendtimeNs(uint32_t sec, uint32_t nsec);
    void setReceivetimeNs(uint32_t sec, uint32_t nsec);
};

struct udp_safeguard_sendQueue {
    uint32_t id;
    uint32_t based_on_id;
    // Send time mit rein ?
    uint32_t payload_length;
    char* payload;

    std::string toStringSend(string sep = ",") {
        return "";
    };
};

struct udp_safeguard_receiveQueue {
    uint32_t id;
    uint32_t based_on_id;
    // Send time mit rein ?
    int64_t receivetimeNs;
    uint32_t payload_length;
    char* payload;

    std::string toStringSend(string sep = ",") {
        return "";
    };

    void setReceivetimeNs(uint32_t sec, uint32_t nsec);
};

#endif
