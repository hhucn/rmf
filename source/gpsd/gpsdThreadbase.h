//=================================================
// Name        : gpsdThreadbase.h
// Author      : Tobias Krauthoff
// Version     : 1.0
// Description : GPSD Wrapper
//=================================================

/* gpsdThreadbase.h */

#ifndef gpsdThreadbase_H_
#define gpsdThreadbase_H_

using namespace std;

// External logging framework
#include "../pantheios/pantheiosHeader.h"
#include "../timeHandler.h"
#include "../inputHandler.h"

#include <algorithm>
#include <ctime>
#include <fstream>
#include <libgpsmm.h>		/* gps daemon lib */
#include <iostream>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <sstream>

#include "gpsDate.h"

// Define some values
#define WAITING_TIME_FACTOR		2
#define RETRY_TIME_FACTOR			3
#define SECOND_IN_NS				1000000
#define SECOND_IN_MS				1000
#define SECOND						1
#define CONSUMING_WAIT_TIME_IN_NS	100000
#define GPSD_WAIT_TIMEOUT_IN_S		5

extern class inputHandler input;

class gpsdThreadbase {
		// wrapper for pthreads
		// from http://peter.bourgon.org/blog/2010/10/27/who-needs-boost-a-simple-pthreads-wrapper.html
		static void * gpsdAquisitionThreadbase_dispatcher(void *arg) {
			gpsdThreadbase *gpsdtb(static_cast<gpsdThreadbase *>(arg));
			gpsdtb->aquisitionRun();
			return NULL;
		}

	private:
		// needed for posix thread
		pthread_t* mGpsdAquisitionThreadbaseThread;

		// flags
		volatile bool runningFlag;
		bool killFlag;
		bool writeFileFlag;

		gpsDate currentGpsdate;
		class timeHandler timing;
		FILE* pipeGpsd;

		// current output file and last start time
		fstream outputFile;
		string lastLogTimeStart;
		string lastLoggedTime;
		string logfile;
		string gpsdSerialPort;
		string gpsdTcpPort;

		void initValues(bool);
//		void gpsDataAquisition();
		void gpsDataConsuming(const char*);
		void aquisitionRun();

		int writeToFile(gpsDate&);
		int startGpsd();
		int killGpsd();
		int checkProcessForError(FILE*);

		double strToDouble(string);
		int searchParameter(const string &, string &, string &);

	public:
		// constructor and destructor
		gpsdThreadbase();
		gpsdThreadbase(bool);
		~gpsdThreadbase();

		gpsDate sendSignalForCondVariableAndGetData();

		// thread
		int start();
		void join();

		// emergency stop for signals
		void stop();

		bool is_running();
		void set_writeFileFlag(bool);
		void set_gpsdSerialPort(string);
		void set_gpsdTcpPort(string);

		string get_lastLogTimeStart();
		gpsDate get_lastGpsDate();

};

#endif /* gpsdThreadbase_H_ */
