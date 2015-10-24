/* timeHandler.h */

#ifndef TIMEHANDLER_H_
#define TIMEHANDLER_H_

using namespace std;

#include <stdint.h>
#include <time.h>
//#include "exttimespec.h"
#include <utility>      // std::pair, std::make_pair
#include <string.h>
#include "rmfMakros.h"
#include <iostream> //ostream
#include <stdlib.h>

class timeHandler {
private:
    struct timespec lastProbedTime;
    int64_t lastProbedTimeNs;
    int timingMethod_;
    int64_t startTimeNs;
    int64_t timeDifNs;

public:
    timeHandler(int timingMethod = 0);
    //    static timeHandler& getInstance() {
    //        static timeHandler instance; //Guaranteed to be destroyed
    //        //instantiated on first use
    //        return instance;
    //    }

    //    timeHandler(timeHandler const&); //don't implement
    //    void operator=(timeHandler const&); //don't implemt

    ~timeHandler();

    void probeTime();

    timespec getLastProbedTime() {
        return lastProbedTime;
    };

    int64_t getLastProbedTimeNs() {
        return lastProbedTimeNs;
    };
    timespec getCurrentTime();
    int64_t getCurrentTimeNs();
    int setTimingMethod(uint32_t);

    int sleepUntil(const timespec& until);
    int sleepUntil(int64_t until);
    int sleepFor(int64_t nsec);
    void start();
    int64_t stop();
    int64_t getstopped();
    
    
    static double getTimeAsDouble(uint32_t sec, uint32_t nsec);
    static void addTimespecs(timespec& result, timespec one, timespec two);
    static void diffTimespecs(timespec& result, timespec end, timespec start);
    static std::string timespecToString(const timespec& time);

    static int64_t timespec_to_ns(const timespec t);
    static int64_t timespec_to_ns(uint32_t,uint32_t);

    static uint32_t get_sec_from_ns(int64_t nsec){return (uint32_t) nsec/BILLION;}; 
    static uint32_t get_nsec_from_ns(int64_t nsec){return (uint32_t) nsec%BILLION;}; 
    
    static std::pair<int32_t, int32_t> ns_to_sec_nsec(int64_t ns) {
        return std::make_pair(ns / BILLION, ns % BILLION);
    };

    static timespec ns_to_timespec(int64_t ns) {
        timespec tmp;
        tmp.tv_sec = ns / BILLION;
        tmp.tv_nsec = ns % BILLION;
        return tmp;
    };
};

/*
 *  
 * The following operators by intention are not members of the class but public
 * 
 */

timespec operator+(const timespec & lhs, const timespec & rhs);

timespec operator+(const timespec & lhs, const long & nsec);


#endif /* TIMEHANDLER_H_ */
