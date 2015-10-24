/* 
 * File:   rmfMakros.h
 * Author: goebel
 *
 * Created on 31. März 2015, 12:59
 */

#ifndef RMFMAKROS_H
#define	RMFMAKROS_H

//disable asserts if NDEBUG is defined prior to include assert.h
#define NDEBUG 

//define some macros to convert dynamic or fixed payload data to useful values
#define GET_PACKET_ID(fixedPayload) (ntohl(((uint32_t*) fixedPayload)[1]))
#define GET_CHIRP_LENGTH(fixedPayload) (ntohl(((uint32_t*) fixedPayload)[1]) >> 16)
#define GET_CHIRP_PACKET_SIZE(fixedPayload) (ntohl(((uint32_t*) fixedPayload)[1]) & 0xFFFF)
#define GET_RATE_INT(dynamicPayload) (ntohl(((uint32_t*) dynamicPayload)[1]))
#define GET_RATE_FRAC(dynamicPayload) (ntohl(((uint32_t*) dynamicPayload)[2]))
#define GET_RATE(dynamicPayload) ((GET_RATE_INT(dynamicPayload) + GET_RATE_FRAC(dynamicPayload) / 1000000000.0) / 1000.0)
#define GET_TIME_FIRST_SEC(fixedPayload) (ntohl(((uint32_t*) fixedPayload)[2]))
#define GET_TIME_FIRST_NSEC(fixedPayload) (ntohl(((uint32_t*) fixedPayload)[3]))
#define GET_TIME_DOUBLE(sec,nsec) (sec + nsec / 1000000000.0)
#define GET_RATE_DOUBLE(rint,rfrac) (rint + rfrac / 1000000000.0)

#define GET_CHIRP(id) (id >> 16)
#define GET_PACKET(id) (id & 0xFFFF)
#define LOG_ID(id) "ID=",integer(GET_CHIRP(id)),".",integer(GET_PACKET(id))
#define LOG_CHIRP_PACKET(c,p) integer(c),".",integer(p)
#define COMPARE(first,second,textOk,textWrong) {if (first == second) {cout << textOk<<" ";} else {cout << textWrong << first << " != "<< second << "\n";}}
// example: COMPARE(deltarec,(GET_TIME_DOUBLE(receivetime_seconds,receivetime_nanoseconds)-GET_TIME_DOUBLE(nReceiveFirstSec_,nReceiveFirstNsec_)),"TIME MISMATCH MACRO");

#define CONSOLE_LOG_THROTTLE 2000000000LL //log once very two second at most
//#define CONSOLE_LOG_THROTTLE 0LL //log once very two second at most
#define BILLION 1000000000LL

//#if ((ULONG_MAX) == (UINT_MAX))
//# define IS32BIT
//#else
//# define IS64BIT
//#endif

// Check GCC
#if __GNUC__
#if __x86_64__ || __ppc64__
#define IS64BIT
#else
#define IS32BIT
#endif
#endif

//#ifdef IS64BIT
//DoMy64BitOperation()
//#else
//DoMy32BitOperation()
//#endif


#endif	/* RMFMAKROS_H */

