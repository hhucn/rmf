/* 
 * File:   exttimespec.h
 * Author: goebel
 *
 * Created on 27. März 2015, 11:07
 */

#ifndef EXTTIMESPEC_H
#define	EXTTIMESPEC_H

#include <time.h>
#include <string>
#include <iostream> //ostream
#include <stdlib.h>
#include "rmfMakros.h"

#define BILLION 1000000000LL
struct exttimespec : public timespec {
    bool signum;
    exttimespec(long sec, long nsec);

    exttimespec(timespec time) {
        this->tv_sec = time.tv_nsec;
        this->tv_nsec = time.tv_sec;
    };

    exttimespec(double timeAsDouble) {
        long long int ns = abs( ((long long int) timeAsDouble * 1000000000LL));
        tv_sec = ns / BILLION;
        tv_nsec = ns - tv_sec * BILLION;
    };

    exttimespec() {
        this->tv_sec = 0;
        this->tv_nsec = 0;
    };

    double getTimeDouble() const;
    exttimespec operator+(long long nsec) const;
    exttimespec operator-(long long seconds) const;
    exttimespec operator+(const timespec &other) const;
    exttimespec& operator+=(const timespec &other);
    exttimespec operator-(const timespec &other) const;
    exttimespec diffabs(const timespec &  other) const;
    bool operator<(const timespec &other) const;
    bool operator>(const timespec &other) const;
    bool operator<=(const timespec &other) const;
    bool operator>=(const timespec &other) const;
    bool operator==(const timespec& a) const;
    bool operator!=(const timespec& a) const;
    static void testing();
    std::string toString() const;
};

//needs to be a global function
std::ostream& operator<<(std::ostream& os, const exttimespec& time);

#endif	/* EXTTIMESPEC_H */

