/**
 * @authors Tobias Krauthoff <tobias.krauthoff@hhu.de, krauthoff@cs.uni-duesseldorf.de>
 * @date 26. June 2014
 * @note when reusing the timer, restart it instead of stop, join and start, because this will delete/new/... the pthread
 * Countdown timer with callback function
 */
#include "countdownTimer.h"

// External logging framework
#include "pantheios/pantheiosHeader.h"
#include "timeHandler.h"
#include "inputHandler.h"

#include <iostream>
#include <errno.h>

pthread_mutex_t countdown_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t countdown_cond = PTHREAD_COND_INITIALIZER;

//static variables
const int32_t CountdownTimer::CALLBACK_REASON_POS_FEEDBACK = 1;
const int32_t CountdownTimer::CALLBACK_REASON_NEG_FEEDBACK = 2;
const int32_t CountdownTimer::CALLBACK_REASON_TIMEOUT = 4;

/**
 * Default constructor, initialize timer with 0.0 s
 */
CountdownTimer::CountdownTimer() {
	countdownTime_=0LL;
	callbackMeasurementMethodClass_ = 0;
	isStopFlag_ = false;
	nCallbackReason_ = 0;
	absoluteEndTime_=0LL;
}

/**
 * default destructor
 */
CountdownTimer::~CountdownTimer() {

}

/**
 * Sets the countdown time
 * @param sec	uint32_t for seconds
 * @param nsec	uint32_t for nanoseconds
 */
void CountdownTimer::set_countdown(uint32_t sec, uint32_t nsec) {
	pantheios::log_DEBUG("CDTI| Countdown time set: ", pantheios::integer(sec), "s and ", pantheios::integer(nsec), "ns");
	countdownTime_=timeHandler::timespec_to_ns(sec,nsec);
}

/**
 * Sets the countdown time
 * @param time struct timespec
 */
void CountdownTimer::set_countdown(struct timespec time) {
	pantheios::log_DEBUG("CDTI| Countdown time set: ", pantheios::integer(time.tv_sec), "s and ", pantheios::integer(time.tv_nsec), "ns");
	countdownTime_=timeHandler::timespec_to_ns(time);
}

/**
 * Sets the class for callback
 * @param measurementClass for callback
 */
void CountdownTimer::set_callbackClass(measurementMethodClass* measurementClass) {
	callbackMeasurementMethodClass_ = measurementClass;
}

/**
 * Will add a callback reason to current callback reasons
 * @param reason int32_t
 */
void CountdownTimer::add_callbackReason(int32_t reason) {
	nCallbackReason_ += reason;
}

/**
 * Sets the callback reason, old values will be removed
 * @param reason  int32_t
 */
void CountdownTimer::set_callbackReason(int32_t reason) {
	nCallbackReason_ = reason;
}

/**
 * Runs the countdown and if the countdown expires, callback will be called
 */
void CountdownTimer::run() {
	pantheios::log_DEBUG("CDTI| Entering run() with seconds: ", pantheios::integer(countdownTime_));
	int retWaitValue = 0;
    timespec tspec;
	//while (!isStopFlag_){

	while (retWaitValue != ETIMEDOUT && !isStopFlag_) {
		// get time until countdown should run
		int64_t wait = calculateAbsolutWaitTime();
		pantheios::log_DEBUG("CDTI| run() waiting for ", pantheios::integer(wait));

		// conditional wait
        tspec = timeHandler::ns_to_timespec(wait);
		pthread_mutex_lock(&countdown_mutex);
		retWaitValue = pthread_cond_timedwait(&countdown_cond, &countdown_mutex, &tspec);
		pthread_mutex_unlock(&countdown_mutex);
	}

	if (retWaitValue != ETIMEDOUT) { // countdown was stopped or restarted
		pantheios::log_DEBUG("CDTI| countdown stopped");
	} else { // countdown expired
		// check callback class for null and do the callback
		if (callbackMeasurementMethodClass_ != 0) {
			pantheios::log_DEBUG("CDTI| countdown expired, callback");
			callbackMeasurementMethodClass_->callback_countdownTimer(nCallbackReason_);
		} else {
			pantheios::log_ERROR("CDTI| countdown expired, but callback class is null");
		}
	}
	pantheios::log_DEBUG("CDTI| Leaving run()");
}

/**
 * Will stop the timer
 * @return int from pthread_cond_signal-call
 */
void CountdownTimer::stop() {
	pantheios::log_DEBUG("CDTI| stop()");
	isStopFlag_ = true;
	pthread_cond_signal(&countdown_cond);
}

/**
 * Sends condition signal for stop current conditional timedwait, so the timer will 'restart'
 * @return int-value from pthread_cond_signal, so 0 if everything is okay
 */
int32_t CountdownTimer::restart() {
	pantheios::log_DEBUG("CDTI| restart()");
	if (isRunning()) {
		pantheios::log_DEBUG("CDTI| send signal for restarting");
		return pthread_cond_signal(&countdown_cond);
	} else {
		pantheios::log_DEBUG("CDTI| start() called");
		start();
	}
	return 0;
}

/**
 * Sends condition signal for stop current conditional timedwait, so the timer will 'restart' with new time
 * @param sec	uint32_t for seconds
 * @param nsec	uint32_t for nanoseconds
 * @return int-value from pthread_cond_signal, so 0 if everything is okay
 */
int32_t CountdownTimer::restartWithTime(uint32_t sec, uint32_t nsec) {
	pantheios::log_DEBUG("CDTI| restart() with ", pantheios::integer(sec), "s, ", pantheios::integer(nsec), " ns");
	set_countdown(sec, nsec);
	restart();
	return 0;
}

/**
 * Sends condition signal for stop current conditional timedwait, so the timer will 'restart' with new time
 * @param time struct timespec
 * @return int-value from pthread_cond_signal, so 0 if everything is okay
 */
int32_t CountdownTimer::restartWithTime(struct timespec time) {
	pantheios::log_DEBUG("CDTI| restart() with ", pantheios::integer(time.tv_nsec), "s, ", pantheios::integer(time.tv_sec), " ns");
	set_countdown(time);
	restart();
	return 0;
}

/**
 * Sends condition signal for stop current conditional timedwait, so the timer will 'restart' with new time
 * @param time struct timespec
 * @return int-value from pthread_cond_signal, so 0 if everything is okay
 */
int32_t CountdownTimer::restartWithTime(int64_t time) {
	pantheios::log_DEBUG("CDTI| restart() with ", pantheios::integer(time));
	set_countdown(time);
	restart();
	return 0;
}

/**
 * Calculates the absolut wait in in dependence of kCountdownTime
 * @return struct timespec with absoult time, until thread will wait
 */
int64_t CountdownTimer::calculateAbsolutWaitTime() {
    static class timeHandler timing(inputH.get_timingMethod());
	pantheios::log_DEBUG("CDTI| calculateAbsolutWaitTime()");

	// add countdown time
	absoluteEndTime_= timing.getCurrentTimeNs()+countdownTime_;
	return absoluteEndTime_;
}
