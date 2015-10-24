/**
 * @authors Tobias Krauthoff <tobias.krauthoff@hhu.de, krauthoff@cs.uni-duesseldorf.de>
 * @date 23. June 2014
 * helper for handling gpsd and gpspipe
 */

#ifndef GPSHELPER_H_
#define GPSHELPER_H_

// Namespace
using namespace std;

#include "pthreadwrapper.h"
#include "measurementMethods/measurementMethodClass.h"
#include <stdint.h>
#include "gpsDate.h"
#include <libgpsmm.h>	/* gps daemon lib */

// Define some values
#define WAITING_TIME_FACTOR			5
#define RETRY_TIME_FACTOR			2
#define SECOND						1
#define SECOND_IN_MS				1000
#define SECOND_IN_NS				1000000
#define CONSUMING_WAIT_TIME_IN_NS	100000
#define GPSD_WAIT_TIMEOUT_IN_S		5
#define ZERO						0

// class for gpsd and gpsmon
class GpsHelper  : public PThreadWrapper{
	private:
		bool isStopFlag_;
		bool isGpsdRunning_;
		bool isGpspipeRunning_;
		bool isLoggingWithGpspipe;

		uint32_t nSleepSec_;
		uint32_t nSleepNsec_;

		GpsDate lastGpsDate_;

		string pidGpspipe_;
		string serialPortGpsd_;
		string tcpPortGpsd_;
		string loggingPath_;
		measurementMethodClass *callbackMeasurementMethodClass_;

		int checkStatusWithPopen(string);
		int checkPidStatusWithPs(string, string);
		int checkGpsd();
		int checkGpspipe();
		int threadSleep(uint32_t, uint32_t);

		string getPid(string);

		void run();
		int aquisiateData(gpsmm &);

		int consumeGpsDataWithTPVTag(string, GpsDate&);
		int searchParameter(const string&, string&, string&);
		//int checkProcessForError(FILE*);

		double strToDouble(string);

	public:
		GpsHelper();
		~GpsHelper();
		void stop();

		int killGpspipe();

		bool isGpsdRunning(){ return isGpsdRunning_;};
		bool isGpspipeRunning(){ return isGpspipeRunning_;};

		void set_loggingAndPorts(string, bool&, string, string);
		void set_callbackClass(measurementMethodClass*);
		int set_sleepTime(uint32_t&, uint32_t&);
};

#endif /* GPSHELPER_H_ */
