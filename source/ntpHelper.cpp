/**
 * @authors Tobias Krauthoff <tobias.krauthoff@hhu.de, krauthoff@cs.uni-duesseldorf.de>
 * @date 23. June 2014
 * helper for handling ntp and ntpd / implements pthreadwrapper
 */

#include "ntpHelper.h"

// External logging framework
#include "pantheios/pantheiosHeader.h"
#include <unistd.h> //getuid

/**
 * Default constructor, will set default sleep time to 10.0s
 */
NtpHelper::NtpHelper() {
	isStopFlag_ = false;
	isNtpRunningFine_ = false;
	isClient_ = false;
	nMaxAttemptRestartNtp_ = 10;
	nAttemptReadNtp_ = 0;
	nMaxAttemptReadNtp_ = 3;
	nSleepSec_ = 15;
	nSleepNsec_ = 0;
	nReach_ = 0;
	nOffset_ = 0;
	nJitter_ = 0;
	nMaxJitter_ = 100;
	callbackMeasurementMethodClass_ = 0;

	read_status_ = "ntpq -pn";
	restart_ntp_ = "service ntp restart";
}

/**
 * Default destructor
 */
NtpHelper::~NtpHelper() {
}

/**
 * Sets the class for callback
 * @param measurementClass for callback
 */
void NtpHelper::set_callbackClass(measurementMethodClass* measurementClass){
	callbackMeasurementMethodClass_ = measurementClass;
}

/**
 * Stop ntp thread, so the stopThreadFlag will be set and thread will stop controlling ntp
 * @return 0
 */
void NtpHelper::stop() {
	isStopFlag_ = true;
}

/**
 * Will run the thread. Every x seconds ntp fix will be checked and maybe restarted.
 * @return 0
 */
void NtpHelper::run() {
	pantheios::log_DEBUG("NTPH| Entering run()");
	string readStatus = "ntpq -pn";
	FILE* pipeNtpq = NULL;
	//FILE* pipeRestart;
	int32_t attemptRestartNtp = 0;

	while (!isStopFlag_) {
		sched_yield();

		// if gps or pps uns unreachable or offset to huge, restart ntp for fresh fix
		if (readStatusOfNtpq(pipeNtpq) == -1) {
			nAttemptReadNtp_ ++;
			if (nAttemptReadNtp_ == nMaxAttemptReadNtp_){
				if (restartNTP(attemptRestartNtp) == -1){
					break;
				}
			}
		}

		if (callbackMeasurementMethodClass_ != 0){
			callbackMeasurementMethodClass_->callback_ntpHelper(isNtpRunningFine_);
		}

		threadSleep(nSleepSec_, nSleepNsec_);
	}
	pantheios::log_DEBUG("NTPH| Leaving run()");
}

/**
 * Checks the status of ntpq
 * @return 0, when ntpq is running and jitter is small, -1 otherwise
 */
int32_t NtpHelper::readStatusOfNtpq(FILE* pipeNtpq){
	pipeNtpq = popen(read_status_.c_str(), "r");
	isNtpRunningFine_ = true;
	int rv = 0;

	if (!pipeNtpq) {
		pantheios::log_DEBUG("NTPH| Could not execute command: ntpq -pn");
	} else {
		char line[128];
		string res = "";

		// getting all information out of the pipeNtpq
		while (!feof(pipeNtpq)) {
			if (fgets(line, 128, pipeNtpq) != NULL) res = line;

			// check * line for "reach" and "jitter"
			if (res.find("*") != string::npos) {
				if (parseLineAndControlValues(res, "*") == -1) {
					rv = -1;
					pantheios::log_ERROR("NTPH| Error: While parsing ntpq * line -> not enough values in line");
					isNtpRunningFine_ = false;
				} else {
					isNtpRunningFine_ = true;
					rv = 0;
				}
			}
		}
	}
	pclose(pipeNtpq);
	return rv;
}

/**
 * Checks for root right and restart ntp; on restart, thread will sleep 5min
 * @return -1 when there there are not root rights
 */
int32_t NtpHelper::restartNTP(int32_t& attemptRestartNtp){
	pantheios::log_ERROR("NTPH| Error: NTP sync with PPS is not possible three times! Trying to restart, then wait 10s.");
	// old code!
//			string cmd = "";
//			if (isClient_){
//				cmd = "service ntp restart";
//			} else {
//				cmd = "service ntp restart";
//			}
//			pipeRestart = popen(cmd.c_str(), "r");
//
//			char line[128];
//			string res = "";
//			while (!feof(pipeRestart))
//				if (fgets(line, 128, pipeRestart) != NULL) res += line;
//			replace(res.begin(), res.end(), '\n', '|');
//			pantheios::log_DEBUG("NTPH| Result from restarting ntp: ", res);

	// check for root rights
	if (geteuid() != 0){
		pantheios::log_ERROR("NTPH| No root rights for restarting NTP, thread will quit.");
		isStopFlag_ = true;
		isNtpRunningFine_ = false;
		return -1;
	}
	if (system(restart_ntp_.c_str()) != 0){
		pantheios::log_DEBUG("NTPH| Could not execute \"service ntp restart\"");
	}

	attemptRestartNtp++;
	threadSleep(nSleepSec_, nSleepNsec_); // extra threadSleep time
	if (attemptRestartNtp >= nMaxAttemptRestartNtp_) {
		pantheios::log_ERROR("NTPH| Error: NTP was restarted more than 10 times...whats wrong? NTP will be restarted later!");
		isNtpRunningFine_ = false;
		//isStopFlag_ = true;
	} else {
		pantheios::log_DEBUG("NTPH| NTP was restarted, wait 300s for fix!");
		isNtpRunningFine_ = true;
		attemptRestartNtp = 0;
		uint32_t sleeps = 300;
		uint32_t sleepns = 0;
		threadSleep(sleeps, sleepns);
	}
	return 0;
}

/**
 * Will tokenize given line and check the values like described
 * @param line, recent line from ntpq -p command
 * @param signal, gps or pps
 * @return 0, if line has 9 tokens (0=remote, 1=refid, 2=st, 3=t, 4=when, 5=poll, 6=reach, 7=delay, 8=offset, 9=jitter) and everything is good, -1 otherwise
 */
int32_t NtpHelper::parseLineAndControlValues(string& line, const string& signal) {
	pantheios::log_DEBUG("NTPH| Entering parseLineAndControlValues()");
	vector<string> vec = strsplit(line);
	//values: 0=remote, 1=refid, 2=st, 3=t, 4=when, 5=poll, 6=reach, 7=delay, 8=offset, 9=jitter
	if (vec.size() < 9) {
		pantheios::log_ERROR("NTPH|ERROR: Leaving parseLineAndControlValues() because of too less values");
		return -1;
	}

	nReach_ = atof(vec.at(6).c_str());
	nOffset_ = atof(vec.at(8).c_str());
	nJitter_ = atof(vec.at(9).c_str());

	pantheios::log_DEBUG("NTPH| NTP with signal:", signal, " is running with: reach=", pantheios::real(nReach_), ", offset=", pantheios::real(nOffset_),
			" and jitter=", pantheios::real(nJitter_));

	pantheios::log_DEBUG("NTPH| Leaving parseLineAndControlValues()");
	return  nReach_ > 0 && nJitter_ < nMaxJitter_ ? 0 : -1;
}

/**
 * Will split the string by all whitespaces and erase multiple spaces
 * @param str, string which should be splitted
 * @return vector<string> with all tokens
 */
vector<string> NtpHelper::strsplit(string& str) {
	vector<string> vec;
	// cut all whitespaces at beggining
	str.erase(0, str.find_first_not_of(" "));
	// endpos of fist token
	size_t endpos = str.find(" ");

	while (endpos != string::npos) {
		vec.push_back(str.substr(0, endpos));	// save current token
		str.erase(0, endpos);					// delete current token
		endpos = str.find_first_not_of(" ");	// get last pos from current whitespaces
		str.erase(0, endpos);					// delete this spaces
		endpos = str.find_first_of(" ");		// position from next token
	}

	// push back last one, which was terminated by newline symbol
	vec.push_back(str);

	return vec;
}

/**
 * Sleep until the relative given input time
 *
 * @param seconds 		absolute seconds for sleeping
 * @param nanoseconds	absolute nanoseconds for sleep
 * @return 0 if thread could sleep, -1 otherwise
 */
int32_t NtpHelper::threadSleep(uint32_t seconds, uint32_t nanoseconds) {
	pantheios::log_DEBUG("NTPH| Entering threadSleep()");
	struct timespec sleeping;
	clock_gettime(CLOCK_REALTIME, &sleeping);

	if (nanoseconds >= 1000000000) {
		sleeping.tv_sec += static_cast<time_t>(seconds + 1);
		sleeping.tv_nsec += static_cast<long>(nanoseconds - 1000000000);
	} else {
		sleeping.tv_sec += static_cast<time_t>(seconds);
		sleeping.tv_nsec += static_cast<long int>(nanoseconds);
	}
	// clock can be interrupted with signals...but it should not occure
	if (clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &sleeping, NULL) != 0) {
		pantheios::log_DEBUG("NTPH| clock_nanosleep error! Values: ", pantheios::integer(seconds), "/", pantheios::integer(nanoseconds));
		pantheios::log_DEBUG("NTPH| Leaving threadSleep()");
		return -1;
	}
	pantheios::log_DEBUG("NTPH| Leaving threadSleep()");
	return 0;
}

/**
 * Will set sleep time. The thread will sleep this time between every run.
 * @param seconds		sets the sleep time variable for seconds
 * @param nanoseconds	sets the sleep time variable for nanoseconds
 */
int32_t NtpHelper::set_sleepTime(uint32_t& seconds, uint32_t& nanoseconds){
	if (seconds < 0 || nanoseconds >= 1000000000){
		pantheios::log_DEBUG("NTPH Malicious values in setSleepTime: ", pantheios::integer(seconds), "/", pantheios::integer(nanoseconds));
		return -1;
	}
	nSleepSec_  = seconds;
	nSleepNsec_ = nanoseconds;
	return 0;
}
