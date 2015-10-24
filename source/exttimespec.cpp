#include "exttimespec.h"

// External logging framework
#include "pantheios/pantheiosHeader.h"

#include <sstream>
#include <iomanip>      // std::setw std::setfill

double exttimespec::getTimeDouble() const{
    return this->tv_sec + this->tv_nsec / 1000000000.0;
}

/**
 * constructor creates a timespec struct
 * it forces tv_nsec to a range of [0..999999999] by changing the signum if negativ and mod BILLION
 * overflows will no be handed to sec.
 * sec is allowd to be negative though!
 * @param sec
 * @param nsec
 */
exttimespec::exttimespec(long sec, long nsec) {
    tv_nsec = nsec > 0 ? nsec % BILLION : -nsec % BILLION;
    tv_sec = sec > 0 ? sec : -sec;
}

//exttimespec::exttimespec(double timeAsDouble) {
//    double del_sec;
//    double del_nsec = modf(timeAsDouble, &del_sec);
//    tv_sec = del_sec;
//    tv_nsec = del_nsec;
//}

exttimespec exttimespec::operator+(long long nsec) const {
    return exttimespec(this->tv_sec + ((this->tv_nsec + nsec) / BILLION), (this->tv_nsec + nsec)%BILLION);
}

exttimespec exttimespec::operator-(long long nsec) const {
    return exttimespec(this->tv_sec + ((this->tv_nsec - nsec) / BILLION), (this->tv_nsec - nsec)%BILLION);
}

exttimespec exttimespec::operator+(const timespec & other) const {
    return exttimespec(this->tv_sec + other.tv_sec + ((this->tv_nsec + other.tv_nsec) / BILLION), (this->tv_nsec + other.tv_nsec)%BILLION);
}

exttimespec& exttimespec::operator+=(const timespec & other) {
     this->tv_sec += other.tv_sec + ((this->tv_nsec + other.tv_nsec) / BILLION);
     this->tv_nsec+= (this->tv_nsec + other.tv_nsec)%BILLION;
    return *this;
}

/**
 * warning, this function calculates the timedifference (always positive) bewtween the two 
 * exttimespec values this and other and returns the positive difference
 * @param other
 * @return 
 */
exttimespec exttimespec::operator-(const timespec & other) const {
    if (this->tv_sec > other.tv_sec || (this->tv_sec == other.tv_sec && tv_nsec > other.tv_nsec)) {
        if ((this->tv_nsec - other.tv_nsec) < 0) {
            return exttimespec(this->tv_sec - other.tv_sec - 1, 1000000000 + this->tv_nsec - other.tv_nsec);
        } else {
            return exttimespec(this->tv_sec - other.tv_sec, this->tv_nsec - other.tv_nsec);
        }
    } else {
        pantheios::log_ERROR("exttimespec.cpp: Always substract smaller from larger timespec! ", toString(), " - ", ((exttimespec) other).toString());
        if ((this->tv_nsec - other.tv_nsec) > 0) {
            return exttimespec(other.tv_sec - this->tv_sec - 1, 1000000000 + other.tv_nsec - this->tv_nsec);
        } else {

            return exttimespec(other.tv_sec - this->tv_sec, other.tv_nsec - this->tv_nsec);
        }
    }
}

exttimespec exttimespec::diffabs(const timespec &  other) const {
        if (this->tv_sec > other.tv_sec || (this->tv_sec == other.tv_sec && tv_nsec > other.tv_nsec)) {
        if ((this->tv_nsec - other.tv_nsec) < 0) {
            return exttimespec(this->tv_sec - other.tv_sec - 1, 1000000000 + this->tv_nsec - other.tv_nsec);
        } else {
            return exttimespec(this->tv_sec - other.tv_sec, this->tv_nsec - other.tv_nsec);
        }
    } else {
        if ((this->tv_nsec - other.tv_nsec) > 0) {
            return exttimespec(other.tv_sec - this->tv_sec - 1, 1000000000 + other.tv_nsec - this->tv_nsec);
        } else {

            return exttimespec(other.tv_sec - this->tv_sec, other.tv_nsec - this->tv_nsec);
        }
    }
}

bool exttimespec::operator<(const timespec & other) const {
    return this->tv_sec < other.tv_sec || (this->tv_sec == other.tv_sec && this->tv_nsec < other.tv_nsec);
}

bool exttimespec::operator>(const timespec & other) const {
    return this->tv_sec > other.tv_sec || (this->tv_sec == other.tv_sec && tv_nsec > other.tv_nsec);
}

bool exttimespec::operator<=(const timespec & other) const {
    return !(*this > other);
}

bool exttimespec::operator>=(const timespec & other) const {
    return !(*this < other);
}

bool exttimespec::operator==(const timespec & a) const {
    return (this->tv_sec == a.tv_sec && this->tv_nsec == a.tv_nsec);
}

bool exttimespec::operator!=(const timespec & a) const {
    return !(this->tv_sec == a.tv_sec && this->tv_nsec == a.tv_nsec);
}

/**
 * just some testing
 */
void exttimespec::testing() {
    exttimespec test1(2, 2);
    std::cout << test1 << "\n";

    exttimespec test2(1, 1);
    std::cout << test2 << "\n";

    //    exttimespec test3(1,3);
    //    cout << test3 << "\n"; 
    //    
    //    exttimespec test4(-1,-1);
    //    cout << test4 << (test4==exttimespec(-1,1)) << "\n"; 
    //    
    //    exttimespec test5(1,-1);
    //    cout << test5 << (test5==exttimespec(1,1))<<"\n"; 
    //    
    //    exttimespec test7(1,-1000000001);
    //    cout << test7 << (test7==exttimespec(1,1))<<"\n"; 
    //    
    //    exttimespec test6(-3,20000000000L);
    //    cout << test6 << (test6==exttimespec(-3,0))<<"\n"; 
    //    cout << test6 << "\n"; 
    std::cout << test1 << " - " << test2 << " = " << (test1 - test2) << "\n";
    std::cout << test2 << " - " << test1 << " = " << (test2 - test1) << "\n";
    std::cout << test2 << " + " << test1 << " = " << (test2 + test1) << "\n";
    std::cout << exttimespec(1, 999999999) << " + " << exttimespec(2, 3) << " = " << (exttimespec(1, 999999999) + exttimespec(2, 3)) << "\n";
    std::cout << (test1 == test2) << "=0\n";
    std::cout << (test1 == test1) << "=1\n";
    std::cout << (test1 <= test1) << "=1\n";
    std::cout << (test1 <= test2) << "=0\n";
    std::cout << (test2 <= test1) << "=1\n";
    std::cout << (test1 > test2) << "=1\n";
    std::cout << (test2 > test1) << "=0\n";
    // << exttimespec(2,2)-exttimespec(1,1) << "\n";
    exit(0);
}

std::string exttimespec::toString() const {
    std::stringstream stream;
    stream << this->tv_sec << "." << std::setw(9) << std::setfill('0') << this->tv_nsec << " s";
    return stream.str();
}

//this function needs to be global!

std::ostream& operator<<(std::ostream& os, const exttimespec & time) {
    return os << time.toString();
}