#include "measurementMethodClass.h"

measurementMethodClass::~measurementMethodClass() {
    ;
};

int measurementMethodClass::dataFileWriterMeasureSender(const payloadStruct&) {
    return 0;
}

int measurementMethodClass::getMaxFixedPayloadSize() { // in bytes
    return 1444;
}

int measurementMethodClass::getReceiveQueueSize() { // in bytes
    return 1024 * 1024 * 10;
}

int measurementMethodClass::getSentQueueSize() { // in bytes
    return 1024 * 1024 * 10;
}

int measurementMethodClass::getSendingQueueSize() { // in bytes
    return 1024 * 1024 * 10;
}

int measurementMethodClass::getSafeguardSendingQueueSize() { // in bytes
    return 1024 * 1024 * 10;
}

int measurementMethodClass::getSafeguardReceivingQueueSize() { // in bytes
    return 1024 * 1024 * 10;
}
