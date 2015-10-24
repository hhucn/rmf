/**
 * @authors Franz Kary, Tobias Krauthoff
 * @date 23. June 2014
 * circular queue for queues with: udp_measure_sendQueue, udp_measure_sentQueue,
 * udp_measure_receiveQueue, udp_safeguard_sendQueue, udp_safeguard_receiveQueue
 */

#ifndef _CIRCULARQUEUE_H_
#define _CIRCULARQUEUE_H_

#define DEBUG_CQ (false)

#include <sys/select.h>
#include <iostream>
#include <vector>
#include "pantheios/pantheiosHeader.h"
#include <stdint.h>
//#include <functional> //std::reference_wrapper
#include <vector>
#include <string>
#include "packetPayloads/TBUSDynamicPayload.h"
#include "packetPayloads/TBUSFixedPayload.h"
#include "packetPayloads/fixedPayload.h"
#include "packetPayloads/packetPayload.h"
#include "networkThreadStructs.h"

template<typename T>
class circularQueue {
private:
    std::vector<T> *values;
    std::string name;
    uint32_t maxElements;
    uint32_t indexOfWriteElement;
    uint32_t indexOfReadElement;
    bool writeParity;
    bool readParity;
//    packetPayload* payloadClass_;
public:

    circularQueue(std::string Name) {
        //        pantheios::log_DEBUG("CICQ| Default constructor with zero memory");
        //uint32_t memTmp = 1250000 * 2; // 10 mbit/s Queuelength = 2 * Bandwidth
        //memory = 12500000*2; //100 mbit/s
        //memory = 125000000*2; //1 gbit/s 
        uint32_t memTmp = 0; // 10 mbit/s Queuelength = 2 * Bandwidth
        name = Name;
        init(memTmp);
//        payloadClass_ = NULL;
    }

    circularQueue(uint32_t setMemorySize, std::string Name) {
        //        pantheios::log_DEBUG("CICQ| Constructor with ", pantheios::integer(setMemorySize), " memory");
        name = Name;
        init(setMemorySize);
//        payloadClass_ = NULL;
    }

    ~circularQueue() {
    }

    void deleteQueue() {
        delete values;
    }

    void setName(std::string Name) {
        name = Name;
    }

    void init(uint32_t memorySize) {
        pantheios::log_DEBUG("CICQ| ", name, " Begin init()");
        uint32_t tempsize = sizeof (T);
        maxElements = (uint32_t) (memorySize / tempsize); 
        pantheios::log_NOTICE("CICQ| ", name, " resized to ",pantheios::integer(maxElements)," elements of ",pantheios::integer(tempsize),
                " Bytes consuming about ",pantheios::integer(maxElements* tempsize/1000000L)," MB");
        values = new std::vector<T>(maxElements);
        //resize(memorySize);
        indexOfReadElement = 0;
        indexOfWriteElement = 0;
        writeParity = true;
        readParity = false;
        pantheios::log_DEBUG("CICQ| ", name, " init() maxElements=", pantheios::integer(maxElements));
        pantheios::log_DEBUG("CICQ| ", name, " Leave init()");
        //std::cout << "memory size init: " << memorySize << "\tsizeof(T)" << sizeof(T) << "\t\tmax Elements: " << values->size() <<  " " << values << endl;
    }

    void resize(uint32_t memorySize) {
        pantheios::log_DEBUG("CICQ| ", name, " Begin resize()");
        uint32_t tempsize = sizeof (T);
        maxElements = (uint32_t) (memorySize / tempsize);
        pantheios::log_NOTICE("CICQ| ", name, " resized to ",pantheios::integer(maxElements)," elements of ",pantheios::integer(tempsize),
                " Bytes consuming about ",pantheios::integer(maxElements* tempsize/1000000L)," MB");
        values->resize(maxElements);
        pantheios::log_DEBUG("CICQ| ", name, " resize() maxElements=", pantheios::integer(maxElements));
        pantheios::log_DEBUG("CICQ| ", name, " Leave resize()");
    }

    uint32_t getMaxSize() {
        if (DEBUG_CQ) {
            pantheios::log_DEBUG("CICQ| ", name, " Begin getMaxSize()");
            pantheios::log_DEBUG("CICQ| ", name, " getMaxSize() =", pantheios::integer(maxElements));
            pantheios::log_DEBUG("CICQ| ", name, " Leave getMaxSize()");
        }
        return maxElements;
    }

    uint32_t size() {
        if (DEBUG_CQ)pantheios::log_DEBUG("CICQ| ", name, " Begin size()");
        if (writeParity == readParity) {
            if (DEBUG_CQ)pantheios::log_DEBUG("CICQ| ", name, " Leave size()");
            return (indexOfWriteElement + maxElements - indexOfReadElement);
        } else {
            if (DEBUG_CQ) pantheios::log_DEBUG("CICQ| ", name, " Leave size()");
            return (indexOfWriteElement - indexOfReadElement);
        }
    }

    bool empty() {
        if (DEBUG_CQ)pantheios::log_DEBUG("CICQ| ", name, " Begin empty()");
        //std::cout << readParity << writeParity << indexOfReadElement << indexOfWriteElement << std::endl;
        if (DEBUG_CQ)pantheios::log_DEBUG("CICQ| ", name, " Leave empty()");
        return ((indexOfReadElement == indexOfWriteElement) && (readParity != writeParity));
    }

    bool isFull() {
        if (DEBUG_CQ)pantheios::log_DEBUG("CICQ| ", name, " Begin full()");
        if (DEBUG_CQ)pantheios::log_DEBUG("CICQ| ", name, " Leave full()");
        return ((indexOfReadElement == indexOfWriteElement) && (readParity == writeParity));
    }

    int32_t push(T & element) {
        if (DEBUG_CQ) pantheios::log_DEBUG("CICQ| ", name, " Begin push() ", element.toStringSend());
        //@FIX TK maxElements is set to zero somewhere, so check it here
        if (maxElements == 0) maxElements = values->size();

        if (!isFull()) {
            //std::cout << "max Elements: " << values->size() <<  " " << values << endl;
            //values->insert(values->begin()[indexOfWriteElement], element);
            if (DEBUG_CQ)pantheios::log_DEBUG("CICQ| ", name, " push(): indexOfWriteElement=", pantheios::integer(indexOfWriteElement),
                    ", queueSize=", pantheios::integer(values->size()),
                    " (maxElements=", pantheios::integer(maxElements), ")");
            values->at(indexOfWriteElement) = element;
            //values[indexOfWriteElement] = element;
            if (indexOfWriteElement == maxElements - 1) {
                indexOfWriteElement = 0;
                writeParity = !writeParity;
            } else {
                indexOfWriteElement++;
            }
            //std::cout << "Füge Element hinzu: " << element << " Queuegröße: " << size() << std::endl;
            if (DEBUG_CQ)pantheios::log_DEBUG("CICQ| ", name, " Leave push()");
            return 0;
        } else {
            //std::cout << "Versuche Element hinzuzufügen, aber die Queue ist voll! Queuegröße: " << size() << std::endl;
            if (DEBUG_CQ)pantheios::log_DEBUG("CICQ| ", name, " Leave push()");
            return -1;
        }
    }

    int32_t waitingPush(T & element) {
        if (DEBUG_CQ)pantheios::log_DEBUG("CICQ| ", name, " Begin waitingPush()");
        bool tmp = false;
        if (DEBUG_CQ)pantheios::log_DEBUG("CICQ| ", name, " Leave waitingPush()");
        return waitingPushWithAbort(element, &tmp);
    }

    int32_t waitingPushWithAbort(T & element, bool *abort) {
        if (DEBUG_CQ) pantheios::log_DEBUG("CICQ| ", name, " Begin waitingPushWithAbort()");
        struct timeval t;
        t.tv_sec = 1L;
        t.tv_usec = 0L;
        do {
            if (*abort == true) {
                break;
            }
            if (!isFull()) {
                if (DEBUG_CQ) pantheios::log_DEBUG("CICQ| ", name, " Leave waitingPushWithAbort()");
                return push(element);
            }
        } while (select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &t) || true);
        if (DEBUG_CQ) pantheios::log_DEBUG("CICQ| ", name, " Leave waitingPushWithAbort()");
        return -1;
    }

    int32_t pop() {
        if (DEBUG_CQ)pantheios::log_DEBUG("CICQ| ", name, " Begin pop()");
        if (!empty()) {
            //	uint32_t oldPointer = indexOfReadElement;
            if (indexOfReadElement == maxElements - 1) {
                indexOfReadElement = 0;
                readParity = !readParity;
            } else {
                indexOfReadElement++;
            }
            //std::cout << "Pop Element aus: " << values[oldPointer] << " Queuegröße: " << size() << std::endl;
            if (DEBUG_CQ)pantheios::log_DEBUG("CICQ| ", name, " Leave pop()");
            return 0;
        } else {
            //std::cout << "Versuche Element auzu\"popen\", aber die Queue ist leer!" << " Queuegröße: " << size() << std::endl;
            if (DEBUG_CQ)pantheios::log_DEBUG("CICQ| ", name, " Leave pop()");
            return -1;
        }
    }

    int32_t pop(T *element) {
        if (DEBUG_CQ)pantheios::log_DEBUG("CICQ| ", name, " Begin pop(..)");
        if (!empty()) {
            uint32_t oldPointer = indexOfReadElement;
            if (indexOfReadElement == maxElements - 1) {
                indexOfReadElement = 0;
                readParity = !readParity;
            } else {
                indexOfReadElement++;
            }
            //std::cout << "Pop Element aus: " << values[oldPointer] << " Queuegröße: " << size() << std::endl;
//            if (payloadClass_ != NULL) {
            
                *element = values->at(oldPointer);
//            } else {
//                TBUSFixedPayload *ele = values->at(oldPointer);//dynamic_cast<TBUSFixedPayload>((values->at(oldPointer)));
                //*element = dynamic_cast<payloadClass_>(values->at(oldPointer));
//            }
            if (DEBUG_CQ) pantheios::log_DEBUG("CICQ| ", name, " Popped ", element->toStringSend());
            return 0;
        } else {
            //std::cout << "Versuche Element auzu\"popen\", aber die Queue ist leer!" << " Queuegröße: " << size() << std::endl;
            if (DEBUG_CQ)pantheios::log_DEBUG("CICQ| ", name, " Leave pop(..)");
            return -1;
        }
    }

    int32_t front(T *element) {
        if (DEBUG_CQ)pantheios::log_DEBUG("CICQ| ", name, " Begin front()");
        if (!empty()) {
            //std::cout << "Lese Element aus: " << values[indexOfReadElement] << " Queuegröße: " << size() << std::endl;
            *element = values->at(indexOfReadElement);
            if (DEBUG_CQ)pantheios::log_DEBUG("CICQ| ", name, " Leave front()");
            return 0;
        } else {
            //std::cout << "Versuche Element auszulesen, aber die Queue ist leer!" << " Queuegröße: " << size() << std::endl;
            if (DEBUG_CQ)pantheios::log_DEBUG("CICQ| ", name, " Leave front()");
            return -1;
        }
    }

    int32_t back(T *element) {
        if (DEBUG_CQ)pantheios::log_DEBUG("CICQ| ", name, " Begin back()");
        if (!empty()) {
            if (indexOfWriteElement == 0) {
                //std::cout << "Lese das zuletzt hinzugefügte Element aus: " << values[maxElements-1] << " Queuegröße: " << size() << std::endl;
                *element = values->at(maxElements - 1);
                if (DEBUG_CQ)pantheios::log_DEBUG("CICQ| ", name, " Leave back()");
                return 0;
            } else {
                //std::cout << "Lese das zuletzt hinzugefügte Element aus: " << values[indexOfWriteElement-1] << " Queuegröße: " << size() << std::endl;
                *element = values->at(indexOfWriteElement - 1);
                if (DEBUG_CQ)pantheios::log_DEBUG("CICQ| ", name, " Leave back()");
                return 0;
            }
        } else {
            //std::cout << "Versuche Element das zuletzt hinzugefügte Element auszulesen, aber die Queue ist leer!" << " Queuegröße: " << size() << std::endl;
            if (DEBUG_CQ)pantheios::log_DEBUG("CICQ| ", name, " Leave back()");
            return -1;
        }
    }

//    void setDynamicCastClass(packetPayload * payloadClass) {
//        payloadClass_ = payloadClass;
//    }
};

template class circularQueue<packetPayload>;
template class circularQueue<fixedPayload>;
template class circularQueue<TBUSFixedPayload>;
template class circularQueue<TBUSDynamicPayload>;
template class circularQueue<payloadStruct>;
template class circularQueue<udp_safeguard_sendQueue>;
template class circularQueue<udp_safeguard_receiveQueue>;
#endif

