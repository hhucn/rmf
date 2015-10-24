/**
 * @authors Tobias Krauthoff <tobias.krauthoff@hhu.de, krauthoff@cs.uni-duesseldorf.de>
 * @date 26. June 2014
 * Countdown timer with callback function
 */

#ifndef COUNTDOWNTIMER_H_
#define COUNTDOWNTIMER_H_

// Namespace
using namespace std;

#include <stdint.h>
#include <time.h>
#include "pthreadwrapper.h" // Wrapper for posix threads
#include "measurementMethods/measurementMethodClass.h"

class CountdownTimer : public PThreadWrapper {

	private:
		int32_t nCallbackReason_;
		bool isStopFlag_;
		int64_t countdownTime_;
		int64_t  absoluteEndTime_;
		measurementMethodClass *callbackMeasurementMethodClass_;

		void run();
		int64_t calculateAbsolutWaitTime();


	public:
		static const int32_t CALLBACK_REASON_POS_FEEDBACK;
		static const int32_t CALLBACK_REASON_NEG_FEEDBACK;
		static const int32_t CALLBACK_REASON_TIMEOUT;

		CountdownTimer();
		virtual ~CountdownTimer();

		void set_countdown(uint32_t, uint32_t);
		void set_countdown(struct timespec);
        void set_countdown(int64_t time){this->countdownTime_=time;};
		void set_callbackClass(measurementMethodClass*);
		void add_callbackReason(int32_t);
		void set_callbackReason(int32_t);
		void set_absouluteEndTime(double);
		int32_t get_callbackReason(){ return nCallbackReason_;};
		int64_t get_absoluteEndTimeNs(){ return absoluteEndTime_;};

		int32_t restart();
		int32_t restartWithTime(uint32_t, uint32_t);
		int32_t restartWithTime(struct timespec);
        int32_t restartWithTime(int64_t time);
		void stop();
};

#endif /* COUNTDOWNTIMER_H_ */
