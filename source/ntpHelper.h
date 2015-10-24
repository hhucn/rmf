/**
 * @authors Tobias Krauthoff <tobias.krauthoff@hhu.de, krauthoff@cs.uni-duesseldorf.de>
 * @date 23. June 2014
 * helper for handling ntp and ntpd
 */

#ifndef NTPHELPER_H_
#define NTPHELPER_H_

// Namespace
using namespace std;

// Wrapper for posix threads
#include "pthreadwrapper.h"


#include <stdint.h>
#include <vector>
#include <string>

// Includes RMF
#include "measurementMethods/measurementMethodClass.h"

// class for ntp and ntpd
class NtpHelper : public PThreadWrapper {

	private:
		string read_status_;
		string restart_ntp_;

		bool isClient_;
		bool isStopFlag_;
		bool isNtpRunningFine_;

		int32_t nMaxAttemptRestartNtp_ ;
		int32_t nMaxAttemptReadNtp_ ;
		int32_t nAttemptReadNtp_ ;
		uint32_t nSleepSec_;
		uint32_t nSleepNsec_;

		double nReach_;
		double nJitter_;
		double nMaxJitter_;
		double nOffset_;

		measurementMethodClass *callbackMeasurementMethodClass_;

		vector<string> strsplit(string&);

		int32_t parseLineAndControlValues(string&, const string&);
		int32_t readStatusOfNtpq(FILE*);
		int32_t restartNTP(int32_t& );
		int32_t threadSleep(uint32_t, uint32_t);

		void run();

	public:
		NtpHelper();
		~NtpHelper();
		void stop();

		double get_reach(){return nReach_;};
		double get_jitter(){return nJitter_;};
		double get_offset(){return nOffset_;};

		bool is_runningFine(){return isNtpRunningFine_;};
		int32_t set_sleepTime(uint32_t&, uint32_t&);
		void set_callbackClass(measurementMethodClass*);
		void setClient(bool isClient){isClient_ = isClient;};
};

#endif /* NTPHELPER_H_ */
