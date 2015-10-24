/* timeHandler.cpp */

#include "timeHandler.h"
// External logging framework

#include "pantheios/pantheiosHeader.h"

#include "rmfMakros.h" //used to find if 32 or 64 bit
#include <stdio.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <iomanip>      // std::setw
#include <sstream>

using namespace std;

timeHandler::timeHandler(int method)
: timingMethod_(method) {
    lastProbedTime.tv_sec = 0;
    lastProbedTime.tv_nsec = 0;
    //timeHandler::timingMethod_ = method;
}

timeHandler::~timeHandler() {

}

/**
 * Set the lastProbedTime struct to the current time
 */
void timeHandler::probeTime() {
    lastProbedTime = getCurrentTime();
    lastProbedTimeNs = timeHandler::timespec_to_ns(lastProbedTime);
}

/**
 * Set the lastProbedTime struct to the current time
 */
timespec timeHandler::getCurrentTime() {
    struct timespec tspec;
    if (timeHandler::timingMethod_ == 0) // HPET ~ 1 nanoseconds
    {
        // https://www.kernel.org/doc/man-pages/online/pages/man2/clock_getres.2.html
        clock_gettime(CLOCK_REALTIME, &tspec); // MONOTONIC Clock only goes up, Realtime Clock can run faster or go back in time for some seconds (f.e. because of NTP/Kernel)
    } else if (timeHandler::timingMethod_ == 1) // getTimeoftheDay ~ mircoseconds
    {
        // https://www.kernel.org/doc/man-pages/online/pages/man2/settimeofday.2.html
        timeval timevalbuffer;
        if (gettimeofday(&timevalbuffer, NULL) != -1) {
            tspec.tv_sec = (uint32_t) timevalbuffer.tv_sec;
            tspec.tv_nsec = (uint32_t) timevalbuffer.tv_usec * 1000;
        }
    } else if (timeHandler::timingMethod_ == -1) // Timing method not set exit
    {
        pantheios::log_ERROR("Timing Method not set yet.");
        exit(-1);
    }
    return tspec;
}

int64_t timeHandler::getCurrentTimeNs() {
    return timeHandler::timespec_to_ns(timeHandler::getCurrentTime());
}

/**
 * Set method for getting the time for this this instance
 *
 * @param method
 *
 * @return 0
 */
int timeHandler::setTimingMethod(uint32_t method) {//pantheios::log_DEBUG("Entering setTimingMethod()");

    switch (method) {
        case 0: timeHandler::timingMethod_ = 0;
            break; //pantheios::log_DEBUG("-> Timing Method is HPET."); break;
        case 1: timeHandler::timingMethod_ = 1;
            break; //pantheios::log_DEBUG("-> Timing Method is getTimeofDay."); break;
        case 2: timeHandler::timingMethod_ = 2;
            break; //pantheios::log_DEBUG("-> Timing Method is OWN (MUST BE IMPLEMENTET !)."); break;

        default: // auto detect
            //pantheios::log_DEBUG("-> Timing Method will be auto detected.");

            int flagHPET = 0; // 0 = no, 1 = yes

            // Check for Kernelversion, must be 2.6.21 or greater
            struct utsname utsbuffer;
            string tmpBuffer;

            uint32_t x = 0;
            uint32_t major = 0;
            uint32_t minor = 0;
            uint32_t patch = 0;

            // Get Kernel Information, this is nearly like bash "uname -r"
            if (uname(&utsbuffer) == -1) {
                /* error */
            }

            tmpBuffer = (string) utsbuffer.release;

            x = tmpBuffer.find(".");
            if (x != string::npos) {
                major = (uint32_t) atoi(tmpBuffer.substr(0, x).c_str());

                if (major == 2) {
                    tmpBuffer.erase(0, x + 1);
                    x = tmpBuffer.find(".");

                    if (x != string::npos) {
                        minor = (uint32_t) atoi(tmpBuffer.substr(0, x).c_str());
                        if (minor == 6) {
                            tmpBuffer.erase(0, x + 1);
                            patch = (uint32_t) atoi(tmpBuffer.substr(0, tmpBuffer.length()).c_str());

                            if (patch >= 21) {
                                flagHPET = 1;
                            }
                        } else if (minor > 6) {
                            flagHPET = 1;
                        }
                    }
                } else if (major > 2) {
                    flagHPET = 1;
                }
            }

            // Kernelversion supports HPET, check if its enabled
            if (flagHPET == 1) {
                FILE *consoleFeedback;
                char buffer[256];

                consoleFeedback = popen("cat /proc/timer_list | grep hpet", "r");

                if (consoleFeedback != NULL) {
                    if (fgets(buffer, 256, consoleFeedback) != NULL) {
                        string console = (string) buffer;

                        // If we find the string we can enable the hpet timing method
                        if (console.find("hpet") != string::npos) {
                            this->timeHandler::timingMethod_ = 0; // HPET
                            //pantheios::log_DEBUG("  -> HPET");
                        } else { //pantheios::log_DEBUG("  -> getTimeofDay.");
                            this->timeHandler::timingMethod_ = 1;
                        } // getTimeoftheDay
                    }

                    pclose(consoleFeedback);
                }
            } else {
                this->timeHandler::timingMethod_ = 1; // getTimeoftheDay
                //pantheios::log_DEBUG("  -> getTimeofDay.");
            }

            break;
    }

    //pantheios::log_DEBUG("Leaving setTimingMethod()");
    //pantheios::log_DEBUG("");
    return (0);
}

/**
 * Sleep until the absolute given input time
 *
 * @param until = the time in ns from epoch until we want to sleep
 * @return
 */
int timeHandler::sleepUntil(int64_t until) {
    return timeHandler::sleepUntil(ns_to_timespec(until));
}

/**
 * Sleep until the absolute given input time
 *
 * @param seconds
 * @param nanoseconds
 * @return
 */
int timeHandler::sleepUntil(const timespec &until) {
    // clock nanosleep can be interrupted with signals
    // this should not happen
    // see clock_nanosleep mapage
    int ret = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &until, NULL);
    if (ret != 0) {
        pantheios::log_NOTICE("TIME| clock_nanosleep error! Values: ", timeHandler::timespecToString(until),
                "; Error: ", pantheios::integer(ret));
        exit(1);
    }
    return (0);
}

/**
 * Sleep until the absolute given input time
 *
 * @param seconds
 * @param nanoseconds
 * @return
 */
int timeHandler::sleepFor(int64_t nsec) {
    return timeHandler::sleepUntil(timeHandler::getCurrentTime() + nsec);
}

/**
 * Return the time in seconds (including nsec) as a double
 * @param sec
 * @param nsec
 * @return time in seconds (including nsec) as a double
 */
double timeHandler::getTimeAsDouble(uint32_t sec, uint32_t nsec) {
    return sec + nsec / 1000000000.0;
}

int64_t timeHandler::timespec_to_ns(const timespec t) {
    return t.tv_sec * BILLION + t.tv_nsec;
};

int64_t timeHandler::timespec_to_ns(uint32_t sec, uint32_t nsec) {
    return sec * BILLION + nsec;
};

/**
 * add the timespec structs one and two and return it in result
 * @param result = one + two
 * @param one
 * @param two
 */
void timeHandler::addTimespecs(timespec& result, const timespec one, const timespec two) {
    //    result.tv_sec = one.tv_sec + two.tv_sec;
    //    result.tv_nsec = one.tv_nsec + two.tv_nsec; 
    //
    //    // overflow ?
    //    if (result.tv_nsec > 999999999) {
    //        result.tv_nsec -= 1000000000;
    //        ++result.tv_sec;
    //    } else if (result.tv_nsec < 0) {
    //        result.tv_nsec += 1000000000;
    //        --result.tv_sec;
    //    }
    result = one + two;
}

/**
 * calculate the difference of timespec stats end-start
 * @param result = end - start
 * @param start
 * @param end
 */
void timeHandler::diffTimespecs(timespec& result, timespec end, timespec start) {
    if ((end.tv_nsec - start.tv_nsec) < 0) {
        result.tv_sec = end.tv_sec - start.tv_sec - 1;
        result.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    } else {
        result.tv_sec = end.tv_sec - start.tv_sec;
        result.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
};

std::string timeHandler::timespecToString(const timespec& time) {
    std::stringstream stream;
    stream << time.tv_sec << "." << std::setw(9) << std::setfill('0') << time.tv_nsec << " s";
    return stream.str();
};

void timeHandler::start() {
    this->startTimeNs = getCurrentTimeNs();
}

int64_t timeHandler::stop() {
    timeDifNs = getCurrentTimeNs() - startTimeNs;
    return timeDifNs;
}

int64_t timeHandler::getstopped() {
    return timeDifNs;
}

/*
 *  
 * The following operators by intention are not members of the class but public
 * 
 */

timespec operator+(const timespec & lhs, const timespec & rhs) {
#ifdef IS64BIT
    timespec tmp = {lhs.tv_sec + rhs.tv_sec + ((lhs.tv_nsec + rhs.tv_nsec) / BILLION), (lhs.tv_nsec + rhs.tv_nsec) % BILLION};
#else
    timespec tmp = {(long) (lhs.tv_sec + rhs.tv_sec + ((lhs.tv_nsec + rhs.tv_nsec) / BILLION)), (long) ((lhs.tv_nsec + rhs.tv_nsec) % BILLION)};
#endif
    return tmp;
};

timespec operator+(const timespec & lhs, const long & nsec) {
#ifdef IS64BIT
    timespec tmp = {lhs.tv_sec + ((lhs.tv_nsec + nsec) / BILLION), (lhs.tv_nsec + nsec) % BILLION};
#else
    timespec tmp = {(long) (lhs.tv_sec + ((lhs.tv_nsec + nsec) / BILLION)), (long) ((lhs.tv_nsec + nsec) % BILLION)};
#endif
    return tmp;
};


