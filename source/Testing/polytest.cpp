/* 
 * File:   polytest.cpp
 * Author: goebel
 *
 * Created on 5. April 2015, 01:30
 */

#include <cstdlib>
#include <iostream>
#include <vector>
#include "circularQueue.h"
#include "../packetPayloads/TBUSFixedPayload.h"
#include "../packetPayloads/fixedPayload.h"
#include "../timeHandler.h"
// External logging framework
//#include "../pantheios/pantheiosHeader.h"
// External logging framework
//int pantheois_logLevel = 3;
//#include "../pantheios/pantheiosHeader.h"
//#include "../pantheios/pantheiosFrontEnd.h"
//#include "../circularQueue.h"

using namespace std;

class Base {
public:
    std::string classname;

    Base() : classname("BASE") {
    };

    virtual std::string fetchClassName() {
        return classname;
    }

    virtual std::string fetchDerivedClassName(Base & base) {
        return base.fetchClassName();
    }

    //    virtual Base& operator=(const Base &source) {
    //        // do the copy
    //        classname = source.classname;
    //
    //        // return the existing object
    //        return *this;
    //    }
};

class Derived : public Base {
public:

    Derived() {
        classname = "DERIVED";
    };

    virtual std::string fetchClassName() {
        return classname;
    }
};

void printClassname(Base base) {
    std::cout << base.fetchClassName() << "\n";
    Base tmp = base;
    std::cout << tmp.fetchClassName() << "\n";
//    std::vector<Base> values;
//    values.at(0) = tmp;
//    std::cout << "Vector<Base>" << values.at(0).fetchClassName() << "\n";
    //    circularQueue<Base> cq;
    //    cq.init(10000);
    //    //cq.resize(2);
    //    cq.push(base);
    //    cq.pop(&tmp);
    //    std::cout << "Circular<Base>" << tmp.fetchClassName() << "\n";
}

int main (int argc, char *argv[]) {
//    switch (pantheois_logLevel) {
//        case 0: // Normal with logfile
//            pantheois_logLevel = pantheios::SEV_NOTICE;
//            pantheios::log_NOTICE("MAIN| --------------------");
//            pantheios::log_NOTICE("MAIN| - rmf initializing -");
//            pantheios::log_NOTICE("MAIN| --------------------");
//            pantheios::log_NOTICE("MAIN| LogLevel: Normal");
//            pantheios::log_NOTICE("MAIN| ");
//            break;
//
//        case 1: // Extended with logfile
//            pantheois_logLevel = pantheios::SEV_INFORMATIONAL;
//            pantheios::log_NOTICE("MAIN| --------------------");
//            pantheios::log_NOTICE("MAIN| - rmf initializing -");
//            pantheios::log_NOTICE("MAIN| --------------------");
//            pantheios::log_NOTICE("MAIN| LogLevel: Extended");
//            pantheios::log_NOTICE("MAIN| ");
//            break;
//
//        case 2: // Debug with logfile
//            pantheois_logLevel = pantheios::SEV_DEBUG;
//            pantheios::log_NOTICE("MAIN| ---------------------");
//            pantheios::log_NOTICE("MAIN| - rmf initializing -");
//            pantheios::log_NOTICE("MAIN| ---------------------");
//            pantheios::log_NOTICE("MAIN| LogLevel: Debug");
//            pantheios::log_NOTICE("MAIN| ");
//            pantheios::log_NOTICE("MAIN| 	WARINING: Not meant for measurement !");
//            pantheios::log_NOTICE("MAIN| ");
//            pantheios::log_NOTICE("MAIN| ");
//            break;
//
//    }


    Base baseObj;
    Derived derivedObj;
    Base &obj_a = derivedObj;
    std::cout << obj_a.fetchClassName() << "\n";
    std::cout << baseObj.fetchDerivedClassName(derivedObj) << "\n";
    printClassname(baseObj);
    printClassname(derivedObj);
    
    fixedPayload fixPay{1L,2LL,3LL };
    TBUSFixedPayload tbusFixPay{4LL,5LL,6L,7L,8L,9L};
    
    
    circularQueue<fixedPayload> cq{"BASE"};
    cq.resize(1000);
    cq.push(tbusFixPay);
    TBUSFixedPayload tralala;
    cq.pop(tralala);
    std::cout << "Tralala: " << tralala.toStringSend(',');
}

