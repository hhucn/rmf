//=================================================
// Name        : gpsdThreadbase.cpp
// Author      : Tobias Krauthoff
// Version     : 1.0
// Description : GPSD Wrapper
//=================================================

#include "gpsdThreadbase.h"

pthread_mutex_t gpsd_readDdata_mutex	= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gpsd_delieverData_mutex	= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  gpsd_readData_cond		= PTHREAD_COND_INITIALIZER;
pthread_cond_t  gpsd_delieverData_cond	= PTHREAD_COND_INITIALIZER;

// TODO
// check log_NOTICE and log_DEBUG

/* --------------------------------------------------------------------------------
 * C O N S T R U C T O R   &   D E S T R U C T O R
 * -------------------------------------------------------------------------------- */

/**
 * Default constructor, will initialize all values to default
 */
gpsdThreadbase::gpsdThreadbase() :	mGpsdAquisitionThreadbaseThread(NULL){
	pantheios::log_NOTICE("GPSD| Creating gpsd thread with default parameters (logwriting=true)");
	initValues(true);
	pantheios::log_DEBUG("GPSD| Created gpsd thread");
}

/**
 * Default constructor, will initialize all values to default, but not debug and writeToFile flag
 *
 * @param debug, boolean for debugging messages
 * @param writeToFile, boolean for writing logfile
 */
gpsdThreadbase::gpsdThreadbase(bool writeToFile) :	mGpsdAquisitionThreadbaseThread(NULL){
	pantheios::log_NOTICE("GPSD| Creating gpsd thread with custom parameters (logwriting=",
			(writeToFile ? "true" : "false"), ")");
	initValues(writeToFile);
	pantheios::log_DEBUG("GPSD| Created gpsd thread");
}

/**
 * Default destructor
 */
gpsdThreadbase::~gpsdThreadbase() {
	pantheios::log_DEBUG("GPSD| Before destructor in gpsdThreadbase");
	stop();
	join();
	pantheios::log_DEBUG("GPSD| After destructor in gpsdThreadbase");
}

/* --------------------------------------------------------------------------------
 * A Q U I S I T I O N   &   C O N S U M I N G
 * -------------------------------------------------------------------------------- */

/**
 * Initialising all values, setting default parameters
 * @param debug, should be true while debugging
 * @param writeToFile, should be true for writing logfiles
 */
void gpsdThreadbase::initValues(bool writeToFile) {
	lastLogTimeStart = "";
	lastLoggedTime = "";
	logfile = "";
	gpsdSerialPort = "/dev/ttyS0";
	gpsdTcpPort = DEFAULT_GPSD_PORT;
	timing.setTimingMethod(input.get_timingMethod());

	killFlag		= false;
	writeFileFlag	= writeToFile;
	runningFlag		= false;
	set_writeFileFlag(writeToFile);
}

/* --------------------------------------------------------------------------------
 * T H R E A D   -   M E T H O D S
 * -------------------------------------------------------------------------------- */

/**
 * First the daemon-thread will be started. After this, the main thread with data aquisition and consumption
 */
int gpsdThreadbase::start() {
	// thread attributes
	int schedMethod = input.get_processScheduling();
	pthread_attr_t threadAttribute;
	if( geteuid() == 0 ){ // only root can to this
		if( schedMethod == 1 ){	// round robin
			if( pthread_attr_setschedpolicy(&threadAttribute, SCHED_RR) != 0 )
				pantheios::log_ERROR("MAIN| -> Setting gpsd PThread CPU scheduling policy to SCHED_RR: FAILED");
			else
				pantheios::log_DEBUG("MAIN| -> Setting gpsd PThread CPU scheduling policy to SCHED_RR: SUCCESSED");
		} else if( schedMethod == 2 ){	// fifo
			if( pthread_attr_setschedpolicy(&threadAttribute, SCHED_FIFO) != 0 )
				pantheios::log_ERROR("MAIN| -> Setting gpsd PThread scheduling policy to SCHED_FIFO: FAILED");
			else
				pantheios::log_DEBUG("MAIN| -> Setting gpsd PThread scheduling policy to SCHED_FIFO: SUCCESSED");
		} else if( schedMethod == 0 ){	// default Scheduler
			// 20 to -20, smaller is better
			//http://www.kernel.org/doc/man-pages/online/pages/man2/setpriority.2.html
			if( pthread_attr_setschedpolicy(&threadAttribute, SCHED_OTHER) != 0 )
				pantheios::log_ERROR("MAIN| -> Setting gpsd PThread scheduling policy to SCHED_OTHER: FAILED");
			else
				pantheios::log_DEBUG("MAIN| -> Setting gpsd PThread scheduling policy to SCHED_OTHER: SUCCESSED");
		}
	} else if( schedMethod > 0 ){
		pantheios::log_ERROR("MAIN| -> Missing root privileges to set gpsd scheduling policy.");
	}


	// starting main gpsd thread
	if (mGpsdAquisitionThreadbaseThread == NULL) {
		mGpsdAquisitionThreadbaseThread = new pthread_t;
		int rd = pthread_create(mGpsdAquisitionThreadbaseThread, &threadAttribute, gpsdAquisitionThreadbase_dispatcher, this);
		if (rd != 0) {
			pantheios::log_ERROR("GPSD| Main threadbase pthread_create failed");
			return -1;
		} else {
			pantheios::log_DEBUG("GPSD| Main threadbase pthread_create succeeded");
			return 0;
		}
	} else {
		pantheios::log_ERROR("GPSD| Multiple main threadbase start");
		return -1;
	}
}

/**
 * Will join the threads
 */
void gpsdThreadbase::join() {
	// joining main thread
	if (mGpsdAquisitionThreadbaseThread) {
		int rc = pthread_join(*mGpsdAquisitionThreadbaseThread, NULL);
		if (rc != 0) {
			pantheios::log_ERROR("GPSD| main threadbase pthread_join failed");
		}
		delete mGpsdAquisitionThreadbaseThread;
		mGpsdAquisitionThreadbaseThread = NULL;
	}
}

/* --------------------------------------------------------------------------------
 * S T A R T I N G   &   K I L L I N G
 * -------------------------------------------------------------------------------- */

/**
 * Will start the gpsd daemon as sudo, when possible
 * @return 0 on success, -1 when an error occured
 */
int gpsdThreadbase::startGpsd() {
	pantheios::log_NOTICE("GPSD| Begin starting gpsd daemon.");
	int retval = 0;

	killGpsd();

	if (geteuid() == 0) { // only root can do this

		string open = "sudo gpsd -n " + gpsdSerialPort;
		pantheios::log_NOTICE("GPSD| Executing: ", open);
		pipeGpsd = popen(open.c_str(), "r");

		if ( !pipeGpsd ){
			// could not open process
			pantheios::log_NOTICE("GPSD| Could not open process for starting gpsd daemons.");
		} else {
			checkProcessForError(pipeGpsd);

			timing.probeTime();
			timing.sleepUntil(timing.lastProbedTime.seconds, timing.lastProbedTime.nanoseconds + SECOND_IN_NS);
			sched_yield();
		}

//		pclose(pipe);
	} else {
		pantheios::log_ERROR("GPSD| -> Missing root privileges to start gpsd daemon.");
		retval = -1;
	}

	runningFlag = (retval == 0);
	killFlag = retval != 0;
	return retval;
}

/**
 * Will kill the gpsd daemon as sudo, when possible
 * @return 0 on success, -1 when an error occured
 */
int gpsdThreadbase::killGpsd() {
	pantheios::log_DEBUG("GPSD| Before killing gpsd daemon.");
	int retval = 0;

	if (geteuid() == 0) { // only root can do this

		// open process for killing old gpsd daemons
		char killGpsd[] = "sudo killall gpsd";
		FILE* pipe = popen(killGpsd, "r");
		if ( !pipe ){
			// could not open process
			pantheios::log_NOTICE("GPSD| Could not open process for killing old gpsd daemon.");
		} else {
			char buffer[128];
			string result = "";

			// getting all information out of the pipe
			while( !feof(pipe) ) {
				if( fgets(buffer, 128, pipe ) != NULL)
					result += buffer;
			}

			// is the pipe was empty, than everythink worked fine
			if ( result.empty() ){
				pantheios::log_NOTICE("GPSD| Old Daemons killed, before running new one.");
			} else {
				pantheios::log_NOTICE("GPSD| Killing old daemons caused problems: ", result);
			}
		}
		pclose(pipe);
	}

	runningFlag = (retval == -1);

	pantheios::log_DEBUG("GPSD| After killing gpsd daemon.");
	return retval;
}

/* --------------------------------------------------------------------------------
 * A Q U I S I T I O N   &   C O N S U M I N G
 * -------------------------------------------------------------------------------- */

/**
 * Aquisitates all gpsd data, which is receives as json stream. Every Second the data will be consumed
 */
void gpsdThreadbase::aquisitionRun() {
	pantheios::log_DEBUG("GPSD| Entering aquisitionRun()");

	// do we have got root privileges?
	if (geteuid() != 0) { // only root can do this
		pantheios::log_ERROR("GPSD| -> Missing root privileges to start gpsd daemon.");
		stop();
		return;
	}

	// kill old daemons
	killGpsd();

	// start new daemoin
	string open = "sudo gpsd -n " + gpsdSerialPort;
	pantheios::log_NOTICE("GPSD| Executing: ", open);
	FILE* pipe = popen(open.c_str(), "r");

	if ( !pipe ){ // could not open process
		pantheios::log_NOTICE("GPSD| Could not open process for starting gpsd daemons.");
	} else {

		// any error while starting the process?
		if (checkProcessForError(pipe) == -1) return;

		// starting gps receiver
		int attempt;
		gpsmm gps_receiver("localhost", gpsdTcpPort.c_str());

		// wait for gps stream
		for (attempt = 1; attempt <= 5 && !killFlag
			&& (gps_receiver.stream(WATCH_ENABLE | WATCH_NEWSTYLE | WATCH_JSON | WATCH_SCALED | WATCH_TIMING) == NULL); attempt++) {
			pantheios::log_ERROR("GPSD| No gpsd is running, retry to connect in ", pantheios::integer(RETRY_TIME_FACTOR),
					" seconds, attempt ", pantheios::integer(attempt), "/5");

			timing.probeTime();
			timing.sleepUntil(timing.lastProbedTime.seconds + RETRY_TIME_FACTOR, timing.lastProbedTime.nanoseconds);
			continue;// I will try to connect to gpsd again
		}

		// did we need to many attempts, so that there is probably no gps daemon?
		if (attempt == 6) {
			pantheios::log_DEBUG("GPSD| No gpsd is running after ", pantheios::integer(attempt) , " attempts -> gpsd thread will quit ");
			stop();
			return;
		}

		const char* buffer = NULL;

		// consume stream
		while (!killFlag) {
			struct gps_data_t* newdata;

			/*
			 Blocking check to see if data from the daemon is waiting. Named
			 something like "waiting()" and taking a wait timeout as argument.
			 Note that choosing a wait timeout of less than twice the cycle
			 time of your device will be hazardous, as the receiver will
			 probably not supply input often enough to prevent a spurious
			 error indication. For the typical 1-second cycle time of GPSes
			 this implies a minimum 2-second timeout.
			 */
			// TODO CHECK THIS
			if (!gps_receiver.waiting((double) WAITING_TIME_FACTOR * (double) SECOND_IN_NS)) continue;

			if (checkProcessForError(pipe) == -1) break;


			if ((newdata = gps_receiver.read()) == NULL && !killFlag) {
				pantheios::log_ERROR("GPSD| read error");
				return;
			} else {
				int rc;
				cout << "1a"<<endl;

				rc = pthread_mutex_lock(&gpsd_readDdata_mutex);
				if (rc != 0)	pantheios::log_ERROR("GPSD| Error on locking read_gpsddata_mutex");
				cout << "2a"<<endl;

				rc = pthread_cond_wait(&gpsd_readData_cond, &gpsd_readDdata_mutex);
				if (rc != 0){
					pantheios::log_ERROR("GPSD| Error waiting for gpsd_readData_cond");
				} else {
					pantheios::log_NOTICE("GPSD| Condition variable was signaled and gpsd data will be consumed.");
				}
				buffer = gps_receiver.data();
				gpsDataConsuming(buffer);
				cout << "3a"<<endl;

				rc = pthread_cond_signal(&gpsd_delieverData_cond);
				if (rc != 0)	pantheios::log_ERROR("GPSD| Error on signaling for gpsd_delieverData_cond");

				rc = pthread_mutex_unlock(&gpsd_readDdata_mutex);
				if (rc != 0)	pantheios::log_ERROR("GPSD| Error on unlocking read_gpsddata_mutex");
				cout << "4a"<<endl;
			}
		}
	}

	pantheios::log_DEBUG("GPSD| Closing gpsd process.");
	pclose(pipe);

	pantheios::log_DEBUG("GPSD| Leaving aquisitionRun()");
}

int gpsdThreadbase::checkProcessForError(FILE* pipe){
	char buff[128];
	string result = "";
	// getting all information out of the pipe, while killflag is not set
	while( !feof(pipe) )
		if( fgets(buff, 128, pipe ) != NULL)
			result += buff;

	pantheios::log_DEBUG("GPSD| checkProcessForError -> pipe: \"", result, "\"");

	// is the pipe was empty, than everything worked fine
	if ( result.find("ERROR") != string::npos ){
		pantheios::log_NOTICE("GPSD| check process: device has an error on " , gpsdSerialPort);
		return -1; // running and kill flag will be set in emergency stop later
	} else {
		pantheios::log_DEBUG("GPSD| checkProcess: device is running on " , gpsdSerialPort);
		runningFlag = true;
		killFlag = false;
		return 0;
	}
}

/**
 * Will consumes incoming gpsd data, if it contains a tpv-type.
 * Information will be parsed and maybe write into a logfile
 * @param buffer, const char* which should be parsed
 */
void gpsdThreadbase::gpsDataConsuming(const char* buffer) {
	pantheios::log_DEBUG("GPSD| Entering gpsDataConsuming()");

	pantheios::log_DEBUG("GPSD| buffer:   ", buffer);

	string data(buffer);

	// we only want TPV data
	if (data.find("TPV") != string::npos) {

		// replace some characters
		data.replace(data.find("{"), 1, "");
		data.replace(data.find("}"), 1, "");
		while (data.find("\"") != string::npos) {
			data.replace(data.find("\""), 1, "");
		}

		// new gps date
		gpsDate gpsdate;
		gpsdate.set_valid(true);

		// variables for string manipulation
		string pattern = "";
		string retval = "";

		pantheios::log_DEBUG("GPSD| input:   ", data);

		// keywords from http://www.catb.org/gpsd/gpsd_json.html
		pattern = "mode";	if (searchParameter(pattern, data, retval) == 0) gpsdate.set_mode(atoi(retval.c_str()));
		pattern = "time";	if (searchParameter(pattern, data, retval) == 0) gpsdate.set_time(retval);
		pattern = "lat";	if (searchParameter(pattern, data, retval) == 0) gpsdate.set_latitude(strToDouble(retval));
		pattern = "lon";	if (searchParameter(pattern, data, retval) == 0) gpsdate.set_longitude(strToDouble(retval));
		pattern = "alt";	if (searchParameter(pattern, data, retval) == 0) gpsdate.set_altitude(strToDouble(retval));
		pattern = "ept";	if (searchParameter(pattern, data, retval) == 0) gpsdate.set_timeError(atof(retval.c_str()));
		pattern = "epx";	if (searchParameter(pattern, data, retval) == 0) gpsdate.set_latitudeError(strToDouble(retval));
		pattern = "epy";	if (searchParameter(pattern, data, retval) == 0) gpsdate.set_longitudeError(strToDouble(retval));
		pattern = "epv";	if (searchParameter(pattern, data, retval) == 0) gpsdate.set_altitudeError(strToDouble(retval));
		pattern = "track";	if (searchParameter(pattern, data, retval) == 0) gpsdate.set_track(strToDouble(retval));
		pattern = "speed";	if (searchParameter(pattern, data, retval) == 0) gpsdate.set_speed(strToDouble(retval));
		pattern = "eps";	if (searchParameter(pattern, data, retval) == 0) gpsdate.set_speedError(strToDouble(retval));

		currentGpsdate = gpsdate;

		/*
		// mode: 2d or 3d
		pos = data.find("mode");
		if (pos != string::npos) {
			endpos = data.find(",", pos);
			substr = data.substr(pos, endpos - pos);
			substr.erase(0, 5);
			gpsdate.set_mode(atoi(substr.c_str()));
			data.erase(pos, endpos - pos + 1);
		}

		// time
		pos = data.find("time");
		if (pos != string::npos) {
			endpos = data.find(",", pos);
			substr = data.substr(pos, endpos - pos);
			substr.erase(0, 5);
			gpsdate.set_time(substr);
			data.erase(pos, endpos - pos + 1);
		}

		// latitude
		pos = data.find("lat");
		if (pos != string::npos) {
			endpos = data.find(",", pos);
			substr = data.substr(pos, endpos - pos);
			substr.erase(0, 4);
			gpsdate.set_latitude(strToDouble(substr));
			data.erase(pos, endpos - pos + 1);
		}

		// longitude
		pos = data.find("lon");
		if (pos != string::npos) {
			endpos = data.find(",", pos);
			substr = data.substr(pos, endpos - pos);
			substr.erase(0, 4);
			gpsdate.set_longitude(strToDouble(substr));
			data.erase(pos, endpos - pos + 1);
		}

		// altitude
		pos = data.find("alt");
		if (pos != string::npos) {
			endpos = data.find(",", pos);
			substr = data.substr(pos, endpos - pos);
			substr.erase(0, 4);
			gpsdate.set_altitude(strToDouble(substr));
			data.erase(pos, endpos - pos + 1);
		}

		// error in time
		pos = data.find("ept");
		if (pos != string::npos) {
			endpos = data.find(",", pos);
			substr = data.substr(pos, endpos - pos);
			substr.erase(0, 4);
			gpsdate.set_timeError(atof(substr.c_str()));
			data.erase(pos, endpos - pos + 1);
		}

		// error in latitude
		pos = data.find("epx");
		if (pos != string::npos) {
			endpos = data.find(",", pos);
			substr = data.substr(pos, endpos - pos);
			substr.erase(0, 4);
			gpsdate.set_latitudeError(strToDouble(substr));
			data.erase(pos, endpos - pos + 1);
		}

		// error in longitude
		pos = data.find("epy");
		if (pos != string::npos) {
			endpos = data.find(",", pos);
			substr = data.substr(pos, endpos - pos);
			substr.erase(0, 4);
			gpsdate.set_longitudeError(strToDouble(substr));
			data.erase(pos, endpos - pos + 1);
		}

		// error in altitude
		pos = data.find("epv");
		if (pos != string::npos) {
			endpos = data.find(",", pos);
			substr = data.substr(pos, endpos - pos);
			substr.erase(0, 4);
			gpsdate.set_altitudeError(strToDouble(substr));
			data.erase(pos, endpos - pos + 1);
		}

		// track or direction
		pos = data.find("track");
		if (pos != string::npos) {
			endpos = data.find(",", pos);
			substr = data.substr(pos, endpos - pos);
			substr.erase(0, 6);
			gpsdate.set_track(strToDouble(substr));
			data.erase(pos, endpos - pos + 1);
		}

		// current speed
		pos = data.find("speed");
		if (pos != string::npos) {
			endpos = data.find(",", pos);
			substr = data.substr(pos, endpos - pos);
			substr.erase(0, 6);
			gpsdate.set_speed(strToDouble(substr));
			data.erase(pos, endpos - pos + 1);
		}

		// error in speed
		pos = data.find("eps");
		if (pos != string::npos) {
			endpos = data.find(",", pos);
			substr = data.substr(pos, endpos - pos);
			substr.erase(0, 4);
			gpsdate.set_speedError(strToDouble(substr));
			data.erase(pos, endpos - pos + 1);
		}
		 */


		if (writeFileFlag && (writeToFile(gpsdate) == -1)){
			pantheios::log_ERROR("GPSD| Error in writeToFile(gpsdate)");
		}
	}

	pantheios::log_DEBUG("GPSD| Leaving gpsDataConsuming()");
}

/* --------------------------------------------------------------------------------
 * H E L P E R   &   O T H E R  S T U F F
 * -------------------------------------------------------------------------------- */

/**
 * Send signal for condition variable and will consume gpsd data
 */
gpsDate gpsdThreadbase::sendSignalForCondVariableAndGetData(){
	pthread_cond_signal(&gpsd_readData_cond);
	gpsDate gd;

	int rc;
	cout << "1b"<<endl;
	rc = pthread_mutex_lock(&gpsd_delieverData_mutex);
	if (rc != 0)	pantheios::log_ERROR("GPSD| Error on locking gpsd_delieverData_mutex");

	cout << "2b"<<endl;
	rc = pthread_cond_wait(&gpsd_delieverData_cond, &gpsd_delieverData_mutex);
	if (rc != 0){
		pantheios::log_ERROR("GPSD| Error while waiting for gpsd_delieverData_cond");
		gd.set_valid(false);
	} else {
		gd = currentGpsdate;
	}

	cout << "3b"<<endl;
	rc = pthread_mutex_unlock(&gpsd_delieverData_mutex);
	if (rc != 0)	pantheios::log_ERROR("GPSD| Error on unlocking gpsd_delieverData_mutex");

	pantheios::log_NOTICE("GPSD| Condition variable was signaled and gpsd data will be delievered.");
	cout << "4b"<<endl;
	return gd;
}

/**
 * Searches the fix in the given input string.
 * Generates a substring from the position of the tag to the following ",".
 *
 * @param fix, parameter tag
 * @param input, parameter string
 * @param retval, string pointer to put generated substring in
 *
 * @return 0, on success (-1), an error occured
 */
inline int gpsdThreadbase::searchParameter(const string &fix, string &input, string &retval) {
	size_t startpos	= input.find(fix);
	if (startpos == string::npos) {
		return (-1);
	}

	size_t endpos		= input.find(",", startpos);
	retval = input.substr(startpos, endpos - startpos);
	retval.erase(0, fix.length() + 1);
	input.erase(startpos, endpos - startpos + 1);

	return (0);
}

/**
 * Converts given input string to double value
 * @param s, given string, which should be a double value
 * @return converted double or -1
 */
inline double gpsdThreadbase::strToDouble(string s) {
	istringstream i(s);
	double x;
	if (!(i >> x)) return -1;
	return x;
}

/**
 * Will kill the thread
 */
void gpsdThreadbase::stop() {
	pantheios::log_NOTICE("GPSD| gpsd is shutting down!");
	this->killFlag = true;
	this->runningFlag = false;
	pclose(pipeGpsd);
	outputFile.close();
//	join();
}

/**
 * Will set the writeToFileFlag, also a logfile will be created
 * @param flag, boolean
 */
void gpsdThreadbase::set_writeFileFlag(bool flag) {
	pantheios::log_DEBUG("GPSD| Entering set_writeFileFlag(", pantheios::boolean(flag) , ")");
	this->writeFileFlag = flag;
	if (flag) {

		// get current time
		char date[20];
		time_t now = time(NULL);
		if (strftime(date, 20, "%Y%m%d_%H%M%S", localtime(&now)) == 0) { 	// This should never happen...
			pantheios::log_ERROR("GPSD| Error occurred when the program tried to get the local date & time of the system. This is bad!");
		}
		lastLogTimeStart = string(date);

		// create logfile
		logfile = input.get_logfilePath() + (input.get_nodeRole() == 1 ? ".server_gpsdLog_" : ".client_gpsdLog_" + lastLogTimeStart);
		pantheios::log_DEBUG("GPSD| logfile = ", logfile ,")");
	} else {
		outputFile.close();
	}
	pantheios::log_DEBUG("GPSD| Leaving set_writeFileFlag(", pantheios::boolean(flag) , ")");
}

/**
 *
 * @param port, string
 */
void gpsdThreadbase::set_gpsdSerialPort(string port){
	this->gpsdSerialPort = port;
}

/**
 *
 * @param port, string
 */
void gpsdThreadbase::set_gpsdTcpPort(string port){
	this->gpsdTcpPort = port;
}

/**
 * Write gpsd data into the logfile, when the writeFileFlag is set and the lastLoggedTime is different to the gpsd time
 * @param gpsd, data which should be written
 * @return 0 on success, -1 when an error occured
 */
int gpsdThreadbase::writeToFile(gpsDate &gpsd) {
	pantheios::log_DEBUG("GPSD| Entering writeToFile()");

	int retval = 0;
	// check for flag and relevance
	if (writeFileFlag && lastLoggedTime.compare(gpsd.get_time()) != 0) {

		// is file open?
		if (!outputFile.is_open()){
			outputFile.open(logfile.c_str(), fstream::in | fstream::out | fstream::app);
			// check outputfile
			if (!outputFile.is_open() || !outputFile.good()) {
				pantheios::log_DEBUG("GPSD| couldn't open output file, so there will be no output");
				writeFileFlag = false;
				return -1;
			} else {
				pantheios::log_DEBUG("GPSD| Logfile is open, so there will be output");
			}
		}

		pantheios::log_DEBUG("GPSD| File will be written in '", logfile, "'");
		lastLoggedTime = gpsd.get_time();
		retval = gpsd.writeToFile(outputFile);
		gpsd.print();
	} else {
		pantheios::log_DEBUG("GPSD| File will not be written, cause lastLoggedTime == gpsd.get_time()");
		pantheios::log_DEBUG("GPSD| ", lastLoggedTime, " = " , gpsd.get_time());
		retval = -1;
	}

	pantheios::log_DEBUG("GPSD| Leaving writeToFile()");

	return retval;
}

/**
 * Returns last time, when data was loged
 * @return string
 */
string gpsdThreadbase::get_lastLogTimeStart() {
	return lastLogTimeStart;
}

/**
 * trivial
 * @return true, when gpsd daemon is running
 */
bool gpsdThreadbase::is_running(){
	return runningFlag;
}

/**
 * trivial
 * @return gpsdDate, which was the last measured gps date
 */
gpsDate gpsdThreadbase::get_lastGpsDate() {
	return currentGpsdate;
}

/*
 * Checks for an empty file. The file is empty, when there are no printable signs
 * @param local path to file
 * @returns true, when file is empty
 *
 bool gpsdThreadbase::is_textfilEmpty( string filename ) {
 std::string s;
 std::ifstream f(filename.c_str(), std::ios::binary);

 // Check for UTF-8 BOM
 if (f.peek() == 0xEF) {
 f.get();
 if (f.get() != 0xBB) return false;
 if (f.get() != 0xBF) return false;
 }

 // Scan every line of the file for non-whitespace characters
 while (getline(f, s)) {
 if (s.find_first_not_of(" \t\n\v\f\n" // whitespace
 "\0\xFE\xFF"// non-printing (used in various Unicode encodings)
 ) != std::string::npos) return false;
 }

 // If we get this far, then the file only contains whitespace
 // (or its size is zero)
 return true;
 }
 */

/*
 // mode flags for setting streaming policy
 // out of /usr/include/gpsh
 #define WATCH_ENABLE	0x000001u	// enable streaming
 #define WATCH_DISABLE	0x000002u	// disable watching
 #define WATCH_JSON		0x000010u	// JSON output
 #define WATCH_NMEA		0x000020u	// output in NMEA
 #define WATCH_RARE		0x000040u	// output of packets in hex
 #define WATCH_RAW		0x000080u	// output of raw packets
 #define WATCH_SCALED	0x000100u	// scale output to floats
 #define WATCH_TIMING	0x000200u	// timing information
 #define WATCH_DEVICE	0x000800u	// watch specific device
 #define WATCH_NEWSTYLE	0x010000u	// force JSON streaming
 #define WATCH_OLDSTYLE	0x020000u	// force old-style streaming
 */

