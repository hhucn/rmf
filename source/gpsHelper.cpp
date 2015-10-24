/**
 * @authors Tobias Krauthoff <tobias.krauthoff@hhu.de, krauthoff@cs.uni-duesseldorf.de>
 * @date 23. June 2014
 * helper for handling gpsd and gpspipe
 */

#include "gpsHelper.h"

// External logging framework
#include "pantheios/pantheiosHeader.h"

#include <string.h>
#include <unistd.h> //getuid

/**
 * Default constructor / initialize variables with default values
 */
GpsHelper::GpsHelper() {
	pidGpspipe_ 					= "";
	serialPortGpsd_	 				= "";
	tcpPortGpsd_					= "";
	loggingPath_					= "results/";
	isStopFlag_ 					= false;
	isGpsdRunning_ 					= false;
	isGpspipeRunning_ 				= false;
	isLoggingWithGpspipe 			= false;
	callbackMeasurementMethodClass_ = 0;
	nSleepSec_ 						= 1;
	nSleepNsec_ 					= 0;
}

/**
 * Default destructor
 */
GpsHelper::~GpsHelper() {
	killGpspipe();
}

/**
 * Sets the class for callback
 * @param measurementClass for callback
 */
void GpsHelper::set_callbackClass(measurementMethodClass* measurementClass){
	callbackMeasurementMethodClass_ = measurementClass;
}

/**
 * Sets boolean for logging with gpspipe and serial port
 * @param path for the logfile
 * @param isLogging boolean for logging
 * @param serialPort servial port for gps, default is /dev/ttyS0
 * @param tcpPort tcp port for gps, default ist 2479
 */
void GpsHelper::set_loggingAndPorts(string path, bool& isLogging, string serialPort, string tcpPort){
	isLoggingWithGpspipe = isLogging;
	serialPortGpsd_ = serialPort;
	tcpPortGpsd_ = tcpPort;
	loggingPath_ = path;
}


/* --------------------------------------------------------------------------------
 * T H R E A D   S T U F F
 * -------------------------------------------------------------------------------- */

/**
 * Stop gos thread, so the stopThreadFlag will be set and thread will stop controlling gps
 * @return 0
 */
void GpsHelper::stop() {
	isStopFlag_ = true;
	killGpspipe();
}

/**
 * Will run the thread. Every x seconds gps fix will be checked and maybe restarted.
 * @return 0
 */
void GpsHelper::run() {
	pantheios::log_DEBUG("GPSH| Entering run()");
	bool doCallback = false;

// TODO CURRENTLY NOT INCLUDED; BECAUSE DEBIAN 7 HAS PROBLEMS WITH LIBGPS BECAUSE OF GLIBC IS NOT SUPPORTED ANYMORE
	// starting gps receiver
	int32_t attempt;
	gpsmm gps_receiver("localhost", tcpPortGpsd_.c_str());

	// wait for gps stream
	uint32_t WATCH_COND = WATCH_ENABLE | WATCH_JSON | WATCH_NEWSTYLE | WATCH_SCALED | WATCH_TIMING;
	for (attempt = 1; attempt <= 5 && (gps_receiver.stream(WATCH_COND) == NULL); attempt++) {
		pantheios::log_ERROR("GPSD| Gps receiver stream is null, look again in ", pantheios::integer(RETRY_TIME_FACTOR),
				" seconds, attempt ", pantheios::integer(attempt), "/5");

		threadSleep(RETRY_TIME_FACTOR, ZERO);
		//continue;// I will try to connect to gpsd again
	}

	// did we need to many attempts, so that there is probably no gps daemon?
	if (attempt == 6) {
		pantheios::log_DEBUG("GPSD| No gpsd is running after ", pantheios::integer(attempt) , " attempts -> gpsd thread will quit ");
		isStopFlag_ = true;
	}

	while (!isStopFlag_) {
		sched_yield();

		if (getuid() != 0){
			pantheios::log_ERROR("GPSH| No root privileges for checking status, thread will quit.");
			isStopFlag_ = true;
			break;
		}

		// check gpsd
		if (checkGpsd() == -1) {
			pantheios::log_DEBUG("MEME| -> Error while checking gpsd status, ",
				"measurement will run without getting gps data! (restart after sleeptime)");
		} else {
			pantheios::log_DEBUG("MEME| Gpsd found!");
		}

		// check gpsusage, if it is running, should run and so on
		if (checkGpspipe() != 0) {
			pantheios::log_ERROR("MEME| -> Error while checking gpspipe status, "
				,"measurement will run without logging gps data! (restart after sleeptime)");
		} else {
			if (isLoggingWithGpspipe) {
				pantheios::log_DEBUG("MEME| Gpspipe found!");
			} else {
				pantheios::log_DEBUG("MEME| Gpspipe found, but you don't want to log, so kill gpspipe");
				killGpspipe();
			}
		}

		// TODO CURRENTLY NOT INCLUDED; BECAUSE DEBIAN 7 HAS PROBLEMS WITH LIBGPS BECAUSE OF GLIBC IS NOT SUPPORTED ANYMORE
		doCallback = aquisiateData(gps_receiver) != -1;
		pantheios::log_DEBUG("GPSH| Aquisiation of data from gps_receiver " , (doCallback ? "": "not "), "successful!");

		if (callbackMeasurementMethodClass_ != 0 && doCallback){
			callbackMeasurementMethodClass_->callback_gpsHelper(lastGpsDate_);
		}

		threadSleep(nSleepSec_, nSleepNsec_);

	}

	pantheios::log_DEBUG("GPSH| Leaving run()");
}

/**
 * Aquisitates all gpsd data, which is receives as json stream.
 */
// TODO CURRENTLY NOT INCLUDED; BECAUSE DEBIAN 7 HAS PROBLEMS WITH LIBGPS BECAUSE OF GLIBC IS NOT SUPPORTED ANYMORE
int32_t GpsHelper::aquisiateData(gpsmm& gps_receiver){
	pantheios::log_DEBUG("GPSD| Entering aquisitionRun()");

	// consume stream
	struct gps_data_t* newdata;

	/*
	 Blocking check to see if data from the daemon is waiting. Named something like
	 "waiting()" and taking a wait timeout as argument. Note that choosing a wait
	 timeout of less than twice the cycle time of your device will be hazardous, as
	 the receiver will probably not supply input often enough to prevent a spurious
	 error indication. For the typical 1-second cycle time of GPSes this implies a
	 minimum 2-second timeout.
	 */
	if (!gps_receiver.waiting((double) WAITING_TIME_FACTOR * (double) SECOND_IN_NS))
		return -1;

	// error in buffer?
	//if (checkProcessForError(pipe) == -1)
	//	return -1;

	// gps data struc equals null?
	if ((newdata = gps_receiver.read()) == NULL) {
		pantheios::log_ERROR("GPSD| read error");
		//pclose(pipe);
		return -1;
	}

	// now get the fine data
	const char* buffer = NULL;
	buffer = gps_receiver.data();

	consumeGpsDataWithTPVTag(buffer, lastGpsDate_);

	pantheios::log_DEBUG("GPSD| Leaving aquisitionRun() via consumeGpsData(buffer, lastGpsDate_)");
	return consumeGpsDataWithTPVTag(buffer, lastGpsDate_);
}

// OLD METHOD FOR CHECKING GPS STATUS, WHEN THIS CLASS WAS NOT A THREAD!!!
/*
 * Checks, weather all gpsd-services and processes are running
 * @param logGpsd true when gpspipe should be used
 * @param gpsdSerialPort serial port for gps mouse
 * @return 0 when no error or misbehaviour occured, -1 otherwise
 *
int32_t GpsHelper::checkAllServices(bool &logGpsd, string gpsdSerialPort) {
	int32_t rv = 0;
	if (useGpsd) {

		// check for root privileges
		if (getuid() == 0) {
			// check gpsd
			if (checkGpsd(gpsdSerialPort) == -1) {
				pantheios::log_ERROR("MEME| -> Error while checking gpsd status, measurement will run without getting gps data!");
				useGpsd = false;
				return 0;
			} else {
				pantheios::log_NOTICE("MEME| Gpsd found!");
			}
		} else {
			pantheios::log_NOTICE("MEME| No root privileges for checking gpsd status.");
		}

		// check for root privileges
		if (getuid() == 0) {
			// check gpsusage, if it is running, should run and so on
			if (checkGpspipe() != 0) {
				pantheios::log_ERROR("MEME| -> Error while checking gpspipe status, measurement will run without logging gps data!");
				useGpsd = false;
				logGpsd = false;
				return -1;
			} else {
				if (logGpsd) {
					pantheios::log_NOTICE("MEME| Gpspipe found!");
				} else {
					pantheios::log_NOTICE("MEME| Gpspipe found, but you don't want to log, so kill gpspipe");
					killGpspipe();
				}
			}
		} else {
			pantheios::log_NOTICE("MEME| No root privileges for checking gpspipe status.");
		}
	}
	retu
	rn rv;
}
*/

/* --------------------------------------------------------------------------------
 * C O N S U M I N G   S T U F F
 * -------------------------------------------------------------------------------- */

/**
 * Consumes given data and creates GPS
 * @param data for consuming
 * @param gpsdate return value
 * @return 0 when data conatins TPV values, -1 otherwise
 */
int32_t GpsHelper::consumeGpsDataWithTPVTag(string data, GpsDate& gpsdate){
	// we only want TPV data
	size_t tpvPos = data.find("{\"class\":\"TPV\"");
	if (tpvPos == string::npos) {
		return -1;
	}

	// get TPV pattern
	data.erase(0, tpvPos);
	data = data.substr(0, data.find_first_of("}")+1); // +1 for the }

	// variables for string manipulation
	string pattern = "";
	string retval = "";

	gpsdate.set_valid(true);
	// keywords from http://www.catb.org/gpsd/gpsd_json.html
	pattern = "\"mode\":";	if (searchParameter(pattern, data, retval) == 0) gpsdate.set_mode(atoi(retval.c_str()));
	pattern = "\"time\":";	if (searchParameter(pattern, data, retval) == 0) gpsdate.set_time(retval.substr(1, retval.length()-1));
	pattern = "\"lat\":";	if (searchParameter(pattern, data, retval) == 0) gpsdate.set_latitude(strToDouble(retval));
	pattern = "\"lon\":";	if (searchParameter(pattern, data, retval) == 0) gpsdate.set_longitude(strToDouble(retval));
	pattern = "\"alt\":";	if (searchParameter(pattern, data, retval) == 0) gpsdate.set_altitude(strToDouble(retval));
	pattern = "\"ept\":";	if (searchParameter(pattern, data, retval) == 0) gpsdate.set_timeError(atof(retval.c_str()));
	pattern = "\"epx\":";	if (searchParameter(pattern, data, retval) == 0) gpsdate.set_latitudeError(strToDouble(retval));
	pattern = "\"epy\":";	if (searchParameter(pattern, data, retval) == 0) gpsdate.set_longitudeError(strToDouble(retval));
	pattern = "\"epv\":";	if (searchParameter(pattern, data, retval) == 0) gpsdate.set_altitudeError(strToDouble(retval));
	pattern = "\"track\":";	if (searchParameter(pattern, data, retval) == 0) gpsdate.set_track(strToDouble(retval));
	pattern = "\"speed\":";	if (searchParameter(pattern, data, retval) == 0) gpsdate.set_speed(strToDouble(retval));
	pattern = "\"eps\":";	if (searchParameter(pattern, data, retval) == 0) gpsdate.set_speedError(strToDouble(retval));

	// mode=2 is a 2d fix, so mode 3 is 3d, but 1 is not valid
	gpsdate.set_valid(gpsdate.get_mode() != 1);
	return 0;
}

// not needed, dummy from preliminary work
///**
// * Checks given file pipe for "ERROR"
// * @param pipe FILE*
// * @return 0 when there is no error, -1 otherwise
// */
//int32_t GpsHelper::checkProcessForError(FILE* pipe){
//	char buff[128];
//	string result = "";
//	// getting all information out of the pipe, while killflag is not set
//	while( !feof(pipe) )
//		if( fgets(buff, 128, pipe ) != NULL)
//			result += buff;
//
//	pantheios::log_DEBUG("GPSD| checkProcessForError -> pipe: \"", result, "\"");
//
//	// is the pipe was empty, than everything worked fine
//	if ( result.find("ERROR") != string::npos ){
//		pantheios::log_DEBUG("GPSD| check process: device has an error on " , serialPortGpsd_);
//		return -1; // running and kill flag will be set in emergency stop later
//	} else {
//		pantheios::log_DEBUG("GPSD| checkProcess: device is running on " , serialPortGpsd_);
//		return 0;
//	}
//}

/**
 * Searches the fix in the given input string.
 * Generates a substring from the position of the tag to the following ",".
 *
 * @param fix, parameter tag
 * @param input, parameter string
 * @param retval, string pointer to put generated substring in
 *
 * @return 0 on success, -1 otherwise
 */
inline int32_t GpsHelper::searchParameter(const string &fix, string &input, string &retval) {
	size_t startpos	= input.find(fix);
	if (startpos == string::npos) return (-1);

	size_t endpos = input.find(",", startpos);
	retval = input.substr(startpos, endpos - startpos);
	retval.erase(0, fix.length());
	input.erase(startpos, endpos - startpos + 1);

	return (0);
}

/**
 * Converts given input string to double value
 * @param s, given string, which should be a double value
 * @return converted double or -1
 */
inline double GpsHelper::strToDouble(string s) {
	istringstream i(s);
	double x;
	if (!(i >> x)) return -1;
	return x;
}


/* --------------------------------------------------------------------------------
 * C H E C K I N G   M E T H O D S
 * -------------------------------------------------------------------------------- */

/**
 * Checks status of a service with given name
 * @param prg, name of this, which status should be checked
 * @return 0, when service 'is running', -1 otherwise
 */
int32_t GpsHelper::checkStatusWithPopen(string prg) {
	int32_t rv = 0;
	string readStatus = "service " + prg + " status";
	FILE* pipe = popen(readStatus.c_str(), "r");
	if (!pipe) {
		pantheios::log_DEBUG("GPSH| Could not open service for checking ", prg, "'s status.");
	} else {
		char buffer[128];
		string result = "";

		// getting all information out of the pipe
		while (!feof(pipe))
			if (fgets(buffer, 128, pipe) != NULL) result += buffer;

		replace(result.begin(), result.end(), '\n', '|');
		pantheios::log_DEBUG("GPSH| CheckStatusWithPopen: ", result);

		// is the pipe was empty, than everythink worked fine
		if (result.find("is running") != string::npos) {
			pantheios::log_DEBUG("GPSH| CheckStatusWithPopen: ", prg, " is running fine.");
		} else if (result.find("failed") != string::npos) {
			pantheios::log_DEBUG("GPSH| CheckStatusWithPopen: ", prg, " has failed.");
			rv = -1;
			isGpsdRunning_ = false;
			isGpspipeRunning_ = false;
		} else if (result.find("is not running") != string::npos) {
			pantheios::log_DEBUG("GPSH| CheckStatusWithPopen: ", prg, " is not running.");
			rv = -1;
		} else {
			pantheios::log_DEBUG("GPSH| CheckStatusWithPopen: Unknown result while checking ", prg, ":", result);
			rv = -1;
		}
	}
	pclose(pipe);
	return rv;
}

/**
 * Return process id (pid unter /var/run*.pid) of input string or empty string
 * @param prg, process which id should be returned
 * @return pid as string or empty string
 */
string GpsHelper::getPid(string prg) {
	string pid = "";

	string line;
	string ps = "/var/run/" + prg + ".pid";
	ifstream myfile(ps.c_str(), ifstream::out);
	if (myfile.is_open()) {
		while (getline(myfile, line)) {
			pid += line;
		}
		myfile.close();
	} else {
		pantheios::log_DEBUG("GPSH| CheckStatusWithPid: Unable to open file ", ps);
	}
	return pid;
}

/**
 * Checks Status of an programm with specific pid via unix ps command
 * @param pid, process id of the programm
 * @param prg, programm name itself
 * @return 0, when the program is running, -1 otherwise
 */
int32_t GpsHelper::checkPidStatusWithPs(string pid, string prg) {
	int32_t rv = 0;
	string ps = "ps " + pid;
	FILE* pipe = popen(ps.c_str(), "r");
	if (!pipe) {
		pantheios::log_DEBUG("GPSH| CheckPidStatusWithPs: Could not run ", ps);
	} else {
		char buffer[PATH_MAX];
		string result = "";

		// getting all information out of the pipe
		while (!feof(pipe))
			if (fgets(buffer, PATH_MAX, pipe) != NULL) result += buffer;

		replace(result.begin(), result.end(), '\n', '|');
		pantheios::log_DEBUG("GPSH| CheckPidStatusWithPs got as result: ", result);

		// is the pipe was empty, than everythink worked fine
		if (result.find(prg) != string::npos) {
			pantheios::log_DEBUG("GPSH| CheckPidStatusWithPs: ", prg, " is running fine.");
		} else {
			pantheios::log_DEBUG("GPSH| CheckPidStatusWithPs: ", prg, " is not running");
			rv = -1;
		}
	}
	pclose(pipe);

	return rv;
}

// NOT USED ANYMORE
/**
 * Returns the serial port from /dev/tty/.., when something is plugged and empty string otherwise
 * @return serial port or empty string
 */
/*
string GpsHelper::getSerialForGpsMouse() {
	string serial = "";
	string jobs = "dmesg | grep tty";
	FILE* pipeDmesg = popen(jobs.c_str(), "r");
	if (!pipeDmesg) {
		pantheios::log_DEBUG("GPSH| Could not open dmesg for checking serial ports.");
	} else {
		char buff1[512];
		string res1 = "";

		// getting all information out of the pipe
		while (!feof(pipeDmesg))
			if (fgets(buff1, 512, pipeDmesg) != NULL) res1 += buff1;

		pantheios::log_DEBUG("GPSH| dmesg | grep tty: ", res1);

		// is the pipe was empty, than everythink worked fine
		size_t startpos = res1.find("ttyS");
		if (startpos != string::npos) {
			pantheios::log_DEBUG("GPSH| Found serial port with gps mouse (maybe). Now look closer");

			// define retval
			size_t endpos = res1.find(" ", startpos);
			serial = res1.substr(startpos, endpos - startpos);

			// look for gps data
			string cat = "sudo cat /dev/" + serial;
			FILE* pipeCat = popen(cat.c_str(), "r");

			// read pipe
			char buff2[64];
			string res2 = "";
			if (!feof(pipeCat) && (fgets(buff2, 64, pipeCat) != NULL)) res2 += buff2;

			// no data, no retval
			// either there is no data in the pipe or there is in-/output error
			if (((res2.find("GPGSV") == string::npos) && (res2.find("PGRME") == string::npos) && (res2.find("GPG") == string::npos) && res2.empty())
					|| (res2.find(serial) != string::npos)) {
				serial = "";
			}

			pclose(pipeCat);

		} else {
			pantheios::log_DEBUG("GPSH| No serial port with gps mouse found.");
		}
	}
	pclose(pipeDmesg);
	if (serial.empty()){
		pantheios::log_DEBUG("GPSH| Serial for gps mouse is empty");
	} else {
		pantheios::log_DEBUG("GPSH| Gpsd mouse should be on seriel port ", serial);
	}
	return serial;
}*/

/**
 * Checks, if gpsd is running as service. If it is not running, the method will try to start it once and then checks again
 * @return 0, when gpsd is running, -1 otherwise
 */
int32_t GpsHelper::checkGpsd() {
	// check gpsd
	bool gpsd = (checkStatusWithPopen("gpsd") == 0);

	// if gpsd is not running, start it!
	if (!gpsd) {

		// try to start gpsd
		// FIXME CHECK WHICH CMD IS RIGHT
		string cmd;
		if (serialPortGpsd_.compare("/dev/ttyS0") != 0)
			cmd = "gpsd -G -n -D 5" + serialPortGpsd_;
		else
			cmd = "/etc/init.d/gpsd restart";
		pantheios::log_DEBUG("GPSH| Executing \"", cmd, "\", because gpsd was not running");
		if (system(cmd.c_str()) != 0){
			pantheios::log_DEBUG("GPSH| Could not execute\"", cmd,"\"");
		}

		// check status again
		gpsd = (checkStatusWithPopen("gpsd") == 0);
		if (gpsd) {
			pantheios::log_DEBUG("GPSH| Gpsd is now running, but we have to wait for a 3d fix.");
			isGpsdRunning_ = true;
			return 0;
		} else {
			pantheios::log_DEBUG("GPSH| Gpsd could not be started on ", serialPortGpsd_, "!");
			isGpsdRunning_ = false;
			return -1;
		}
	}

	pantheios::log_DEBUG("GPSH| Gpsd is running.");
	isGpsdRunning_ = true;
	return 0;
}

/**
 * Checks, if gpspipe is running as process. If it is not running, the method will try to start it once and then checks again
 * @return 0, when gpspipe is running, -1 otherwise
 */
int32_t GpsHelper::checkGpspipe() {
	// check if pid is empty
	if (pidGpspipe_.empty()) pidGpspipe_ = getPid("gpspipe");

	pantheios::log_DEBUG("GPSH| Gpspipe's pid is ", (pidGpspipe_.empty() ? "empty" : pidGpspipe_));

	// if pid is not empty, check for running process
	isGpspipeRunning_ = false;
	if (!pidGpspipe_.empty())
		isGpspipeRunning_ = (checkPidStatusWithPs(pidGpspipe_, "gpspipe") == 0);

	// if process runs, everything is fine, otherwise try to start is
	if (isGpspipeRunning_) {
		pantheios::log_DEBUG("GPSH| Gpspipe's is running with pid ", pidGpspipe_);
		isGpspipeRunning_ = true;
		return 0;
	} else {
		pantheios::log_DEBUG("GPSH| Gpspipe is not running, because there is no process with: ", pidGpspipe_);

		string cmd = "sudo sh -c 'gpspipe -w -t -T %s -p -o " + loggingPath_ + "gpspipe.log & echo $! > /var/run/gpspipe.pid'";
		if (system(cmd.c_str())){
			pantheios::log_DEBUG("GPSH| Execute\"", cmd, "\", because gpspipe is not running");
		} else {
			pantheios::log_DEBUG("GPSH| Could not execute\"", cmd,"\"");
		}

		// get new pid and check for running process
		pidGpspipe_ = getPid("gpspipe");
		pantheios::log_DEBUG("GPSH| Gpspipe was restartet and pid is ", (pidGpspipe_.empty() ? "empty" : pidGpspipe_));
		if (!pidGpspipe_.empty()) {
			isGpspipeRunning_ = (checkPidStatusWithPs(pidGpspipe_, "gpspipe") == 0);
		} else {
			pantheios::log_DEBUG("GPSH| Gpspipe is not running and could not be started");
			isGpspipeRunning_ = false;
			return -1;
		}
	}

	isGpspipeRunning_ = true;
	return 0;
}

/**
 * Will kill gpspipe
 * @return 0
 */
int32_t GpsHelper::killGpspipe() {
	pantheios::log_DEBUG("GPSH| Is gpspipe running = ", pantheios::boolean(isGpspipeRunning_));
	if (isGpspipeRunning_) {
		string kill = "sudo kill " + pidGpspipe_;
		pantheios::log_NOTICE("GPSH| Gpspipe was killed");
		FILE* pipe = popen(kill.c_str(), "r");
		pclose(pipe);
		return 0;
	} else {
		pantheios::log_DEBUG("GPSH| Gpspipe is not running, so it wont be killed.");
		return 0;
	}
}


/* --------------------------------------------------------------------------------
 * M E T H O D S   F O R   S L E E P I N G
 * -------------------------------------------------------------------------------- */

/**
 * Sleep until the relative given input time
 *
 * @param seconds 		absolute seconds for sleeping
 * @param nanoseconds	absolute nanoseconds for sleep
 * @return 0 if thread could sleep, -1 otherwise
 */
int32_t GpsHelper::threadSleep(uint32_t seconds, uint32_t nanoseconds) {
	pantheios::log_DEBUG("GPSH| Entering threadSleep()");
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
		pantheios::log_DEBUG("GPSH| clock_nanosleep error! Values: ", pantheios::integer(seconds), "/", pantheios::integer(nanoseconds));
		pantheios::log_DEBUG("GPSH| Leaving threadSleep()");
		return -1;
	}
	pantheios::log_DEBUG("GPSH| Leaving threadSleep()");
	return 0;
}

/**
 * Will set sleep time. The thread will sleep this time between every run.
 * @param seconds		sets the sleep time variable for seconds
 * @param nanoseconds	sets the sleep time variable for nanoseconds
 */
int32_t GpsHelper::set_sleepTime(uint32_t& seconds, uint32_t& nanoseconds){
	if (seconds < 0 || nanoseconds >= 1000000000){
		pantheios::log_DEBUG("GPSH Malicious values in setSleepTime: ", pantheios::integer(seconds), "/", pantheios::integer(nanoseconds));
		return -1;
	}
	nSleepSec_  = seconds;
	nSleepNsec_ = nanoseconds;
	return 0;
}
