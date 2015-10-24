/**
 * @authors Tobias Krauthoff, Sebastian Wilken
 * @date 26. June 2014
 * @note when using, beware of structure used in all log files!!!
 * calculates and smoothen results
 */

#include "resultCalculator.h"


// External logging framework
#include "pantheios/pantheiosHeader.h"
#include <iomanip>

/**
 * Default calculator need biggest used chirpsize for detecting out-of-order pakets
 *
 * @param chirpsize			biggest chirpsize used
 * @param noderole			0 for client and 1 for server
 * @param countSmoothening	how many chirps should be taken together for smoothening? (must be >1, default 3)
 * @param logfilepath		path for logfile
 */
ResultCalculator::ResultCalculator(uint32_t chirpsize, uint32_t noderole, uint32_t countSmoothening, string logfilepath) {
	nChirpsize_ = chirpsize;
	logfilePath_ = logfilepath;
	nNodeRole_ = noderole;
	nCountSmoothening_ = countSmoothening;
	isCreateLossPlot_ = false;
	isCreateGapPlot_ = false;
	isCreateCongestionPlot_ = false;
	isCreateChirpAndPacketSizePlot_ = false;
}

/**
 * default destructor
 */
ResultCalculator::~ResultCalculator() {
}

/**
 * Creates the processed plot file and writes the gnuplot instructions
 * @return 0 on success, -1 otherwise
 */
int32_t ResultCalculator::createProcessedPlotFile() {
	pantheios::log_DEBUG("RECA| Entering createProcessedPlotFile()");
	fstream processedPlot;
	string processedPlotFile("");
	string processedDataFile("");

	if (nNodeRole_ == 1) processedDataFile = logfilePath_ + ".server_mReceiverProcessedData";
	else if (nNodeRole_ == 0) processedDataFile = logfilePath_ + ".client_mReceiverProcessedData";

	processedPlotFile = processedDataFile + ".plot";

	// Open processedPlot File
	processedPlot.open(processedPlotFile.c_str(), ios::out);
	if (!processedPlot.is_open()) {
		pantheios::log_ERROR("RECA| Error: Opening \"", processedPlotFile.c_str(), "\".");
		return (-1);
	}

	// Remove leading folders
	uint32_t position = processedDataFile.find_last_of('/');
	string plotFileName(processedDataFile.substr(position + 1, processedDataFile.length()));

	// Adjust gnuplot Options
	processedPlot << "set title \"" << plotFileName << "\" " << "\n";
	processedPlot << "set xlabel \"Chirp number (id)\"" << "\n";
	processedPlot << "set ylabel \"Data rate (kbyte/s)\"" << "\n";
	processedPlot << "set yrange [0:1300]" << "\n";
	processedPlot << "\n";
	processedPlot << "plot '" << plotFileName << "' using 1:6 title \"measured data rate (kbyte/s)\" with points lt 1";
	processedPlot << ", '" << plotFileName << "' using 1:7 title \"moving average (" << nCountSmoothening_ << ")\" with line lt 3" << "\n";

	processedPlot.close();

	pantheios::log_DEBUG("RECA| Leaving createProcessedPlotFile()");
	return 0;
}

/**
 * Creates the loss plot file and writes the gnuplot instructions
 * @return 0 on success, -1 otherwise
 */
int32_t ResultCalculator::createLossPlotFile() {
	pantheios::log_DEBUG("RECA| Entering createLossPlotFile()");
	fstream processedPlot;
	fstream lossPlot;
	string processedDataFile("");
	string lossPlotFile("");

	if (nNodeRole_ == 1) {
		processedDataFile = logfilePath_ + ".server_mReceiverProcessedData";
		lossPlotFile = logfilePath_ + ".server_mReceiverLossData.plot";
	} else if (nNodeRole_ == 0) {
		processedDataFile = logfilePath_ + ".client_mReceiverProcessedData";
		lossPlotFile = logfilePath_ + ".client_mReceiverLossData.plot";
	}

	// Open lossPlot File
	lossPlot.open(lossPlotFile.c_str(), ios::out);
	if (!lossPlot.is_open()) {
		pantheios::log_ERROR("RECA| Error: Opening \"", lossPlotFile.c_str(), "\".");
		return (-1);
	}

	// Gettin data and plot file name
	uint32_t position1 = processedDataFile.find_last_of('/');
	uint32_t position2 = lossPlotFile.find_last_of('/');
	string dataFileName(processedDataFile.substr(position1 + 1, processedDataFile.length()));
	string plotFileName(lossPlotFile.substr(position2 + 1, lossPlotFile.length() - 6 - position2)); // -6 for .plot

	// Adjust gnuplot Options
	lossPlot << "set title \"" << plotFileName << "\" " << "\n";
	lossPlot << "set xlabel \"Chirp number (id)\"" << "\n";
	lossPlot << "set yrange [0:100]" << "\n";
	lossPlot << "set ylabel \"Loss (percent)\"" << "\n";
	lossPlot << "set ytics nomirror" << "\n";
	lossPlot << "set y2range [0:1300]" << "\n";
	lossPlot << "set y2tics nomirror" << "\n";
	lossPlot << "set y2label \"Data rate (kbyte/s)\"" << "\n";
	lossPlot << "set key right center" << "\n";
	lossPlot << "\n";
	lossPlot << "plot '" << dataFileName << "' using 1:9 title \"loss in percent\" with linespoints lt 3,\\" << "\n";
	lossPlot << "'" << dataFileName << "' using 1:6 title \"moving average (" << nCountSmoothening_ << ")\" with linespoints lt 1 axes x1y2" << "\n";

	lossPlot.close();

	pantheios::log_DEBUG("RECA| Leaving createLossPlotFile()");
	return 0;
}

/**
 * Creates the chirp- and packetsize plot file and writes the gnuplot instructions
 * @return 0 on success, -1 otherwise
 */
int32_t ResultCalculator::createChirpAndPacketSizePlotFile() {
	pantheios::log_DEBUG("RECA| Entering createChirpAndPacketSizePlotFile()");
	fstream processedPlot;
	fstream chirpAndPacketSizePlot;
	string processedDataFile("");
	string chirpAndPacketSizePlotFile("");

	if (nNodeRole_ == 1) {
		processedDataFile = logfilePath_ + ".server_mReceiverProcessedData";
		chirpAndPacketSizePlotFile = logfilePath_ + ".server_mReceiverChirpAndPacketSizeData.plot";
	} else if (nNodeRole_ == 0) {
		processedDataFile = logfilePath_ + ".client_mReceiverProcessedData";
		chirpAndPacketSizePlotFile = logfilePath_ + ".client_mReceiverChirpAndPacketSizeData.plot";
	}

	// Open processedPlot File
	chirpAndPacketSizePlot.open(chirpAndPacketSizePlotFile.c_str(), ios::out);
	if (!chirpAndPacketSizePlot.is_open()) {
		pantheios::log_ERROR("RECA| Error: Opening \"", chirpAndPacketSizePlotFile.c_str(), "\".");
		return (-1);
	}

	// Gettin data and plot file name
	uint32_t position1 = processedDataFile.find_last_of('/');
	uint32_t position2 = chirpAndPacketSizePlotFile.find_last_of('/');
	string dataFileName(processedDataFile.substr(position1 + 1, processedDataFile.length()));
	string plotFileName(chirpAndPacketSizePlotFile.substr(position2 + 1, chirpAndPacketSizePlotFile.length() - 6 - position2)); // -6 for .plot

	// Adjust gnuplot Options
	chirpAndPacketSizePlot << "set title \"" << plotFileName << "\" " << "\n";
	chirpAndPacketSizePlot << "set xlabel \"Chirp number (id)\"" << "\n";
	chirpAndPacketSizePlot << "set ylabel \"Packet Size and Data Rate (kbytes)\"" << "\n";
	chirpAndPacketSizePlot << "set yrange [0:1500]" << "\n";
	chirpAndPacketSizePlot << "set ytics nomirror" << "\n";
	chirpAndPacketSizePlot << "set y2tics nomirror" << "\n";
	chirpAndPacketSizePlot << "set y2label \"Chirp Length\"" << "\n";
	chirpAndPacketSizePlot << "\n";
	chirpAndPacketSizePlot << "plot '" << dataFileName << "' using 1:3 title \"size of packets\" with step lt 1,\\\n";
	chirpAndPacketSizePlot << "'" << dataFileName << "' using 1:2 title \"length of chirp\" with points lt 3 axes x1y2,\\\n";
	chirpAndPacketSizePlot << "'" << dataFileName << "' using 1:7 title \"data rate(kbyte/s)\" with line lt 2" << "\n";

	chirpAndPacketSizePlot.close();

	pantheios::log_DEBUG("RECA| Leaving createChirpAndPacketSizePlotFile()");
	return 0;
}

/**
 * Creates the debug plot file and writes the gnuplot instructions
 * @return 0 on success, -1 otherwise
 */
int32_t ResultCalculator::createDebugPlotFile() {
	pantheios::log_DEBUG("RECA| Entering createDebugPlotFile()");
	fstream debugPlot;
	string debugPlotFile("");
	string debugDataFile("");

	// Open debugPlot File
	if (nNodeRole_ == 1) debugDataFile = logfilePath_ + ".server_mReceiverDebugData";
	else if (nNodeRole_ == 0) debugDataFile = logfilePath_ + ".client_mReceiverDebugData";
	debugPlotFile = debugDataFile + ".plot";

	// Open plot file
	debugPlot.open(debugPlotFile.c_str(), ios::out);
	if (!debugPlot.is_open()) {
		pantheios::log_ERROR("RECA| Error: Opening \"", debugPlotFile.c_str(), "\".");
		return (-1);
	}

	// Remove leading folders
	uint32_t position = debugDataFile.find_last_of('/');
	string fileNameD(debugDataFile.substr(position + 1, debugDataFile.length()));

	// Adjust gnuplot Options
	debugPlot << "set title \"" << fileNameD << "\" " << "\n";
	debugPlot << "set xlabel \"ID (received Packet)\"" << "\n";
	debugPlot << "set ylabel \"Time (s)\"" << "\n";
	debugPlot << "set ytics nomirror" << "\n";
	debugPlot << "set y2tics nomirror" << "\n";
	debugPlot << "set y2label \"Latency(s)\"" << "\n";
	debugPlot << "set y2range [0:5]" << "\n";
	debugPlot << "\n";
	debugPlot << "plot '" << fileNameD << "' using 1:2 title \"Send time (sender clock)\" with linespoints,\\\n";
	debugPlot << "'" << fileNameD << "' using 1:3 title \"Receive time (receivers clock)\" with linespoints,\\\n";
	debugPlot << "'" << fileNameD << "' using 1:6 title \"Latency\" with linespoints axes x1y2\\\n"  << "\n";

	debugPlot.close();

	pantheios::log_DEBUG("RECA| Leaving createDebugPlotFile()");
	return 0;
}

/**
 * Creates the debug data file, sets the precision
 * and writes the headline
 *
 * @param debug_data, pointer to the stream object
 * @return -1 on error, 0 on success
 */
int32_t ResultCalculator::createDebugDataFile(fstream &debugData) {
	pantheios::log_DEBUG("RECA| Entering CreateDebugDataFile()");
	// determine filename, create and open file
	string debugDataFileName;

	if (nNodeRole_ == 0) {  // Client
		debugDataFileName = logfilePath_ + ".client_mReceiverDebugData";
	} else if (nNodeRole_ == 1) {   // Server
		debugDataFileName = logfilePath_ + ".server_mReceiverDebugData";
	} else {  // wtf?
		pantheios::log_ERROR("RECA| ERROR: Unknown role value: ", pantheios::integer(nNodeRole_));
		return -1;  // unknown node role
	}

	debugData.open(debugDataFileName.c_str(), ios::out);
	if (!debugData.is_open()) {
		pantheios::log_ERROR("RECA| ERROR: Opening \"", debugDataFileName, "\".");
		return -1;  // open file failed
	}

	// set options and write headline
	debugData.setf(ios::fixed);
	debugData << setprecision(10);
	debugData << "Count_of_received_packets\t"
			  << "Sender_Timestamp(s)\t"
			  << "Receiver_Timestamp(s)\t"
			  << "Sender_Latency(s)\t"
			  << "Receiver_Latency(s)\t"
			  << "Latency(s)" << "\n";
	pantheios::log_DEBUG("RECA| Leaving CreateProcessedDataFile()");
	return 0;
}

/**
 * Creates a plotfile for all gaps between received packets
 *
 * @return 0 when log could be written, -1 otherwise
 */
int32_t ResultCalculator::creatGapPlotFile(){
	pantheios::log_DEBUG("SICD| Entering creatGapPlotFile()");
	ofstream gapPlot;
	string gapPlotFile = logfilePath_ + (nNodeRole_ == 0 ? ".client" : ".server") + "_mReceiverGaps.plot";
	string gapDataFile = logfilePath_ + (nNodeRole_ == 0 ? ".client" : ".server") + "_mReceiverRawData";

	// Open plot file
	gapPlot.open(gapPlotFile.c_str(), ios::out | ios::app);
	if (!gapPlot.is_open()) {
		pantheios::log_ERROR("RECA| Error: Opening \"", gapPlotFile.c_str(), "\".");
		return (-1);
	}

	// Remove leading folders for getting data source name
	uint32_t position = gapDataFile.find_last_of('/');
	string fileNameD = gapDataFile.substr(position + 1, gapDataFile.length());

	// Adjust gnuplot Options
	gapPlot << "set title \"" << fileNameD << "\" " << "\n";
	gapPlot << "set xlabel \"ID (received Packet)\"" << "\n";
	gapPlot << "set ylabel \"Time (s)\"" << "\n";
	gapPlot << "\n";
	gapPlot << "plot '" << fileNameD << "' using 8 title \"Gap between packet i and i-1\" with pt 7 ps 10  " << "\n";

	gapPlot.close();

	pantheios::log_DEBUG("SICD| Entering createLogFile()");
	return 0;
}

/**
 * Creates a plotfile for all congestion times
 *
 * @return 0 when log could be written, -1 otherwise
 */
int32_t ResultCalculator::createCongestionPlotFile(){
	pantheios::log_DEBUG("SICD| Entering creatCongestionPlotFile()");
	ofstream gapPlot;
	string gapPlotFile = logfilePath_ + (nNodeRole_ == 0 ? ".client" : ".server") + "_mCongestionData.plot";
	string gapDataFile = logfilePath_ + (nNodeRole_ == 0 ? ".client" : ".server") + "_mSafeguardReceiverData";

	// Open plot file
	gapPlot.open(gapPlotFile.c_str(), ios::out | ios::app);
	if (!gapPlot.is_open()) {
		pantheios::log_ERROR("RECA| Error: Opening \"", gapPlotFile.c_str(), "\".");
		return (-1);
	}

	// Remove leading folders for getting data source name
	uint32_t position = gapDataFile.find_last_of('/');
	string fileNameD = gapDataFile.substr(position + 1, gapDataFile.length());

	// Adjust gnuplot Options
	gapPlot << "set title \"" << fileNameD << "\" " << "\n";
	gapPlot << "set xlabel \"Based on ID\"" << "\n";
	gapPlot << "set ylabel \"Congestion Time\"" << "\n";
	gapPlot << "\n";
	gapPlot << "plot '" << fileNameD << "' using 1:12 title \"Congestion Time\" with linespoints" << "\n";

	gapPlot.close();

	pantheios::log_DEBUG("SICD| Entering creatCongestionPlotFile()");
	return 0;
}

/**
 * Creates the processed data file, sets the precision
 * and writes the headline
 *
 * @param processed_data, pointer to the stream object
 * @return -1 on error, 0 on success
 */
int32_t ResultCalculator::createProcessedDataFile(fstream &processedData) {
	pantheios::log_DEBUG("RECA| Entering CreateProcessedDataFile()");
	// determine filename, create and open the file
	string processedDataFileName;

	if (nNodeRole_ == 0) {  // Client
		processedDataFileName = logfilePath_ + ".client_mReceiverProcessedData";
	} else if (nNodeRole_ == 1) {   // Server
		processedDataFileName = logfilePath_ + ".server_mReceiverProcessedData";
	} else {
		pantheios::log_ERROR("RECA| ERROR: Unknown role value.");
		return -1;  // unknown node role
	}

	processedData.open(processedDataFileName.c_str(), ios::out);
	if (!processedData.is_open()) {
		pantheios::log_ERROR("RECA| ERROR: Opening \"", processedDataFileName, "\".");
		return -1;  // open file failed
	}

	// set options and write headline
	processedData.setf(ios::fixed);
	processedData << setprecision(10);
	processedData << "Chirpnumber\t"
				  << "Packetcount\t"
				  << "SizeOfChirp(byte)\t"
				  << "Chirp_Transmit_Time(s)\t"
				  << "SizeOfPacket(byte)"
				  << "\tDataRate(kbyte/s)\t"
				  << "MovingAverage(k)\t"
				  << "Latency_of_first_received_packet(s)\t"
				  << "loss_in_chirp\t"
				  << "Receive_of_first_received_packet(s)\t"
				  << "Receive_of_first_received_packet(ns)\t"
				  << "\n";
	pantheios::log_DEBUG("RECA| Leaving CreateProcessedDataFile()");
	return 0;
}

/**
 * Opens the raw data file for reading
 *
 * @param raw_data, pointer to the stream object
 * @return -1 on error, 0 on success
 */
int32_t ResultCalculator::openRawDataFile(fstream &rawData) {
	pantheios::log_DEBUG("RECA| Entering OpenRawDataFile()");
	string rawDataFileName;

	if (nNodeRole_ == 0) {  // Client
		rawDataFileName = logfilePath_ + ".client_mReceiverRawData";
	} else if (nNodeRole_ == 1) {  // Server
		rawDataFileName = logfilePath_ + ".server_mReceiverRawData";
	} else {
		pantheios::log_ERROR("RECA| ERROR: Unknown role value.");
		return -1;  // unknown node role
	}

	rawData.open(rawDataFileName.c_str(), ios::in);
	if (!rawData.is_open()) {
		pantheios::log_ERROR("RECA| ERROR: Opening \"", rawDataFileName, "\".");
		return -1;  // open file failed
	}

	pantheios::log_DEBUG("RECA| Leaving OpenRawDataFile()");
	return 0;
}

/**
 * Complex process: Will calculate the final results
 * @return 0
 */
int32_t ResultCalculator::calculateResults() {
	pantheios::log_DEBUG("RECA| Entering calculateResults()");

	fstream rawData, processedData, debugData;

	if (openRawDataFile(rawData)) return -1;
	if (createProcessedDataFile(processedData)) return -1;
	if (createDebugDataFile(debugData)) return -1;

	double moving_average[nCountSmoothening_];
	for (uint32_t i = 0; i < nCountSmoothening_; i++) {
		moving_average[i] = 0.0;
	}

	double moving_average_result = 0.0;
	uint32_t moving_average_counter = 0;
	double first_sender_timestamp = 0;
	double first_sender_seconds = 0;
	double first_sender_nanoseconds = 0;
	double first_receiver_timestamp = 0;
	double first_receiver_seconds = 0;
	double first_receiver_nanoseconds = 0;
	double first_sender_timestamp_of_chirp = 10000.0;
	double first_packet_latency = 0;
	uint32_t highestFoundChirpNumber = 0;
	uint32_t chirpToProcess = 1;
	uint32_t packetsize = 1;
	uint32_t loss_in_chirp = 0;
	int startpos_for_nextchirp = 0;

	cout << "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n" ;
	cout << "#chirp\t#packet\tps[B]\tcs[B]\t\tA[kB/s]\t\tloss[%]\n";

	do { // while (chirpToProcess <= highestFoundChirpNumber);
		double calculatedDataRate = 0.0;
		double chirp_send_duration = 0.0;
		uint32_t countOfPackets = 0;
		uint32_t chirpSizeInByte = 0;
		uint32_t lowestRecTime_Seconds = 4294967295u;
		uint32_t highestRecTime_Seconds = 0;
		uint32_t lowestRecTime_NanoSeconds = 4294967295u;
		uint32_t highestRecTime_NanoSeconds = 0;
		uint32_t lastfoundPacket_linenumber_of_chirp = 1;
		bool found_packet_of_chirp = false;

		// Prepare field separation detection
		size_t field1_start, field2_start, field3_start, field4_start, field5_start, field6_start, field7_start, field8_start, field9_start,
				field10_start, field11_start, field12_start;
		size_t field1_len, field2_len, field3_len, field4_len, field5_len, field6_len, field7_len, field11_len;
		//field8_len, field9_len, field10_len,
		string separator_Symbol = "\t";

		// Remove HeaderLine if we start from beginning of the file
		char buffer[5000];
		if (startpos_for_nextchirp == 0) rawData.getline(buffer, 5000);
		else {
			rawData.seekg(startpos_for_nextchirp, ios_base::beg);
			startpos_for_nextchirp = 0;
		}

		uint32_t linecounter = 0;

		double last_sender_time, last_receiver_time;

		while (rawData.good()) { //  while (chirpToProcess <= highestFoundChirpNumber);
			int32_t pos_nextchirp = rawData.tellg();
			rawData.getline(buffer, 5000);
			linecounter++;
			string readLine = (string) buffer;

			if (readLine.compare("") != 0) {
				// Find starting position of fields
				field1_start = 0;													// Chirp Number
				field2_start = readLine.find(separator_Symbol, field1_start) + 1;	// Packet Number
				field3_start = readLine.find(separator_Symbol, field2_start) + 1;	// Sendtime Seconds
				field4_start = readLine.find(separator_Symbol, field3_start) + 1;	// Sendtime NanoSeconds
				field5_start = readLine.find(separator_Symbol, field4_start) + 1;	// Receivetime Seconds
				field6_start = readLine.find(separator_Symbol, field5_start) + 1;	// Receivetime NanoSeconds
				field7_start = readLine.find(separator_Symbol, field6_start) + 1;	// Packetsize
				field8_start = readLine.find(separator_Symbol, field7_start) + 1;	// gap time
				field9_start = readLine.find(separator_Symbol, field8_start) + 1;	// Backchannel chirp length
				field10_start = readLine.find(separator_Symbol, field9_start) + 1;	// Backchannel data rate
				field11_start = readLine.find(separator_Symbol, field10_start) + 1;	// Fast Data Calc loss rate
				field12_start = readLine.find(separator_Symbol, field11_start) + 1;	// (may not be present,but a "\t" will be at the end)

				// Find lenght of fields
				field1_len = (field2_start - 1) - field1_start;
				field2_len = (field3_start - 1) - field2_start;
				field3_len = (field4_start - 1) - field3_start;
				field4_len = (field5_start - 1) - field4_start;
				field5_len = (field6_start - 1) - field5_start;
				field6_len = (field7_start - 1) - field6_start;
				field7_len = (field8_start - 1) - field7_start;
				//field8_len  = (field9_start - 1) - field8_start;
				//field9_len  = (field10_start - 1) - field9_start;
				//field10_len = (field11_start - 1) - field10_start;
				field11_len = (field12_start - 1) - field11_start;

				// Get substrings
				stringstream ss_chirp(readLine.substr(field1_start, field1_len));
				stringstream ss_packet(readLine.substr(field2_start, field2_len));
				stringstream ss_sender_seconds(readLine.substr(field3_start, field3_len));
				stringstream ss_sender_nanoseconds(readLine.substr(field4_start, field4_len));
				stringstream ss_receiver_seconds(readLine.substr(field5_start, field5_len));
				stringstream ss_receiver_nanoseconds(readLine.substr(field6_start, field6_len));
				stringstream ss_packetsize(readLine.substr(field7_start, field7_len));
				//stringstream ss_gap(readLine.substr(field8_start, field8_len));
				//stringstream ss_backchannel_chirpnr(readLine.substr(field9_start, field9_len));
				//stringstream ss_backchannel_datarate(readLine.substr(field10_start, field10_len));
				stringstream ss_fastdata_loss(readLine.substr(field11_start, field11_len));

				// Transform String Streams into correct datatype
				uint32_t chirp, packet, receiver_seconds, receiver_nanoseconds, sender_seconds, sender_nanoseconds;
				ss_chirp >> chirp;
				ss_packet >> packet;
				ss_sender_seconds >> sender_seconds;
				ss_sender_nanoseconds >> sender_nanoseconds;
				ss_receiver_seconds >> receiver_seconds;
				ss_receiver_nanoseconds >> receiver_nanoseconds;
				// FIXME Krauthoff This hast to be set, when the chirp itself is read, because packetsize
				// is variable (otherwise all chirps has got the packetsize of last chirp
				if (chirp == chirpToProcess) {
					ss_packetsize >> packetsize;
					ss_fastdata_loss >> loss_in_chirp;
				}

				double sender_time, receiver_time;

				// Write to debugfile, only needed once since we read the file multiple times
				if (chirpToProcess == 1) {
					if (linecounter == 1) {
						first_sender_seconds = (double) sender_seconds;
						first_sender_nanoseconds = (double) sender_nanoseconds;
						first_sender_timestamp = first_sender_seconds + (first_sender_nanoseconds / 1000000000.0);

						first_receiver_seconds = (double) receiver_seconds;
						first_receiver_nanoseconds = (double) receiver_nanoseconds;
						first_receiver_timestamp = first_receiver_seconds + (first_receiver_nanoseconds / 1000000000.0);

						last_sender_time = 0.0;
						last_receiver_time = 0.0;
					}
					sender_time = (double) sender_seconds + (double) (sender_nanoseconds / 1000000000.0) - first_sender_timestamp;
					receiver_time = (double) receiver_seconds + (double) (receiver_nanoseconds / 1000000000.0) - first_receiver_timestamp;

					//FIXME wrong
					debugData << linecounter
							<< "\t" << sender_time
							<< "\t" << receiver_time
							<< "\t"	<< (sender_time - last_sender_time)
							<< "\t" << (receiver_time - last_receiver_time)
							<< "\t" << (receiver_time - sender_time) << "\n";

					last_sender_time = sender_time;
					last_receiver_time = receiver_time;

					found_packet_of_chirp = true;
					lastfoundPacket_linenumber_of_chirp = linecounter;
				} else {
					sender_time = (double) sender_seconds + (double) (sender_nanoseconds / 1000000000.0) - first_sender_timestamp;
					receiver_time = (double) receiver_seconds + (double) (receiver_nanoseconds / 1000000000.0) - first_receiver_timestamp;
				}

				// Only remember packets of this chirp
				if (chirp == chirpToProcess) {

					if (first_sender_timestamp_of_chirp > sender_time) {
						first_packet_latency = receiver_time - sender_time;
						first_sender_timestamp_of_chirp = sender_time;
					}

					// Remember lowest found time
					if (lowestRecTime_Seconds > receiver_seconds) {
						lowestRecTime_Seconds = receiver_seconds;
						lowestRecTime_NanoSeconds = receiver_nanoseconds;
					} else if (lowestRecTime_Seconds == receiver_seconds && lowestRecTime_NanoSeconds > receiver_nanoseconds) {
						lowestRecTime_Seconds = receiver_seconds;
						lowestRecTime_NanoSeconds = receiver_nanoseconds;
					}

					// Remember highest found time
					if (highestRecTime_Seconds < receiver_seconds) {
						highestRecTime_Seconds = receiver_seconds;
						highestRecTime_NanoSeconds = receiver_nanoseconds;
					} else if (highestRecTime_Seconds == receiver_seconds && highestRecTime_NanoSeconds < receiver_nanoseconds) {
						highestRecTime_Seconds = receiver_seconds;
						highestRecTime_NanoSeconds = receiver_nanoseconds;
					}

					chirpSizeInByte += packetsize;

					countOfPackets++;

					last_sender_time = sender_time;
					last_receiver_time = receiver_time;

					found_packet_of_chirp = true;
					lastfoundPacket_linenumber_of_chirp = linecounter;
				} // Remember starting position of next chirp first packet
				else if (chirp == (chirpToProcess + 1) && startpos_for_nextchirp == 0) {
					startpos_for_nextchirp = pos_nextchirp;
				}

				// Remember highest chirp number, needed for termination of outer while
				if (chirp > highestFoundChirpNumber) highestFoundChirpNumber = chirp;

				// Exit loop when the packets of a chirp are not found
				// int32_t the 2*max of chirpsize next lines after the last packet we found
				if (linecounter > lastfoundPacket_linenumber_of_chirp + (2 * nChirpsize_) && found_packet_of_chirp) break;

			}
		}
		rawData.clear();
		rawData.seekg(0, ios::beg);	// Reset the file to its beginning

		// Check if received not to many packets for this chirp
		if (countOfPackets > 0) {
			// Calculate Data
			chirp_send_duration = static_cast<double>(static_cast<int32_t>(highestRecTime_Seconds) - static_cast<int32_t>(lowestRecTime_Seconds))
					+ ((static_cast<int32_t>(highestRecTime_NanoSeconds) - static_cast<int32_t>(lowestRecTime_NanoSeconds)) / 1000000000.0);
			calculatedDataRate = chirpSizeInByte / chirp_send_duration;

			// Adjust from byte/s to kbyte/s
			calculatedDataRate /= 1000;

			// calculate Moving Average over k elements
			moving_average[moving_average_counter] = calculatedDataRate;
			moving_average_result = 0;
			for (uint32_t i = 0; i < nCountSmoothening_; i++)
				moving_average_result += moving_average[i];
			moving_average_result = moving_average_result / nCountSmoothening_;
			moving_average_counter++;
			if (moving_average_counter >= nCountSmoothening_) moving_average_counter = 0;

			//FIXME Krauthoff old code
			// calculate Moving Average over 3 elements
//			if (chirpToProcess == 1) {
//				moving_average[moving_average_counter] = calculatedDataRate;
//				moving_average_result = calculatedDataRate;
//				moving_average_counter++;
//			} else if (chirpToProcess == 2) {
//				moving_average[moving_average_counter] = calculatedDataRate;
//				moving_average_result = (moving_average[0] + moving_average[1]) / 2;
//				moving_average_counter++;
//			} else {
//				moving_average[moving_average_counter] = calculatedDataRate;
//				moving_average_result = (moving_average[0] + moving_average[1] + moving_average[2]) / 3;
//				moving_average_counter++;
//				if (moving_average_counter > 2) {
//					moving_average_counter = 0;
//				}
//			}

			// Write calculated Data
			processedData << chirpToProcess << "\t"
					<< countOfPackets << "\t"
					<< packetsize << "\t"
					<< chirpSizeInByte << "\t"
					<< chirp_send_duration << "\t"
					<< calculatedDataRate << " \t"
					<< moving_average_result << "\t"
					<< first_packet_latency << "\t"
					<< loss_in_chirp << "\t"
					<< lowestRecTime_Seconds << "\t"
					<< lowestRecTime_NanoSeconds
					<< "\n";

			cout << chirpToProcess << "\t"
					<< countOfPackets << "\t"
					<< packetsize << "\t"
					<< chirpSizeInByte << "\t"
					<< calculatedDataRate << "\t"
					<< loss_in_chirp << "\n";
		}

		first_packet_latency = 0;
		first_sender_timestamp_of_chirp = 10000.0;
		chirpToProcess++;
	} while (chirpToProcess <= highestFoundChirpNumber);

	rawData.close();
	processedData.close();

	// Now we create gnuplot files for this node

	if (createProcessedPlotFile() == -1) {
		pantheios::log_ERROR("RECA| Error while createProcessedPlotFile()");
		return -1;
	}

	if (createDebugPlotFile() == -1) {
		pantheios::log_ERROR("RECA| Error while createDebugPlotFile()");
		return -1;
	}

	if (isCreateGapPlot_){
		if (creatGapPlotFile() == -1) {
			pantheios::log_ERROR("RECA| Error while createLossPlotFile()");
			return -1;
		}
	}

	if (isCreateCongestionPlot_){
		if (createCongestionPlotFile() == -1){
			pantheios::log_ERROR("RECA| Error while createCongestionPlotFile()");
			return -1;
		}
	} else {
		pantheios::log_INFORMATIONAL("RECA| No congestion plot, because you do not want it.");
	}

	if (isCreateLossPlot_){
		if (createLossPlotFile() == -1) {
			pantheios::log_ERROR("RECA| Error while createLossPlotFile()");
			return -1;
		}
	} else {
		pantheios::log_INFORMATIONAL("RECA| No loss plot, because you do not want it.");
	}

	if (isCreateChirpAndPacketSizePlot_){
		if (createChirpAndPacketSizePlotFile() == -1) {
			pantheios::log_ERROR("RECA| Error while createChirpAndPacketSizePlotFile()");
			return -1;
		}
	} else {
		pantheios::log_INFORMATIONAL("RECA| No chirpAndPacketSize plot, because you do not want it.");
	}

	pantheios::log_DEBUG("RECA| Leaving calculateResults()");

	return (0);
}
