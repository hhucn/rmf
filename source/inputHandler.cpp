/* inputHandler.cpp */
#include "inputHandler.h"

// Includes
#include <net/if.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <unistd.h>

const string help_msg =
        "\
	---------\n\
	- Other -\n\
	---------\n\
	-h   Help\n\
	-v   Version\n\
	\n\
	\n\
	-----------------\n\
	- Local Setting -\n\
	-----------------\n\
	-a   0|1             Enable binding of process/Thread to cpu depending on system specs\n\
	-G   0|1             Activate performance scaling governor\n\
	-l   0|1|2           LogLevel\n\
	-L   0|1             Deactivate swapping of memory\n\
	-r   client|server   Node Role\n\
	-o   FOLDER          Output folder for logfiles\n\
	-F   Filename        Filename of experiment without ending\n\
	-P   0|1|2           Process scheduling (default | round robin | fifo)\n\
	-Q   Count           Count of max elements per list (memory managment)\n\
	-dQ  Count			 Count of the max elements in the device queue, before putting more packets in it for the measurement Channel (-1 = off) (root needed) \n\
	-pQ  %               Percentage of the maximum queuesize of the measurementchannel interface. An interface for the measurementchannel has to be given as an additional parameter (-cm for client, -sm for server). \n\
	-R   Count           max read retry on socket\n\
	-W   Count           max write retry on socket\n\
	-T   0|1|2|3         Select Timing Method (0 = Hpet, 1 = getTimeOfDay, 2 = eigene, 3 = autoselect)\n\
	------------------\n\
	- Global Setting -\n\
	------------------\n\
	-M   method          Messverfahren	(basic, assolo, new, mobile, algo)\n\
	-m   up, down, both  Measured Link uplink downlink (clients view)\n\
	-t   time            Dauer der Messzeit (Sekunden)\n\
	-g   on|off          Safeguard Channel\n\
	\n\
	-ip4|-ip6           IP Protocoll Version\n\
	\n\
	\n\
	----------\n\
	- Client -\n\
	----------\n\
	-CC IP              Client Control Channel Address      (TCP)\n\
	-CM IP              Client Measurement Channel Address  (UDP)\n\
	-CS IP              Client Safeguard Channel Address    (UDP)\n\
	\n\
	-Cc PORT            Client Control Channel Port\n\
	-Cm PORT            Client Measurement Channel Port\n\
	-Cs PORT            Client Safeguard Channel Port\n\
	\n\
	-cc INTERFACE       Client Control Channel Interface\n\
	-cm INTERFACE       Client Measurement Channel Interface\n\
	-cs INTERFACE       Client Safeguard Channel Interface\n\
	\n\
	-csg IP             Client Safeguard Channel Gateway\n\
	\n\
	\n\
	----------\n\
	- Server -\n\
	----------\n\
	-SC IP              Server Control Channel Address      (TCP)\n\
	-SM IP              Server Measurement Channel Address  (UDP)\n\
	-SS IP              Server Safeguard Channel Address    (UDP)\n\
	\n\
	-Sc PORT            Server Control Channel Port\n\
	-Sm PORT            Server Measurement Channel Port\n\
	-Ss PORT            Server Safeguard Channel Port\n\
	\n\
	-sc INTERFACE       Server Control Channel Interface\n\
	-sm INTERFACE       Server Measurement Channel Interface\n\
	-ss INTERFACE       Server Safeguard Channel Interface\n\
	\n\
	\n\
	-----------------------------\n\
	- mTBUS Parameter -\n\
	-----------------------------\n\
	-mT_a    [0|1]       Adapting different chirp sizes                        (default:          1)\n\
	-mT_d    [0|1]       Use of Congestion Detector                            (default:          1)\n\
	-mT_s    [Byte/s]    initial data rate                                     (default:      16000)\n\
	-mT_l    [%]         Channel load(0,100]                                   (default:         50)\n\
	-mT_clu  [int]       Maximum chirp length for the uplink   (#packets)      (default:        500)\n\
	-mT_cld  [int]       Maximum chirp length for the downlink (#packets)      (default:       1000)\n\
	-mT_mdu  [Byte/s]    Maximum data rate for the uplink                      (default:  1000*1000)\n\
	-mT_mdd  [Byte/s]    Maximum data rate for the downlink                    (default:  1000*1000)\n\
	\n\
	\n\
";

/**
 * Constructor set all variables to default values.
 */
inputHandler::inputHandler() {
    this->experimentDuration = 300; // 300 Seconds
    this->ipVersion = 0; // 0=IPV4 1=IPV6
    this->logfilePath = "results/"; // Path for the logfile
    this->inputLogfilePath = "results/"; // Path for the logfile which was entered on commandline
    this->inputFileNameFormat = ""; // Format of the generated filenames, if empty use default
    this->logLevel = 0; // normal
    //	this->measurementMethod= 0;				// brute
    this->measuredLink = 2; // both
    this->nodeRole = 1; // server
    this->safeguardChannel = 0; // off
    this->timingMethod = 1; // getTime
    this->uint32_t_size = 0; // Set 0 to be sure that we get later the correct value

    this->maxDeviceQueueSize = -1; // Max Count of elements in the device queue before queuing more packets to the measurement Channel
    this->maxDeviceQueueSizePercentage = -1; // Max Count of elements in the device queue before queuing more packets to the measurement Channel

    this->maxQueueSize = 1000; // Max Count of elements of each Queue
    this->maxWriteAttemptsOnSocket = 20; // Max attempts for writing on a socket before the program exits
    this->maxWaitForSocketWrite = 3; // Max seconds we wait for a socket to be ready for writing
    this->maxReadAttemptsOnSocket = 20; // Max attempts for reading on a socket before the program exits
    this->maxWaitForSocketRead = 3; // Max seconds we wait for a socket to be ready for reading

    this->memorySwapping = 0; // Memory Swapping allowed
    this->processScheduling = 0; // System default
    this->scalingGovernor = 0; // System default
    this->cpuAffinity = 1; // on

    // Client
    this->client_cAddress = "0.0.0.0"; // "Autoselect"
    this->client_mAddress = ""; // empty, use control Address when empty
    this->client_sAddress = ""; // empty, use control Address when empty
    this->client_cPort = 8388; //
    this->client_mPort = 8390; //
    this->client_sPort = 8392; //
    this->client_cInterface = ""; // empty
    this->client_mInterface = ""; // empty
    this->client_sInterface = ""; // empty
    this->client_sGateway = ""; // empty

    // Server
    this->server_cAddress = "0.0.0.0"; // "Autoselect"
    this->server_mAddress = ""; // empty, use control Address when empty
    this->server_sAddress = ""; // empty, use control Address when empty
    this->server_cPort = 8387; //
    this->server_mPort = 8389; //
    this->server_sPort = 8391; //
    this->server_cInterface = ""; // empty
    this->server_mInterface = ""; // empty
    this->server_sInterface = ""; // empty

    this->measurement_magic = 0;
    this->safeguard_magic = 0;

    this->measurementMethodClassObj = 0;
}

void inputHandler::reset() {
    this->client_cAddress = "0.0.0.0"; // "Autoselect"
}

// ------------

/**
 * Destructor
 */
inputHandler::~inputHandler(void) {
    delete this->get_measurementMethodClass();
}

// ------------

/**
 *	Prints the program version and exit
 */
void inputHandler::printVersion(void) {
    cout << "Version 1.0" << "\n";
    exit(0);
}

// ------------

/**
 *	Prints the help and exit
 */
void inputHandler::printHelp(void) {
    cout << help_msg << "\n";
    exit(0);
}

// ------------

/**
 *	This function will parse the command line input for the logging level, logging path, master node address and client node address.
 *	Initialize the logging framework
 *
 *	@param argc 	Parameter count
 *	@param argv 	Parameter Array
 *  @return 0 on success. -1 on error
 */
int inputHandler::activateLogging(int argc, char **argv) {
    // Check if we have at least one parameter and we are not the server
    if (argc < 2 && this->nodeRole == 0) this->printHelp();

    // Cast argv into a string for easier handling
    string givenParameter;
    for (int i = 1; i < argc; i++) {
        // Instead of "-l o " the string contains "-l|0|"
        // '|' will Separator for parsing. ' ' is needed in (-o FOLDER):
        givenParameter += (string) argv[i] + "|";
    }

    cout << givenParameter << "\n";
    
    // Check if only Version has to be printed => exit
    if (givenParameter.find("-v|") != string::npos) this->printVersion();

    // Check if only Help has to be printed => exit
    if (givenParameter.find("-h|") != string::npos) this->printHelp();

    // - Loglevel -
    // ------------
    // Search for logLevel pattern. Set logLevel to parameter value, delete processed parameter from string
    if (givenParameter.find("-l|0|") != string::npos) {
        this->logLevel = 0;
        givenParameter.erase(givenParameter.find("-l|0|"), 5);
    } else if (givenParameter.find("-l|1|") != string::npos) {
        this->logLevel = 1;
        givenParameter.erase(givenParameter.find("-l|1|"), 5);
    } else if (givenParameter.find("-l|2|") != string::npos) {
        this->logLevel = 2;
        givenParameter.erase(givenParameter.find("-l|2|"), 5);
    }

    // There should be no LogLevel Parameter left, otherwise exit
    if (givenParameter.find("-l") != string::npos) {
        cout << "Error: Log level parameter malformed or more then one (-l)." << "\n";
        return (-1);
    }
    // ------------

    // - Output folder -
    // -----------------
    // Try to find the parameter
    if (givenParameter.find("-o|") != string::npos) {
        // Try to get the PATH substring in it
        uint32_t afterFirstSeparator = givenParameter.find("-o|") + 3; // +3 for the 3 three chars in "-o|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find was successful and adjust the path
        if (beforeLastSeparator != string::npos) {
            // Set to the path
            this->inputLogfilePath = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // check if the last char is a '/', if not add it
            if (this->inputLogfilePath.substr(this->inputLogfilePath.length() - 1, this->inputLogfilePath.length()).compare("/") != 0) this->inputLogfilePath =
                    this->inputLogfilePath + "/";

            // Delete processed parameter from string. To include the separators use -3 & +4
            givenParameter.erase(afterFirstSeparator - 3, beforeLastSeparator + 4);
        }
    }
    // There should be no more Output folder Parameters left, otherwise exit
    if (givenParameter.find("-o") != string::npos) {
        cout << "Error: Logfile path parameter malformed or more then one (-o)." << "\n";
        return (-1);
    }
    // -----------------

    // - inputFileNameFormat -
    // -----------------------
    // Try to find the parameter
    if (givenParameter.find("-F|") != string::npos) {
        // Try to get the PATH substring in it
        uint32_t afterFirstSeparator = givenParameter.find("-F|") + 3; // +3 for the 3 three chars in "-o|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find was successful and adjust the path
        if (beforeLastSeparator != string::npos) {
            // Set to the path
            this->inputFileNameFormat = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Delete processed parameter from string. To include the separators use -3 & +4
            givenParameter.erase(afterFirstSeparator - 3, beforeLastSeparator + 4);
        }
    }
    // There should be no more Output folder Parameters left, otherwise exit
    if (givenParameter.find("-F") != string::npos) {
        cout << "Error: Filename Format path parameter malformed or more then one (-F)." << "\n";
        return (-1);
    }
    // -----------------

    // Set the desired log level and open the logfile
    // ------------------------------------------------------------------------
    switch (this->logLevel) {
        case 0: // Normal with logfile
            pantheois_logLevel = pantheios::SEV_NOTICE;
            pantheios::log_NOTICE("MAIN| --------------------");
            pantheios::log_NOTICE("MAIN| - rmf initializing -");
            pantheios::log_NOTICE("MAIN| --------------------");
            pantheios::log_NOTICE("MAIN| LogLevel: Normal");
            pantheios::log_NOTICE("MAIN| ");
            break;

        case 1: // Extended with logfile
            pantheois_logLevel = pantheios::SEV_INFORMATIONAL;
            pantheios::log_NOTICE("MAIN| --------------------");
            pantheios::log_NOTICE("MAIN| - rmf initializing -");
            pantheios::log_NOTICE("MAIN| --------------------");
            pantheios::log_NOTICE("MAIN| LogLevel: Extended");
            pantheios::log_NOTICE("MAIN| ");
            break;

        case 2: // Debug with logfile
            pantheois_logLevel = pantheios::SEV_DEBUG;
            pantheios::log_NOTICE("MAIN| ---------------------");
            pantheios::log_NOTICE("MAIN| - rmf initializing -");
            pantheios::log_NOTICE("MAIN| ---------------------");
            pantheios::log_NOTICE("MAIN| LogLevel: Debug");
            pantheios::log_NOTICE("MAIN| ");
            pantheios::log_NOTICE("MAIN| 	WARINING: Not meant for measurement !");
            pantheios::log_NOTICE("MAIN| ");
            pantheios::log_NOTICE("MAIN| ");
            break;

    }
    // ------------------------------------------------------------------------

    return (0);
}

// ------------

/**
 * Creates the logfile name. Format: rmf_MEASUREMENT_METHOD_DATE+TIME_ClientAddress-to-ServerAddress.log
 * or uses the given one -F NAME
 *
 *
 * @return 0
 */
//int inputHandler::createLogfileName(uint32_t measurementMethod, uint32_t nodeRole)

int inputHandler::createLogfileName() {
    pantheios::log_DEBUG("MAIN| Entering createLogfileName():");

    string logfile;

    // Try to create the logfile path
    string mkdirPath("mkdir -p " + inputLogfilePath);

    if (system(mkdirPath.c_str()) == -1) cout << "Error: Creation of the logfile path failed \"" << inputLogfilePath << "\"" << "\n";

    // If no filename format was given build default
    if (this->inputFileNameFormat.empty()) {
        // Create the logfile name => Format: rmf_MEASUREMENT_METHOD_DATE+TIME_ClientAddress-to-ServerAddress.log
        // ------------------------------------------------------------------------

        // First get the date then build the file-path
        time_t result = time(NULL);
        char date[20];

        if (strftime(date, 20, "%Y.%m.%d-%H:%M:%S", localtime(&result)) == 0) { // This should never happen...
            cout << "Error: An error occurred when the program tried to get the local date&time of the system. This is bad :(" << "\n";
            return (-1);
        }

        // Build the string Logfile Path
        //switch( measurementMethod )
        //{
        //case 0:	logfile = inputLogfilePath + "mBasic_"  + date + "_" + this->client_cAddress + "_to_" + this->server_cAddress;	break;
        //case 1:	logfile = inputLogfilePath + "mAssolo_" + date + "_" + this->client_cAddress + "_to_" + this->server_cAddress;	break;
        //case 2:	logfile = inputLogfilePath + "mNew_"    + date + "_" + this->client_cAddress + "_to_" + this->server_cAddress;	break;
        //case 3:	logfile = inputLogfilePath + "mMobile_"  + date + "_" + this->client_cAddress + "_to_" + this->server_cAddress;	break;

        //default: logfile = inputLogfilePath + "UNKOWN_" + date + "_" + this->client_cAddress + "_to_" + this->server_cAddress;	break;
        //}
        logfile = this->measurementMethodClassObj->getLogFileName();
        string mkdirPath2("mkdir -p " + logfile);
        if (system(mkdirPath2.c_str()) == -1) cout << "Error: Creation of the logfile path failed \"" << logfile << "\"" << "\n";
    } else {// Use given format
        logfile = inputLogfilePath + inputFileNameFormat;
    }

    // Remeber it for the other file we want to create
    logfilePath = logfile;

    switch (nodeRole) {
        case 0:
            logfile.append("client.log");
            break;
        case 1:
            logfile.append("server.log");
            break;
        default:
            logfile.append("log");
            break;
    }
    
    //create the complete path
    string substring = logfile.substr(0,logfile.find_last_of("\\/"));
    string mkdirPath2("mkdir -p " + substring);
    if (system(mkdirPath2.c_str()) == -1) cout << "Error: Creation of the logfile path failed \"" << substring << "\"" << "\n";

    pantheios::log_NOTICE("MAIN| -> Logfile set to: ", logfile);

    pantheios_be_file_setFilePath(logfile.c_str());

    pantheios::log_DEBUG("MAIN| Leaving createLogfileName()");
    pantheios::log_DEBUG("MAIN| ");

    return (0);
}

// ------------

/**
 *	This function will parse the command line input for known parameters.
 *	It will store the data for later use.
 *
 *	@param argc 	Parameter count
 *	@param argv		Parameter Array
 *  @return 0 on success. -1 on error
 */
int inputHandler::parseParameter(int argc, char **argv) {
    pantheios::log_DEBUG("MAIN| Entering parseParameter():");

    // Cast argv into a string for easier handling
    string givenParameter;
    for (int i = 1; i < argc; i++) {
        // Instead of "-l o " the string contains "-l|0|"
        // '|' will Separator for parsing. ' ' is needed in (-o FOLDER):
        givenParameter += (string) argv[i] + "|";
    }
    pantheios::log_DEBUG("MAIN| -> Casted input to string = ", givenParameter.c_str());
    pantheios::log_DEBUG("MAIN| -> Starting parsing string.");

    // - Loglevel -
    // ------------
    // Search for logLevel pattern and delete it. Was already set in activateLogging()
    if (givenParameter.find("-l|0|") != string::npos) {
        givenParameter.erase(givenParameter.find("-l|0|"), 5);
        pantheios::log_DEBUG("MAIN|    -> Found loglevel = 0");
    } else if (givenParameter.find("-l|1|") != string::npos) {
        givenParameter.erase(givenParameter.find("-l|1|"), 5);
        pantheios::log_DEBUG("MAIN|    -> Found loglevel = 1");
    } else if (givenParameter.find("-l|2|") != string::npos) {
        givenParameter.erase(givenParameter.find("-l|2|"), 5);
        pantheios::log_DEBUG("MAIN|    -> Found loglevel = 2");
    } else pantheios::log_DEBUG("MAIN|    -> Log level not found. Default = 0");
    // ------------

    // - Output folder -
    // -----------------
    // Search for Output folder pattern and delete it. Was already set in activateLogging()
    if (givenParameter.find("-o|") != string::npos) {
        // Try to get the PATH substring in it
        uint32_t afterFirstSeparator = givenParameter.find("-o|") + 3; // +3 for the 3 three chars in "-o|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find was successful and adjust the path
        if (beforeLastSeparator != string::npos) {
            pantheios::log_DEBUG("MAIN|    -> Found output folder = ", givenParameter.substr(afterFirstSeparator, beforeLastSeparator));

            // Delete processed parameter from string. To include the separators use -3 & +4
            givenParameter.erase(afterFirstSeparator - 3, beforeLastSeparator + 4);
        }
    }
    // -----------------

    // - Filename Format  -
    // --------------------
    // Search for Filename Format pattern and delete it. Was already set in activateLogging()
    if (givenParameter.find("-F|") != string::npos) {
        // Try to get the PATH substring in it
        uint32_t afterFirstSeparator = givenParameter.find("-F|") + 3; // +3 for the 3 three chars in "-o|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find was successful and adjust the path
        if (beforeLastSeparator != string::npos) {
            pantheios::log_DEBUG("MAIN|    -> Found Filename Format = ", givenParameter.substr(afterFirstSeparator, beforeLastSeparator));

            // Delete processed parameter from string. To include the separators use -3 & +4
            givenParameter.erase(afterFirstSeparator - 3, beforeLastSeparator + 4);
        }
    }
    // -----------------

    // - Measured Link -
    // -----------------
    // Search for logLevel pattern. Set logLevel to parameter value, delete processed parameter from string
    if (givenParameter.find("-m|up|") != string::npos) {
        this->measuredLink = 0;
        givenParameter.erase(givenParameter.find("-m|up|"), 6);
        pantheios::log_DEBUG("MAIN|    -> Found measured link = up ");
    } else if (givenParameter.find("-m|down|") != string::npos) {
        this->measuredLink = 1;
        givenParameter.erase(givenParameter.find("-m|down|"), 8);
        pantheios::log_DEBUG("MAIN|    -> Found measured link = down");
    } else if (givenParameter.find("-m|both|") != string::npos) {
        this->measuredLink = 2;
        givenParameter.erase(givenParameter.find("-m|both|"), 8);
        pantheios::log_DEBUG("MAIN|    -> Found measured link = both");
    } else pantheios::log_DEBUG("MAIN|    -> Measured link not found. Default = both");

    // There should be no Measured Link Parameter left, otherwise exit
    if (givenParameter.find("-m|") != string::npos) {
        pantheios::log_ERROR("MAIN| Error: Measured link parameter malformed or more then one (-m).");
        return (-1);
    }
    // -----------------

    // - Timing Method -
    // -----------------
    if (givenParameter.find("-T|0|") != string::npos) {
        this->timingMethod = 0;
        givenParameter.erase(givenParameter.find("-T|0|"), 5);
        pantheios::log_DEBUG("MAIN|    -> Found timing method = HPET ");
    } else if (givenParameter.find("-T|1|") != string::npos) {
        this->timingMethod = 1;
        givenParameter.erase(givenParameter.find("-T|1|"), 5);
        pantheios::log_DEBUG("MAIN|    -> Found timing method = GetTheTimeOfDay() ");
    } else if (givenParameter.find("-T|2|") != string::npos) {
        this->timingMethod = 2;
        givenParameter.erase(givenParameter.find("-T|2|"), 5);
        pantheios::log_DEBUG("MAIN|    -> Found timing method = custom ");
    } else if (givenParameter.find("-T|3|") != string::npos) {
        this->timingMethod = 3;
        givenParameter.erase(givenParameter.find("-T|3|"), 5);
        pantheios::log_DEBUG("MAIN|    -> Found timing method = auto detect ");
    } else pantheios::log_DEBUG("MAIN|    -> Found timing method = auto detect ");

    // There should be no Measured Link Parameter left, otherwise exit
    if (givenParameter.find("-T") != string::npos) {
        pantheios::log_ERROR("MAIN| Error: Timing method parameter malformed or more then one (-T).");
        return (-1);
    }
    // -----------------

    // - Node Role -
    // -------------
    // Search for Node Role pattern. Set Node Role to parameter value, delete processed parameter from string
    if (givenParameter.find("-r|client|") != string::npos) {
        this->nodeRole = 0;
        givenParameter.erase(givenParameter.find("-r|client|"), 10);
        pantheios::log_DEBUG("MAIN|    -> Found node role = client ");
    } else if (givenParameter.find("-r|server|") != string::npos) {
        this->nodeRole = 1;
        givenParameter.erase(givenParameter.find("-r|server|"), 10);
        pantheios::log_DEBUG("MAIN|    -> Found node role = server ");
    } else pantheios::log_DEBUG("MAIN|    -> Node role not found. Default = server ");

    // There should be no Node Role left, otherwise exit
    if (givenParameter.find("-r") != string::npos) {
        pantheios::log_ERROR("MAIN| Error: Node role parameter malformed or more then one (-r).");
        return (-1);
    }
    // -------------

    // - Experiment duration -
    //------------------------
    if (givenParameter.find("-t|") != string::npos) {
        uint32_t afterFirstSeparator = givenParameter.find("-t|") + 3; // +3 for the 3 three chars in "-t|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find() was successful and adjust the time
        if (beforeLastSeparator != string::npos) {
            uint32_t i = 0;
            string input = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Check if input only contains numbers
            while (isdigit((input.c_str())[i]) && i < input.length()) {
                i++;
            }
            if (i == input.length()) {
                this->experimentDuration = (uint32_t) atoi(input.c_str());
                pantheios::log_DEBUG("MAIN|    -> Found experiment duration = ", input.c_str());
            } else {
                pantheios::log_ERROR("MAIN| Error: Experiment duration value is not a number.");
                return (-1);
            }

            // Delete processed parameter from string. To include the separators use -3 & +4
            givenParameter.erase(afterFirstSeparator - 3, beforeLastSeparator + 4);
        } else {
            pantheios::log_ERROR("MAIN| Error: Experiment duration parameter malformed.");
            return (-1);
        }
    } else pantheios::log_DEBUG("MAIN|    -> Experiment duration not found. Default = 300");

    // There should be no more Experiment duration Parameters left, otherwise exit
    if (givenParameter.find("-t") != string::npos) {
        pantheios::log_ERROR("MAIN| Error: Experiment duration parameter malformed or more then one (-t).");
        return (-1);
    }
    //------------------------

    // - IP Protocol Version -
    // -----------------------
    // Search for IP Version pattern. Set IP Version to parameter value, delete processed parameter from string
    if (givenParameter.find("-ip4|") != string::npos) {
        this->ipVersion = 0;
        givenParameter.erase(givenParameter.find("-ip4|"), 5);
        pantheios::log_DEBUG("MAIN|    -> Found IP protocol version = IPv4");
    } else if (givenParameter.find("-ip6|") != string::npos) {
        this->ipVersion = 1;
        givenParameter.erase(givenParameter.find("-ip6|"), 5);
        pantheios::log_DEBUG("MAIN|    -> Found IP protocol version = IPv6");
    } else pantheios::log_DEBUG("MAIN|    -> IP protocol version not found. Default = IPv4");

    // There should be no IP Version left, otherwise exit
    if (givenParameter.find("-ip4") != string::npos || givenParameter.find("-ip6") != string::npos) {
        pantheios::log_ERROR("MAIN| Error: IP protocol version parameter malformed or more then one (-ip4|-ip6).");
        return (-1);
    }
    // -----------------------

    // - Measurement Method -
    // ---------------------
    // Search for Measurement pattern. Set it to parameter value, delete processed parameter from string
    vector<measurementMethodClass*> methods = inputH.get_measurementMethods();
    bool foundMethod = false;
    measurementMethodClass* method;
    //	unsigned int j;
    for (unsigned int i = 0; i < methods.size(); i++) {
        method = methods.at(i);

        string compare = "-M|" + method->get_Name() + "|";
        if (givenParameter.find(compare) != string::npos) {
            givenParameter.erase(givenParameter.find(compare.c_str()), strlen(compare.c_str()));
            this->measurementMethodClassObj = method;
            foundMethod = true;
            //			j = i;
            pantheios::log_DEBUG("MAIN|    -> Found measurement method = " + method->get_Name());
        } else if (i != 0) {
            delete method;
        }
    }

    //	for (unsigned int i = 0; i < methods.size(); i++)
    //		if (i!=j)
    //			delete methods.at(i);

    if (!foundMethod) {
        this->measurementMethodClassObj = methods.at(0);
        //for (unsigned int i = 1; i < methods.size(); i++) delete methods.at(i);
        pantheios::log_DEBUG("MAIN|    -> Measurement method not found. Default = " + this->measurementMethodClassObj->get_Name());
    } else {
        //delete methods.at(0);
    }

    pantheios::log_NOTICE("MAIN| Resizing SafeguardQueue");
    network.getSafeguardReceivingQueue().resize(this->measurementMethodClassObj->getSafeguardReceivingQueueSize());
    network.getSafeguardSendQueue().resize(this->measurementMethodClassObj->getSafeguardSendingQueueSize());
    network.getReceiveQueue().resize(this->measurementMethodClassObj->getReceiveQueueSize());
    network.getSendingQueue().resize(this->measurementMethodClassObj->getSendingQueueSize());
    network.getSentQueue().resize(this->measurementMethodClassObj->getSentQueueSize());
    

    //	if( givenParameter.find("-M|basic|") != string::npos )
    //		{
    //			this->measurementMethod = 0;
    //			givenParameter.erase(givenParameter.find("-M|basic|"),9);
    //			this->measurementMethodClassObj = new measureBasic();
    //			pantheios::log_DEBUG("MAIN|    -> Found measurement method = basic"); }
    //	else if( givenParameter.find("-M|assolo|") != string::npos )
    //		{	
    //			this->measurementMethod = 1;
    //			givenParameter.erase(givenParameter.find("-M|assolo|"),10);
    //			this->measurementMethodClassObj = new measureAssolo();
    //			pantheios::log_DEBUG("MAIN|    -> Found measurement method = assolo"); }
    //  else if( givenParameter.find("-M|new|") != string::npos )
    //		{	
    //			this->measurementMethod = 2;
    //			givenParameter.erase(givenParameter.find("-M|new|"),7);
    //			this->measurementMethodClassObj = new measureNew();
    //			pantheios::log_DEBUG("MAIN|    -> Found measurement method = brute"); }
    //  else if( givenParameter.find("-M|mobile|") != string::npos )
    //		{	
    //			this->measurementMethod = 3;
    //			givenParameter.erase(givenParameter.find("-M|mobile|"),10);
    //			this->measurementMethodClassObj = new MeasureMobile();
    //			pantheios::log_DEBUG("MAIN|    -> Found measurement method = mobile"); }
    //  else
    //		{
    //		this->measurementMethodClassObj = new measureBasic();
    //		pantheios::log_DEBUG("MAIN|    -> Measurement method not found. Default = basic");
    //}

    // There should be no Measurement method left, otherwise exit
    if (givenParameter.find("-ip4") != string::npos || givenParameter.find("-ip6") != string::npos) {
        pantheios::log_ERROR("MAIN| Error: Measurement method parameter malformed or more then one (-M).");
        return (-1);
    }
    // --------------

    // - Node Role -
    // -------------
    // Search for Node Role pattern. Set Node Role to parameter value, delete processed parameter from string
    if (givenParameter.find("-g|off|") != string::npos) {
        this->safeguardChannel = 0;
        givenParameter.erase(givenParameter.find("-g|off|"), 7);
        pantheios::log_DEBUG("MAIN|    -> Found Safeguard status = off ");
    } else if (givenParameter.find("-g|on|") != string::npos) {
        this->safeguardChannel = 1;
        givenParameter.erase(givenParameter.find("-g|on|"), 6);
        pantheios::log_DEBUG("MAIN|    -> Found Safeguard status = on ");
    } else pantheios::log_DEBUG("MAIN|    -> Found Safeguard status = off ");

    // There should be no Node Role left, otherwise exit
    if (givenParameter.find("-g") != string::npos) {
        pantheios::log_ERROR("MAIN| Error: Safeguard parameter malformed or more then one (-s).");
        return (-1);
    }
    // -------------

    // - Client Control Channel Address -
    //-----------------------------------
    if (givenParameter.find("-CC|") != string::npos) {
        uint32_t afterFirstSeparator = givenParameter.find("-CC|") + 4; // +4 for the 3 three chars in "-Ma|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find() was successful and adjust the Address
        if (beforeLastSeparator != string::npos) {
            uint32_t i = 0;
            string input = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Check if input only contains valid chars and then set it
            while ((isalnum(input.c_str()[i]) || input.c_str()[i] == '[' || input.c_str()[i] == ']' || input.c_str()[i] == ':'
                    || input.c_str()[i] == '.') && i <= input.length()) {
                i++;
            }

            if (i == input.length()) this->client_cAddress = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);
            else {
                cout << "Error: Client Control Channel address contains invalid characters." << "\n";
                return (-1);
            }

            // Delete processed parameter from string. To include the separators use -4 & +5
            givenParameter.erase(afterFirstSeparator - 4, beforeLastSeparator + 5);
        } else {
            cout << "Error: Client Control Channel address malformed." << "\n";
            return (-1);
        }
    }

    // There should be no more master Node Address Parameters left, otherwise exit
    if (givenParameter.find("-Ma") != string::npos) {
        cout << "Error: Client Control Channel address malformed or more then one (-CC)." << "\n";
        return (-1);
    }
    //-----------------------------------

    // - Client Control Channel Port -
    // -------------------------------
    if (givenParameter.find("-Cc|") != string::npos) {
        uint32_t afterFirstSeparator = givenParameter.find("-Cc|") + 4; // +4 for the 4 three chars in "-Cc|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find() was successful and adjust the time
        if (beforeLastSeparator != string::npos) {
            uint32_t i = 0;
            string input = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Check if input only contains numbers
            while (isdigit((input.c_str())[i]) && i < input.length()) {
                i++;
            }
            if (i == input.length()) {
                this->client_cPort = (uint32_t) atoi(input.c_str());
                pantheios::log_DEBUG("MAIN|    -> Found client control channel port = ", input.c_str());
            } else {
                pantheios::log_ERROR("MAIN| Error: Client control channel port value is not a number.");
                return (-1);
            }

            // Delete processed parameter from string. To include the separators use -3 & +4
            givenParameter.erase(afterFirstSeparator - 4, beforeLastSeparator + 5);
        } else {
            pantheios::log_ERROR("MAIN| Error: Client control channel port parameter malformed.");
            return (-1);
        }
    } else {
        pantheios::log_DEBUG("MAIN|    -> Client control channel port not found. Default = 8387");
    }

    // There should be no more Control channel port Parameters left, otherwise exit
    if (givenParameter.find("-Cc") != string::npos) {
        pantheios::log_ERROR("MAIN| Error: Client control channel port parameter malformed or more then one (-t).");
        return (-1);
    }
    //-------------------------

    // - Client Control Channel Interface -
    // ------------------------------------
    if (givenParameter.find("-cc|") != string::npos) {
        uint32_t afterFirstSeparator = givenParameter.find("-cc|") + 4; // +4 for the 3 three chars in "-cc|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find() was successful and adjust the path
        if (beforeLastSeparator != string::npos) {
            pantheios::log_DEBUG("MAIN|    -> Found client control channel interface = ",
                    givenParameter.substr(afterFirstSeparator, beforeLastSeparator));
            this->client_cInterface = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Delete processed parameter from string. To include the separators use -4 & +5
            givenParameter.erase(afterFirstSeparator - 4, beforeLastSeparator + 5);
        }
    }

    // There should be no more Control channel port Parameters left, otherwise exit
    if (givenParameter.find("-cc") != string::npos) {
        pantheios::log_ERROR("MAIN| Error: Client control channel interface parameter malformed or more then one (-cc).");
        return (-1);
    }
    // ------------------------------------

    // - Server Control Channel Address -
    //-----------------------------------
    if (givenParameter.find("-SC|") != string::npos) {
        uint32_t afterFirstSeparator = givenParameter.find("-SC|") + 4; // +4 for the 3 three chars in "-Sa|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find() was successful and adjust the Address
        if (beforeLastSeparator != string::npos) {
            uint32_t i = 0;
            string input = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Check if input only contains valid chars and then set it
            while ((isalnum(input.c_str()[i]) || input.c_str()[i] == '[' || input.c_str()[i] == ']' || input.c_str()[i] == ':'
                    || input.c_str()[i] == '.') && i < input.length()) {
                i++;
            }

            if (i == input.length()) this->server_cAddress = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);
            else {
                cout << "Error: Server Control Channel address contains invalid characters." << "\n";
                return (-1);
            };

            // Delete processed parameter from string. To include the separators use -4 & +5
            givenParameter.erase(afterFirstSeparator - 4, beforeLastSeparator + 5);
        } else {
            cout << "Error: Server Control Channel address parameter malformed." << "\n";
            return (-1);
        }
    } else if (this->nodeRole == 0) {
        cout << "Error: Server Control Channel address parameter not set." << "\n";
        return (-1);
    }

    // There should be no more client Node Address Parameters left, otherwise exit
    if (givenParameter.find("-Sa") != string::npos) {
        cout << "Error: Server Control Channel address parameter malformed or more then one (-SC)." << "\n";
        return (-1);
    }
    //-----------------------------------

    // - Server Control Channel Port -
    // -------------------------------
    if (givenParameter.find("-Sc|") != string::npos) {
        uint32_t afterFirstSeparator = givenParameter.find("-Sc|") + 4; // +4 for the 4 three chars in "-Sc|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find() was successful and adjust the time
        if (beforeLastSeparator != string::npos) {
            uint32_t i = 0;
            string input = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Check if input only contains numbers
            while (isdigit((input.c_str())[i]) && i < input.length()) {
                i++;
            }
            if (i == input.length()) {
                this->server_cPort = (uint32_t) atoi(input.c_str());
                pantheios::log_DEBUG("MAIN|    -> Found server control channel port = ", input.c_str());
            } else {
                pantheios::log_ERROR("MAIN| Error: Server control channel port value is not a number.");
                return (-1);
            }

            // Delete processed parameter from string. To include the separators use -3 & +4
            givenParameter.erase(afterFirstSeparator - 4, beforeLastSeparator + 5);
        } else {
            pantheios::log_ERROR("MAIN| Error: Server control channel port parameter malformed.");
            return (-1);
        }
    } else {
        pantheios::log_DEBUG("MAIN|    -> Server control channel port not found. Default = 8387");
    }

    // There should be no more Control channel port Parameters left, otherwise exit
    if (givenParameter.find("-Sc") != string::npos) {
        pantheios::log_ERROR("MAIN| Error: Server control channel port parameter malformed or more then one (-Sc).");
        return (-1);
    }
    //-------------------------

    // - Server Control Channel Interface -
    // ------------------------------------
    if (givenParameter.find("-sc|") != string::npos) {
        uint32_t afterFirstSeparator = givenParameter.find("-sc|") + 4; // +4 for the 3 three chars in "-sc|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find() was successful and adjust the path
        if (beforeLastSeparator != string::npos) {
            pantheios::log_DEBUG("MAIN|    -> Found server control channel interface = ",
                    givenParameter.substr(afterFirstSeparator, beforeLastSeparator));
            this->server_cInterface = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Delete processed parameter from string. To include the separators use -4 & +5
            givenParameter.erase(afterFirstSeparator - 4, beforeLastSeparator + 5);
        }
    }

    // There should be no more Control channel port Parameters left, otherwise exit
    if (givenParameter.find("-sc") != string::npos) {
        pantheios::log_ERROR("MAIN| Error: Server control channel interface parameter malformed or more then one (-cc).");
        return (-1);
    }
    // ------------------------------------

    // - Client Measurement Channel Address -
    // --------------------------------------
    if (givenParameter.find("-CM|") != string::npos) {
        uint32_t afterFirstSeparator = givenParameter.find("-CM|") + 4; // +4 for the 3 three chars in "-CM|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find() was successful and adjust the Address
        if (beforeLastSeparator != string::npos) {
            uint32_t i = 0;
            string input = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Check if input only contains valid chars and then set it
            while ((isalnum(input.c_str()[i]) || input.c_str()[i] == '[' || input.c_str()[i] == ']' || input.c_str()[i] == ':'
                    || input.c_str()[i] == '.') && i <= input.length()) {
                i++;
            }

            if (i == input.length()) {
                this->client_mAddress = input;
                pantheios::log_DEBUG("MAIN|    -> Found client measurement control channel address = ", input);
            } else {
                cout << "Error: Client Measurement Channel address contains invalid characters." << "\n";
                return (-1);
            }

            // Delete processed parameter from string. To include the separators use -4 & +5
            givenParameter.erase(afterFirstSeparator - 4, beforeLastSeparator + 5);
        } else {
            cout << "Error: Client Measurement Channel address malformed." << "\n";
            return (-1);
        }
    }

    // There should be no more master Node Address Parameters left, otherwise exit
    if (givenParameter.find("-CM") != string::npos) {
        cout << "Error: Client Measurement Channel address malformed or more then one (-CM)." << "\n";
        return (-1);
    }
    //-----------------------------------

    // - Client Measurement Channel Port -
    // -----------------------------------
    if (givenParameter.find("-Cm|") != string::npos) {
        uint32_t afterFirstSeparator = givenParameter.find("-Cm|") + 4; // +4 for the 4 three chars in "-Cm|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find() was successful and adjust the time
        if (beforeLastSeparator != string::npos) {
            uint32_t i = 0;
            string input = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Check if input only contains numbers
            while (isdigit((input.c_str())[i]) && i < input.length()) {
                i++;
            }
            if (i == input.length()) {
                this->client_mPort = (uint32_t) atoi(input.c_str());
                pantheios::log_DEBUG("MAIN|    -> Found client Measurement channel port = ", input.c_str());
            } else {
                pantheios::log_ERROR("MAIN| Error: Client Measurement channel port value is not a number.");
                return (-1);
            }

            // Delete processed parameter from string. To include the separators use -3 & +4
            givenParameter.erase(afterFirstSeparator - 4, beforeLastSeparator + 5);
        } else {
            pantheios::log_ERROR("MAIN| Error: Client Measurement channel port parameter malformed.");
            return (-1);
        }
    } else {
        pantheios::log_DEBUG("MAIN|    -> Client Measurement channel port not found. Default = 8387");
    }

    // There should be no more Control channel port Parameters left, otherwise exit
    if (givenParameter.find("-Cm") != string::npos) {
        pantheios::log_ERROR("MAIN| Error: Client Measurement channel port parameter malformed or more then one (-Cm).");
        return (-1);
    }
    //-------------------------

    // - Client Measurement Channel Interface -
    // ------------------------------------
    if (givenParameter.find("-cm|") != string::npos) {
        uint32_t afterFirstSeparator = givenParameter.find("-cm|") + 4; // +4 for the 3 three chars in "-cm|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find() was successful and adjust the path
        if (beforeLastSeparator != string::npos) {
            pantheios::log_DEBUG("MAIN|    -> Found client Measurement channel interface = ",
                    givenParameter.substr(afterFirstSeparator, beforeLastSeparator));
            this->client_mInterface = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Delete processed parameter from string. To include the separators use -4 & +5
            givenParameter.erase(afterFirstSeparator - 4, beforeLastSeparator + 5);
        }
    }

    // There should be no more Control channel port Parameters left, otherwise exit
    if (givenParameter.find("-cm") != string::npos) {
        pantheios::log_ERROR("MAIN| Error: Client Measurement channel interface parameter malformed or more then one (-cm).");
        return (-1);
    }
    // ------------------------------------

    // - Server Measurement Channel Address -
    // ----------------------------------
    if (givenParameter.find("-SM|") != string::npos) {
        uint32_t afterFirstSeparator = givenParameter.find("-SM|") + 4; // +4 for the 3 three chars in "-SM|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find() was successful and adjust the Address
        if (beforeLastSeparator != string::npos) {
            uint32_t i = 0;
            string input = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Check if input only contains valid chars and then set it
            while ((isalnum(input.c_str()[i]) || input.c_str()[i] == '[' || input.c_str()[i] == ']' || input.c_str()[i] == ':'
                    || input.c_str()[i] == '.') && i <= input.length()) {
                i++;
            }

            if (i == input.length()) {
                this->server_mAddress = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);
                pantheios::log_DEBUG("MAIN|    -> Found server measurement channel address = ", input);
            } else {
                cout << "Error: Server Measurement Channel address contains invalid characters." << "\n";
                return (-1);
            }

            // Delete processed parameter from string. To include the separators use -4 & +5
            givenParameter.erase(afterFirstSeparator - 4, beforeLastSeparator + 5);
        } else {
            cout << "Error: Server Measurement Channel address malformed." << "\n";
            return (-1);
        }
    }

    // There should be no more master Node Address Parameters left, otherwise exit
    if (givenParameter.find("-SM") != string::npos) {
        cout << "Error: Server Measurement Channel address malformed or more then one (-SM)." << "\n";
        return (-1);
    }
    //-----------------------------------

    // - Server Measurement Channel Port -
    // -------------------------------
    if (givenParameter.find("-Sm|") != string::npos) {
        uint32_t afterFirstSeparator = givenParameter.find("-Sm|") + 4; // +4 for the 4 three chars in "-Sm|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find() was successful and adjust the time
        if (beforeLastSeparator != string::npos) {
            uint32_t i = 0;
            string input = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Check if input only contains numbers
            while (isdigit((input.c_str())[i]) && i < input.length()) {
                i++;
            }
            if (i == input.length()) {
                this->server_mPort = (uint32_t) atoi(input.c_str());
                pantheios::log_DEBUG("MAIN|    -> Found server Measurement channel port = ", input.c_str());
            } else {
                pantheios::log_ERROR("MAIN| Error: Server Measurement channel port value is not a number.");
                return (-1);
            }

            // Delete processed parameter from string. To include the separators use -3 & +4
            givenParameter.erase(afterFirstSeparator - 4, beforeLastSeparator + 5);
        } else {
            pantheios::log_ERROR("MAIN| Error: Server Measurement channel port parameter malformed.");
            return (-1);
        }
    } else {
        pantheios::log_DEBUG("MAIN|    -> Server Measurement channel port not found. Default = ");
    }

    // There should be no more Control channel port Parameters left, otherwise exit
    if (givenParameter.find("-Sm") != string::npos) {
        pantheios::log_ERROR("MAIN| Error: Server Measurement channel port parameter malformed or more then one (-Sm).");
        return (-1);
    }
    //-------------------------

    // - Server Measurement Channel Interface -
    // ------------------------------------
    if (givenParameter.find("-sm|") != string::npos) {
        uint32_t afterFirstSeparator = givenParameter.find("-sm|") + 4; // +4 for the 3 three chars in "-Ma|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find() was successful and adjust the path
        if (beforeLastSeparator != string::npos) {
            pantheios::log_DEBUG("MAIN|    -> Found server Measurement channel interface = ",
                    givenParameter.substr(afterFirstSeparator, beforeLastSeparator));
            this->server_mInterface = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Delete processed parameter from string. To include the separators use -4 & +5
            givenParameter.erase(afterFirstSeparator - 4, beforeLastSeparator + 5);
        }
    }

    // There should be no more Control channel port Parameters left, otherwise exit
    if (givenParameter.find("-sm") != string::npos) {
        pantheios::log_ERROR("MAIN| Error: Server Measurement channel interface parameter malformed or more then one (-sm).");
        return (-1);
    }
    // ------------------------------------

    // - Client Safeguard Channel Address -
    // --------------------------------------
    if (givenParameter.find("-CS|") != string::npos) {
        uint32_t afterFirstSeparator = givenParameter.find("-CS|") + 4; // +4 for the 3 three chars in "-CS|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find() was successful and adjust the Address
        if (beforeLastSeparator != string::npos) {
            uint32_t i = 0;
            string input = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Check if input only contains valid chars and then set it
            while ((isalnum(input.c_str()[i]) || input.c_str()[i] == '[' || input.c_str()[i] == ']' || input.c_str()[i] == ':'
                    || input.c_str()[i] == '.') && i <= input.length()) {
                i++;
            }

            if (i == input.length()) {
                this->client_sAddress = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);
                pantheios::log_DEBUG("MAIN|    -> Found client safeguard channel address = ", input);
            } else {
                cout << "Error: Client Safeguard Channel address contains invalid characters." << "\n";
                return (-1);
            }

            // Delete processed parameter from string. To include the separators use -4 & +5
            givenParameter.erase(afterFirstSeparator - 4, beforeLastSeparator + 5);
        } else {
            cout << "Error: Client Safeguard Channel address malformed." << "\n";
            return (-1);
        }
    }

    // There should be no more Parameters left, otherwise exit
    if (givenParameter.find("-CS") != string::npos) {
        cout << "Error: Client Safeguard Channel address malformed or more then one (-CS)." << "\n";
        return (-1);
    }
    //-----------------------------------

    // - Client Safeguard Channel Port -
    // -----------------------------------
    if (givenParameter.find("-Cs|") != string::npos) {
        uint32_t afterFirstSeparator = givenParameter.find("-Cs|") + 4; // +4 for the 4 three chars in "-Cs|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find() was successful and adjust the time
        if (beforeLastSeparator != string::npos) {
            uint32_t i = 0;
            string input = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Check if input only contains numbers
            while (isdigit((input.c_str())[i]) && i < input.length()) {
                i++;
            }
            if (i == input.length()) {
                this->client_sPort = (uint32_t) atoi(input.c_str());
                pantheios::log_DEBUG("MAIN|    -> Found client Safeguard channel port = ", input.c_str());
            } else {
                pantheios::log_ERROR("MAIN| Error: Client Safeguard channel port value is not a number.");
                return (-1);
            }

            // Delete processed parameter from string. To include the separators use -3 & +4
            givenParameter.erase(afterFirstSeparator - 4, beforeLastSeparator + 5);
        } else {
            pantheios::log_ERROR("MAIN| Error: Client Safeguard channel port parameter malformed.");
            return (-1);
        }
    } else {
        pantheios::log_DEBUG("MAIN|    -> Client Safeguard channel port not found. Default = 8387");
    }

    // There should be no more Control channel port Parameters left, otherwise exit
    if (givenParameter.find("-Cs") != string::npos) {
        pantheios::log_ERROR("MAIN| Error: Client Safeguard channel port parameter malformed or more then one (-Cm).");
        return (-1);
    }
    //-------------------------

    // - Client Safeguard Channel Interface -
    // ------------------------------------
    if (givenParameter.find("-cs|") != string::npos) {
        uint32_t afterFirstSeparator = givenParameter.find("-cs|") + 4; // +4 for the 3 three chars in "-cs|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find() was successful and adjust the path
        if (beforeLastSeparator != string::npos) {
            pantheios::log_DEBUG("MAIN|    -> Found client Safeguard channel interface = ",
                    givenParameter.substr(afterFirstSeparator, beforeLastSeparator));
            this->client_sInterface = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Delete processed parameter from string. To include the separators use -4 & +5
            givenParameter.erase(afterFirstSeparator - 4, beforeLastSeparator + 5);
        }
    }

    // - Client Safeguard Channel Gateway -
    // ------------------------------------
    if (givenParameter.find("-csg|") != string::npos) {
        uint32_t afterFirstSeparator = givenParameter.find("-csg|") + 5; // +5 for the 4 three chars in "-csg|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find() was successful and adjust the path
        if (beforeLastSeparator != string::npos) {
            pantheios::log_DEBUG("MAIN|    -> Found client Safeguard channel default gateway = ",
                    givenParameter.substr(afterFirstSeparator, beforeLastSeparator));
            this->client_sGateway = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Delete processed parameter from string. To include the separators use -4 & +5
            givenParameter.erase(afterFirstSeparator - 5, beforeLastSeparator + 6);
        }
    }

    // There should be no more Control channel port Parameters left, otherwise exit
    if (givenParameter.find("-cs") != string::npos) {
        pantheios::log_ERROR("MAIN| Error: Client Safeguard channel interface parameter malformed or more then one (-cs).");
        return (-1);
    }
    // ------------------------------------

    // - Server Safeguard Channel Address -
    // ----------------------------------
    if (givenParameter.find("-SS|") != string::npos) {
        uint32_t afterFirstSeparator = givenParameter.find("-SS|") + 4; // +4 for the 3 three chars in "-SS|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find() was successful and adjust the Address
        if (beforeLastSeparator != string::npos) {
            uint32_t i = 0;
            string input = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Check if input only contains valid chars and then set it
            while ((isalnum(input.c_str()[i]) || input.c_str()[i] == '[' || input.c_str()[i] == ']' || input.c_str()[i] == ':'
                    || input.c_str()[i] == '.') && i <= input.length()) {
                i++;
            }

            if (i == input.length()) {
                this->server_sAddress = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);
                pantheios::log_DEBUG("MAIN|    -> Found server safeguard channel address = ", input);
            } else {
                cout << "Error: Server Safeguard Channel address contains invalid characters." << "\n";
                return (-1);
            }

            // Delete processed parameter from string. To include the separators use -4 & +5
            givenParameter.erase(afterFirstSeparator - 4, beforeLastSeparator + 5);
        } else {
            cout << "Error: Server Safeguard Channel address malformed." << "\n";
            return (-1);
        }
    }

    // There should be no more master Node Address Parameters left, otherwise exit
    if (givenParameter.find("-SS") != string::npos) {
        cout << "Error: Server Safeguard Channel address malformed or more then one (-SS)." << "\n";
        return (-1);
    }
    //-----------------------------------

    // - Server Safeguard Channel Port -
    // -------------------------------
    if (givenParameter.find("-Ss|") != string::npos) {
        uint32_t afterFirstSeparator = givenParameter.find("-Ss|") + 4; // +4 for the 4 three chars in "-Ss|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find() was successful and adjust the time
        if (beforeLastSeparator != string::npos) {
            uint32_t i = 0;
            string input = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Check if input only contains numbers
            while (isdigit((input.c_str())[i]) && i < input.length()) {
                i++;
            }
            if (i == input.length()) {
                this->server_sPort = (uint32_t) atoi(input.c_str());
                pantheios::log_DEBUG("MAIN|    -> Found server Safeguard channel port = ", input.c_str());
            } else {
                pantheios::log_ERROR("MAIN| Error: Server Safeguard channel port value is not a number.");
                return (-1);
            }

            // Delete processed parameter from string. To include the separators use -3 & +4
            givenParameter.erase(afterFirstSeparator - 4, beforeLastSeparator + 5);
        } else {
            pantheios::log_ERROR("MAIN| Error: Server Safeguard channel port parameter malformed.");
            return (-1);
        }
    } else {
        pantheios::log_DEBUG("MAIN|    -> Server Safeguard channel port not found. Default = ", pantheios::integer(server_sPort));
    }

    // There should be no more Control channel port Parameters left, otherwise exit
    if (givenParameter.find("-Ss") != string::npos) {
        pantheios::log_ERROR("MAIN| Error: Server Safeguard channel port parameter malformed or more then one (-Ss).");
        return (-1);
    }
    //-------------------------

    // - Server Safeguard Channel Interface -
    // ------------------------------------
    if (givenParameter.find("-ss|") != string::npos) {
        uint32_t afterFirstSeparator = givenParameter.find("-ss|") + 4; // +4 for the 3 three chars in "-ss|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find() was successful and adjust the path
        if (beforeLastSeparator != string::npos) {
            pantheios::log_DEBUG("MAIN|    -> Found server safeguard channel interface = ",
                    givenParameter.substr(afterFirstSeparator, beforeLastSeparator));
            this->server_sInterface = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Delete processed parameter from string. To include the separators use -4 & +5
            givenParameter.erase(afterFirstSeparator - 4, beforeLastSeparator + 5);
        }
    }

    // There should be no more Control channel port Parameters left, otherwise exit
    if (givenParameter.find("-ss") != string::npos) {
        pantheios::log_ERROR("MAIN| Error: Server Safeguard channel interface parameter malformed or more then one (-ss).");
        return (-1);
    }
    // ------------------------------------

    // - Process/Thread Scheduling -
    // -------------------------------
    if (givenParameter.find("-P|") != string::npos) {
        uint32_t afterFirstSeparator = givenParameter.find("-P|") + 3; // +3 for the 3 three chars in "-P|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find() was successful and adjust the time
        if (beforeLastSeparator != string::npos) {
            uint32_t i = 0;
            string input = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Check if input only contains numbers
            while (isdigit((input.c_str())[i]) && i < input.length()) {
                i++;
            }
            if (i == input.length()) {
                switch (atoi(input.c_str())) {
                    case 0:
                        this->processScheduling = (uint32_t) atoi(input.c_str());
                        pantheios::log_DEBUG("MAIN|    -> Found process Scheduling = system default");
                        break;
                    case 1:
                        this->processScheduling = (uint32_t) atoi(input.c_str());
                        pantheios::log_DEBUG("MAIN|    -> Found process Scheduling = Realtime Round Robin");
                        break;
                    case 2:
                        this->processScheduling = (uint32_t) atoi(input.c_str());
                        pantheios::log_DEBUG("MAIN|    -> Found process Scheduling = Realtime FIFO");
                        break;

                    default:
                        pantheios::log_ERROR("MAIN| Error: Process Scheduling not recognized, using default.");
                        break;
                }
            } else {
                pantheios::log_ERROR("MAIN| Error: Process Scheduling value is not a number.");
                return (-1);
            }

            // Delete processed parameter from string. To include the separators use -3 & +4
            givenParameter.erase(afterFirstSeparator - 3, beforeLastSeparator + 4);
        } else {
            pantheios::log_ERROR("MAIN| Error: Process Scheduling parameter malformed.");
            return (-1);
        }
    } else {
        pantheios::log_DEBUG("MAIN|    -> Process Scheduling parameter not found. Default = system default");
    }

    // There should be no more Control channel port Parameters left, otherwise exit
    if (givenParameter.find("-P") != string::npos) {
        pantheios::log_ERROR("MAIN| Error: Process Scheduling parameter malformed or more then one (-P).");
        return (-1);
    }
    //-------------------------

    // - Performance scaling governor -
    // --------------------------------
    if (givenParameter.find("-G|") != string::npos) {
        uint32_t afterFirstSeparator = givenParameter.find("-G|") + 3; // +3 for the 3 three chars in "-P|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find() was successful and adjust the time
        if (beforeLastSeparator != string::npos) {
            uint32_t i = 0;
            string input = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Check if input only contains numbers
            while (isdigit((input.c_str())[i]) && i < input.length()) {
                i++;
            }
            if (i == input.length()) {
                switch (atoi(input.c_str())) {
                    case 0:
                        this->scalingGovernor = (uint32_t) atoi(input.c_str());
                        pantheios::log_DEBUG("MAIN|    -> Found Scaling governor = off");
                        break;
                    case 1:
                        this->scalingGovernor = (uint32_t) atoi(input.c_str());
                        pantheios::log_DEBUG("MAIN|    -> Found Scaling governor = on");
                        break;

                    default:
                        break;
                }
            } else {
                pantheios::log_ERROR("MAIN| Error: Scaling governor value is not a number.");
                return (-1);
            }

            // Delete processed parameter from string. To include the separators use -3 & +4
            givenParameter.erase(afterFirstSeparator - 3, beforeLastSeparator + 4);
        } else {
            pantheios::log_ERROR("MAIN| Error: Scaling governor parameter malformed.");
            return (-1);
        }
    } else {
        pantheios::log_DEBUG("MAIN|    -> Scaling governor parameter not found. Default = off");
    }

    // There should be no more Control channel port Parameters left, otherwise exit
    if (givenParameter.find("-G") != string::npos) {
        pantheios::log_ERROR("MAIN| Error: Scaling governor parameter malformed or more then one (-G).");
        return (-1);
    }
    //-------------------------

    // - Memory Swapping Scheduling -
    // -------------------------------
    if (givenParameter.find("-L|") != string::npos) {
        uint32_t afterFirstSeparator = givenParameter.find("-L|") + 3; // +3 for the 3 three chars in "-P|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find() was successful and adjust the time
        if (beforeLastSeparator != string::npos) {
            uint32_t i = 0;
            string input = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Check if input only contains numbers
            while (isdigit((input.c_str())[i]) && i < input.length()) {
                i++;
            }
            if (i == input.length()) {
                switch (atoi(input.c_str())) {
                    case 0:
                        this->memorySwapping = (uint32_t) atoi(input.c_str());
                        pantheios::log_DEBUG("MAIN|    -> Found memory swapping = off");
                        break;
                    case 1:
                        this->memorySwapping = (uint32_t) atoi(input.c_str());
                        pantheios::log_DEBUG("MAIN|    -> Found memory swapping = on");
                        break;

                    default:
                        break;
                }
            } else {
                pantheios::log_ERROR("MAIN| Error: Memory swapping value is not a number.");
                return (-1);
            }

            // Delete processed parameter from string. To include the separators use -3 & +4
            givenParameter.erase(afterFirstSeparator - 3, beforeLastSeparator + 4);
        } else {
            pantheios::log_ERROR("MAIN| Error: Memory swapping parameter malformed.");
            return (-1);
        }
    } else {
        pantheios::log_DEBUG("MAIN|    -> Memory swapping parameter not found. Default = off");
    }

    // There should be no more Control channel port Parameters left, otherwise exit
    if (givenParameter.find("-L") != string::npos) {
        pantheios::log_ERROR("MAIN| Error: Memory swapping parameter malformed or more then one (-L).");
        return (-1);
    }
    //-------------------------

    // - Memory Swapping Scheduling -
    // -------------------------------
    if (givenParameter.find("-a|") != string::npos) {
        uint32_t afterFirstSeparator = givenParameter.find("-a|") + 3; // +3 for the 3 three chars in "-P|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find() was successful and adjust the time
        if (beforeLastSeparator != string::npos) {
            uint32_t i = 0;
            string input = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Check if input only contains numbers
            while (isdigit((input.c_str())[i]) && i < input.length()) {
                i++;
            }
            if (i == input.length()) {
                switch (atoi(input.c_str())) {
                    case 0:
                        this->cpuAffinity = (uint32_t) atoi(input.c_str());
                        pantheios::log_DEBUG("MAIN|    -> Found cpu affinity = off");
                        break;
                    case 1:
                        this->cpuAffinity = (uint32_t) atoi(input.c_str());
                        pantheios::log_DEBUG("MAIN|    -> Found cpu affinity = on");
                        break;
                }
            } else {
                pantheios::log_ERROR("MAIN| Error: CPU affinity value is not a number.");
                return (-1);
            }

            // Delete processed parameter from string. To include the separators use -3 & +4
            givenParameter.erase(afterFirstSeparator - 3, beforeLastSeparator + 4);
        } else {
            pantheios::log_ERROR("MAIN| Error: CPU affinity parameter malformed.");
            return (-1);
        }
    } else {
        pantheios::log_DEBUG("MAIN|    -> CPU affinity parameter not found. Default = off");
    }

    // There should be no more Control channel port Parameters left, otherwise exit
    if (givenParameter.find("-a") != string::npos) {
        pantheios::log_ERROR("MAIN| Error: CPU affinity parameter malformed or more then one (-a).");
        return (-1);
    }
    //-------------------------

    // - Maximum Queue Size -
    // ----------------------
    if (givenParameter.find("-Q|") != string::npos) {
        uint32_t afterFirstSeparator = givenParameter.find("-Q|") + 3; // +3 for the 3 three chars in "-P|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find() was successful and adjust the time
        if (beforeLastSeparator != string::npos) {
            uint32_t i = 0;
            string input = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Check if input only contains numbers
            while (isdigit((input.c_str())[i]) && i < input.length()) {
                i++;
            }
            if (i == input.length()) {
                this->maxQueueSize = atoi(input.c_str());
                pantheios::log_DEBUG("MAIN|    -> Found Maximum Queue Size = ", input.c_str());
            } else {
                pantheios::log_ERROR("MAIN| Error: Maximum Queue Size value is not a number.");
                return (-1);
            }

            // Delete processed parameter from string. To include the separators use -3 & +4
            givenParameter.erase(afterFirstSeparator - 3, beforeLastSeparator + 4);
        } else {
            pantheios::log_ERROR("MAIN| Error: Maximum Queue Size parameter malformed.");
            return (-1);
        }
    } else {
        pantheios::log_DEBUG("MAIN|    -> Maximum Queue Size not found. Default = 100");
    }

    // - Maximum Device Queue Size -
    // ----------------------
    if (givenParameter.find("-dQ|") != string::npos) {
        uint32_t afterFirstSeparator = givenParameter.find("-dQ|") + 4; // +4 for the 4 three chars in "-dQ|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find() was successful and adjust the time
        if (beforeLastSeparator != string::npos) {
            uint32_t i = 0;
            string input = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Check if input only contains numbers
            while ((isdigit((input.c_str())[i]) || (input.c_str())[i] == '-') && i < input.length()) {
                i++;
            }

            if (i == input.length()) {
                this->maxDeviceQueueSize = atoi(input.c_str());
                pantheios::log_NOTICE("MAIN|    -> Found Maximum Device Queue Size = ", input.c_str());
            } else {
                pantheios::log_ERROR("MAIN| Error: Maximum Device Queue Size value is not a number.");
                return (-1);
            }

            // Delete processed parameter from string. To include the separators use -3 & +4
            givenParameter.erase(afterFirstSeparator - 4, beforeLastSeparator + 5);
        } else {
            pantheios::log_ERROR("MAIN| Error: Maximum Device Queue Size parameter malformed.");
            return (-1);
        }
    } else {
        pantheios::log_DEBUG("MAIN|    -> Maximum Device Queue Size not found. Default = 100");
    }

    // - Maximum Device Queue Size as percentage-
    // ----------------------
    if (givenParameter.find("-pQ|") != string::npos) {
        uint32_t afterFirstSeparator = givenParameter.find("-pQ|") + 4; // +4 for the 4 three chars in "-pQ|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find() was successful and adjust the time
        if (beforeLastSeparator != string::npos) {
            uint32_t i = 0;
            string input = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Check if input only contains numbers
            while ((isdigit((input.c_str())[i]) || (input.c_str())[i] == '-') && i < input.length()) {
                i++;
            }

            if (i == input.length()) {
                this->maxDeviceQueueSizePercentage = atoi(input.c_str());
                pantheios::log_NOTICE("MAIN|    -> Found Maximum Device Queue Size = ", input.c_str());
            } else {
                pantheios::log_ERROR("MAIN| Error: Maximum Device Queue Size value is not a number.");
                return (-1);
            }

            // Delete processed parameter from string. To include the separators use -3 & +4
            givenParameter.erase(afterFirstSeparator - 4, beforeLastSeparator + 5);
        } else {
            pantheios::log_ERROR("MAIN| Error: Maximum Queue Size parameter malformed.");
            return (-1);
        }
    } else {
        pantheios::log_NOTICE("MAIN|    -> Maximum Queue Size not found. Default = 100");
    }

    // There should be no more Control channel port Parameters left, otherwise exit
    if (givenParameter.find("-Q") != string::npos) {
        pantheios::log_ERROR("MAIN| Error: Maximum Queue Size malformed or more then one (-Q).");
        return (-1);
    }
    //-------------------------

    // - Maximum waiting for read on socket in seconds -
    // -------------------------------------------------
    if (givenParameter.find("-R|") != string::npos) {
        uint32_t afterFirstSeparator = givenParameter.find("-R|") + 3; // +3 for the 3 three chars in "-P|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find() was successful and adjust the time
        if (beforeLastSeparator != string::npos) {
            uint32_t i = 0;
            string input = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Check if input only contains numbers
            while (isdigit((input.c_str())[i]) && i < input.length()) {
                i++;
            }
            if (i == input.length()) {
                this->maxWaitForSocketRead = atoi(input.c_str());
                pantheios::log_DEBUG("MAIN|    -> Found Maximum Waiting time for read attempt = ", input.c_str());
            } else {
                pantheios::log_ERROR("MAIN| Error: Maximum Waiting time for read attempt value is not a number.");
                return (-1);
            }

            // Delete processed parameter from string. To include the separators use -3 & +4
            givenParameter.erase(afterFirstSeparator - 3, beforeLastSeparator + 4);
        } else {
            pantheios::log_ERROR("MAIN| Error: Maximum Waiting time for read attempt parameter malformed.");
            return (-1);
        }
    } else {
        pantheios::log_DEBUG("MAIN|    -> Maximum Waiting time for read attempt not found. Default = 10");
    }

    // There should be no more Control channel port Parameters left, otherwise exit
    if (givenParameter.find("-R") != string::npos) {
        pantheios::log_ERROR("MAIN| Error: Maximum Waiting time for read attempt malformed or more then one (-Q).");
        return (-1);
    }
    //-------------------------

    // - Maximum waiting for write on socket in seconds -
    // --------------------------------------------------
    if (givenParameter.find("-W|") != string::npos) {
        uint32_t afterFirstSeparator = givenParameter.find("-W|") + 3; // +3 for the 3 three chars in "-P|"
        uint32_t beforeLastSeparator = givenParameter.substr(afterFirstSeparator, givenParameter.length()).find("|");

        // Check if last find() was successful and adjust the time
        if (beforeLastSeparator != string::npos) {
            uint32_t i = 0;
            string input = givenParameter.substr(afterFirstSeparator, beforeLastSeparator);

            // Check if input only contains numbers
            while (isdigit((input.c_str())[i]) && i < input.length()) {
                i++;
            }
            if (i == input.length()) {
                this->maxWaitForSocketWrite = atoi(input.c_str());
                pantheios::log_DEBUG("MAIN|    -> Found Maximum Waiting time for write attempt = ", input.c_str());
            } else {
                pantheios::log_ERROR("MAIN| Error: Maximum Waiting time for write attempt value is not a number.");
                return (-1);
            }

            // Delete processed parameter from string. To include the separators use -3 & +4
            givenParameter.erase(afterFirstSeparator - 3, beforeLastSeparator + 4);
        } else {
            pantheios::log_ERROR("MAIN| Error: Maximum Waiting time for write attempt parameter malformed.");
            return (-1);
        }
    } else {
        pantheios::log_DEBUG("MAIN|    -> Maximum Waiting time for write attempt not found. Default = 10");
    }

    // There should be no more Control channel port Parameters left, otherwise exit
    if (givenParameter.find("-W") != string::npos) {
        pantheios::log_ERROR("MAIN| Error: Maximum Waiting time for write attempt malformed or more then one (-Q).");
        return (-1);
    }
    //-------------------------

    if (givenParameter.length() != 0) {
        pantheios::log_DEBUG("MAIN|    -> Unknown parameter(s) found: \"", givenParameter, "\". We be transfered to measurement method parser.");
    }
    this->parameterString = givenParameter;

    pantheios::log_DEBUG("MAIN| parseParameter() done.");
    pantheios::log_DEBUG("MAIN|  ");

    return (0);
}

// ------------

/**
 *	Check if the stored values of the member variables are in correct bounds. Resolve DNS names and if the master/client address fits IPv4/IPv6/DNS format.
 *
 *  @return 0 on success. -1 on error
 */
int inputHandler::checkSanityOfInput(void) {
    pantheios::log_DEBUG("MAIN| Entering checkSanityOfInput()");

    // Needed for output because pantheios can't handle uint32_t
    string sOut;
    stringstream ssOut;

    // - Check Log Level Bounds -
    // ---------------------------
    ssOut.str("");
    ssOut << this->logLevel;
    sOut = ssOut.str();

    // 0 = normal, 1 = extended, 2 = debug
    if (this->logLevel < 0 && this->logLevel > 2) {
        pantheios::log_ERROR("MAIN| Error: Log level not in correct bounds [0-2] => ", sOut);
        return (-1);
    }

    pantheios::log_DEBUG("MAIN| -> Log level in correct bounds [0-2] => ", sOut);
    // ---------------------------

    // Check if experiment duration is not 0
    // ---------------------------
    ssOut.str("");
    ssOut << this->experimentDuration;
    sOut = ssOut.str();

    // 1 - 4294967295
    if (this->experimentDuration == 0) {
        pantheios::log_ERROR("MAIN| Error: Experiment duration not in acceptable range [1-4294967295] => ", sOut);
        return (-1);
    }

    pantheios::log_DEBUG("MAIN| -> Experiment duration in correct bounds [1-4294967295] => ", sOut);
    // ---------------------------

    // Check if timing method is between 0-3
    // ---------------------------
    ssOut.str("");
    ssOut << this->timingMethod;
    sOut = ssOut.str();

    // 0 - 3
    if (this->timingMethod < 0 && this->timingMethod > 3) {
        pantheios::log_ERROR("MAIN| Error: Timing method not in acceptable range [0-3] => ", sOut);
        return (-1);
    }

    pantheios::log_DEBUG("MAIN| -> Timing method in correct bounds [0-3] => ", sOut);
    // ---------------------------

    // Check if measurementMethod is in correct bounds 0-3
    // ---------------------------
    //	ssOut.str(""); ssOut << this->logLevel;
    //	sOut = ssOut.str();
    //
    //	// basic 0, assolo 1, new 2, mobile 3
    //	if( this->measurementMethod < 0 && this->measurementMethod > 3 )
    //		{	pantheios::log_ERROR("MAIN| Error: Measurement method not in acceptable range [0-3] =>", sOut); return(-1);	}
    //
    //	pantheios::log_DEBUG("MAIN| -> Measurement method in correct bounds [0-2] => ", sOut);
    // ---------------------------

    // Check if nodeRole is in correct bounds 0-1
    // ---------------------------
    ssOut.str("");
    ssOut << this->nodeRole;
    sOut = ssOut.str();

    // 0 = master, 1 = client
    if (this->nodeRole < 0 && this->nodeRole > 1) {
        pantheios::log_ERROR("MAIN| Error: Node role not in acceptable range [0-1] => ", sOut);
        return (-1);
    }

    pantheios::log_DEBUG("MAIN| -> Node Role in correct bounds [0-1] => ", sOut);
    // ---------------------------

    // Check if measuredLink is in correct bounds 0-2
    // ---------------------------
    ssOut.str("");
    ssOut << this->measuredLink;
    sOut = ssOut.str();

    // 0 = uplink, 1 = downlink, 2 = both
    if (this->measuredLink < 0 && this->measuredLink > 2) {
        pantheios::log_ERROR("MAIN| Error: Measurement link not in acceptable range [0-2] => ", sOut);
        return (-1);
    }

    pantheios::log_DEBUG("MAIN| -> Measured link in correct bounds [0-2] => ", sOut);
    // ---------------------------

    // Check if IP protocol version is in correct bounds 0-1
    // ---------------------------
    ssOut.str("");
    ssOut << this->ipVersion;
    sOut = ssOut.str();

    // 0 = IPv4, 1 = IPv6
    if (this->ipVersion < 0 && this->ipVersion > 2) {
        pantheios::log_ERROR("MAIN| Error: IP protocol version not in acceptable range [0-1] => ", sOut);
        return (-1);
    }

    pantheios::log_DEBUG("MAIN| -> IP protocol version in correct bounds [0-1] => ", sOut);
    // ---------------------------

    // Check if maxQueueSize is between 2-4294967295
    // ---------------------------
    ssOut.str("");
    ssOut << this->maxQueueSize;
    sOut = ssOut.str();

    // 2 - 4294967295
    if (this->maxQueueSize < 2 && this->maxQueueSize > 4294967295u) {
        pantheios::log_ERROR("MAIN| Error: Max. Queue Size not in acceptable range [2-4294967295] => ", sOut);
        return (-1);
    }

    pantheios::log_DEBUG("MAIN| -> Max. Queue Size in correct bounds [2-4294967295] => ", sOut);
    // ---------------------------

    // Check if maxWriteAttemptsOnSocket is between 1-4294967295
    // ---------------------------
    ssOut.str("");
    ssOut << this->maxWriteAttemptsOnSocket;
    sOut = ssOut.str();

    // 1 - 4294967295
    if (this->maxWriteAttemptsOnSocket < 1 && this->maxWriteAttemptsOnSocket > 4294967295u) {
        pantheios::log_ERROR("MAIN| Error: Max. write attempts on socket not in acceptable range [1-4294967295] => ", sOut);
        return (-1);
    }

    pantheios::log_DEBUG("MAIN| -> Max. write attempts on socket in correct bounds [1-4294967295] => ", sOut);
    // ---------------------------

    // Check if maxReadAttemptsOnSocket is between 1-4294967295
    // ---------------------------
    ssOut.str("");
    ssOut << this->maxReadAttemptsOnSocket;
    sOut = ssOut.str();

    // 1 - 4294967295
    if (this->maxReadAttemptsOnSocket < 1 && this->maxReadAttemptsOnSocket > 4294967295u) {
        pantheios::log_ERROR("MAIN| Error: Max. read attempts on socket not in acceptable range [1-4294967295] => ", sOut);
        return (-1);
    }

    pantheios::log_DEBUG("MAIN| -> Max. read attempts on socket in correct bounds [1-4294967295] => ", sOut);
    // ---------------------------

    // Check if maxWaitForSocketWrite is between 0-4294967295
    // ---------------------------
    ssOut.str("");
    ssOut << this->maxWaitForSocketWrite;
    sOut = ssOut.str();

    // 0 - 4294967295
    if (this->maxWaitForSocketWrite < 0 && this->maxWaitForSocketWrite > 4294967295u) {
        pantheios::log_ERROR("MAIN| Error: Max. waiting time for writing on socket not in acceptable range [0-4294967295] => ", sOut);
        return (-1);
    }

    pantheios::log_DEBUG("MAIN| -> Max. waiting time for writing on socket in correct bounds [0-4294967295] => ", sOut);
    // ---------------------------

    // Check if maxWaitForSocketWrite is between 0-4294967295
    // ---------------------------
    ssOut.str("");
    ssOut << this->maxWaitForSocketRead;
    sOut = ssOut.str();

    // 0 - 4294967295
    if (this->maxWaitForSocketRead < 0 && this->maxWaitForSocketRead > 4294967295u) {
        pantheios::log_ERROR("MAIN| Error: Max. waiting time for reading on socket not in acceptable range [0-4294967295] => ", sOut);
        return (-1);
    }

    pantheios::log_DEBUG("MAIN| -> Max. waiting time for reading on socket in correct bounds [0-4294967295] => ", sOut);
    // ---------------------------

    // Check if memorySwapping is between 0-1
    // ---------------------------
    ssOut.str("");
    ssOut << this->memorySwapping;
    sOut = ssOut.str();

    // 0 - 1
    if (this->memorySwapping < 0 && this->memorySwapping > 1) {
        pantheios::log_ERROR("MAIN| Error: Memory swapping not in acceptable range [0-1] => ", sOut);
        return (-1);
    }

    pantheios::log_DEBUG("MAIN| -> Memory swapping in correct bounds [0-1] => ", sOut);
    // ---------------------------

    // Check if processScheduling is between 0-1
    // ---------------------------
    ssOut.str("");
    ssOut << this->processScheduling;
    sOut = ssOut.str();

    // 0 - 2
    if (this->processScheduling < 0 && this->processScheduling > 2) {
        pantheios::log_ERROR("MAIN| Error: Process scheduling not in acceptable range [0-2] => ", sOut);
        return (-1);
    }

    pantheios::log_DEBUG("MAIN| -> Process scheduling in correct bounds [0-2] => ", sOut);
    // ---------------------------

    // Check if scalingGovernor is between 0-1
    // ---------------------------
    ssOut.str("");
    ssOut << this->scalingGovernor;
    sOut = ssOut.str();

    // 0 - 1
    if (this->scalingGovernor < 0 && this->scalingGovernor > 1) {
        pantheios::log_ERROR("MAIN| Error: Scaling Governor not in acceptable range [0-1] => ", sOut);
        return (-1);
    }

    pantheios::log_DEBUG("MAIN| -> Scaling Governor in correct bounds [0-1] => ", sOut);
    // ---------------------------

    // Check if cpuAffinity is between 0-1
    // ---------------------------
    ssOut.str("");
    ssOut << this->cpuAffinity;
    sOut = ssOut.str();

    // 0 - 1
    if (this->cpuAffinity < 0 && this->cpuAffinity > 1) {
        pantheios::log_ERROR("MAIN| Error: CPU Affinity not in acceptable range [0-1] => ", sOut);
        return (-1);
    }

    pantheios::log_DEBUG("MAIN| -> CPU Affinity in correct bounds [0-1] => ", sOut);
    // ---------------------------

    // Check if safeguardChannel is in correct bounds 0-1
    // ---------------------------
    ssOut.str("");
    ssOut << this->safeguardChannel;
    sOut = ssOut.str();

    // 0 = off, 1 = on
    if (this->safeguardChannel < 0 && this->safeguardChannel > 1) {
        pantheios::log_ERROR("MAIN| Error: Safeguard Status not in acceptable range [0-1] => ", sOut);
        return (-1);
    }

    pantheios::log_DEBUG("MAIN| -> Safeguard Status in correct bounds [0-1] => ", sOut);
    // ---------------------------

    // Check if client_cPort is in correct bounds 1-65535
    // ---------------------------
    ssOut.str("");
    ssOut << this->client_cPort;
    sOut = ssOut.str();

    // 1 - 65535
    if (this->client_cPort < 0 && this->client_cPort > 65535) {
        pantheios::log_ERROR("MAIN| Error: Client control port not in acceptable range [1-65535] => ", sOut);
        return (-1);
    }

    pantheios::log_DEBUG("MAIN| -> Client control port in correct bounds [1-65535] => ", sOut);
    // ---------------------------

    // Check if client_mPort is in correct bounds 1-65535
    // ---------------------------
    ssOut.str("");
    ssOut << this->client_mPort;
    sOut = ssOut.str();

    // 1 - 65535
    if (this->client_mPort < 0 && this->client_mPort > 65535) {
        pantheios::log_ERROR("MAIN| Error: Client control port not in acceptable range [1-65535] => ", sOut);
        return (-1);
    }

    pantheios::log_DEBUG("MAIN| -> Client measurement port in correct bounds [1-65535] => ", sOut);
    // ---------------------------

    // Check if client_sPort is in correct bounds 1-65535
    // ---------------------------
    ssOut.str("");
    ssOut << this->client_sPort;
    sOut = ssOut.str();

    // 1 - 65535
    if (this->client_sPort < 0 && this->client_sPort > 65535) {
        pantheios::log_ERROR("MAIN| Error: Client control port not in acceptable range [1-65535] => ", sOut);
        return (-1);
    }

    pantheios::log_DEBUG("MAIN| -> Client safeguard port in correct bounds [1-65535] => ", sOut);
    // ---------------------------

    // Check if server_cPort is in correct bounds 1-65535
    // ---------------------------
    ssOut.str("");
    ssOut << this->server_cPort;
    sOut = ssOut.str();

    // 1 - 65535
    if (this->server_cPort < 0 && this->server_cPort > 65535) {
        pantheios::log_ERROR("MAIN| Error: server control port not in acceptable range [1-65535] => ", sOut);
        return (-1);
    }

    pantheios::log_DEBUG("MAIN| -> server control port in correct bounds [1-65535] => ", sOut);
    // ---------------------------

    // Check if server_mPort is in correct bounds 1-65535
    // ---------------------------
    ssOut.str("");
    ssOut << this->server_mPort;
    sOut = ssOut.str();

    // 1 - 65535
    if (this->server_mPort < 0 && this->server_mPort > 65535) {
        pantheios::log_ERROR("MAIN| Error: server control port not in acceptable range [1-65535] => ", sOut);
        return (-1);
    }

    pantheios::log_DEBUG("MAIN| -> server measurement port in correct bounds [1-65535] => ", sOut);
    // ---------------------------

    // Check if server_sPort is in correct bounds 1-65535
    // ---------------------------
    ssOut.str("");
    ssOut << this->server_sPort;
    sOut = ssOut.str();

    // 1 - 65535
    if (this->server_sPort < 0 && this->server_sPort > 65535) {
        pantheios::log_ERROR("MAIN| Error: server control port not in acceptable range [1-65535] => ", sOut);
        return (-1);
    }

    pantheios::log_DEBUG("MAIN| -> server safeguard port in correct bounds [1-65535] => ", sOut);
    // ---------------------------

    // - Resolve Addresses of client control channel -
    // -----------------------------------------------
    string returnString;

    // Try to resolve the interface if present otherwise the IP
    if (this->nodeRole == 0 && !this->client_cInterface.empty()) {
        if (this->ipVersion == 0) pantheios::log_DEBUG("MAIN| -> Entering resolveInterface() for client control interface address => ",
                this->client_cInterface, " [IPv4]");
        else if (this->ipVersion == 1) pantheios::log_DEBUG("MAIN| -> Entering resolveInterface() for client control interface address => ",
                this->client_cInterface, " [IPv6]");

        returnString = this->resolveInterface(this->client_cInterface, this->ipVersion);

        if (returnString.empty()) {
            pantheios::log_ERROR("MAIN| ");
            pantheios::log_ERROR("MAIN| Error: Resolving Address of the client control interface (", client_cInterface, ") failed.");
            return (-1);
        }
    } else {
        if (this->ipVersion == 0) pantheios::log_DEBUG("MAIN| -> Entering resolveAddress() for client control channel address => ",
                this->client_cAddress, " [IPv4]");
        else if (this->ipVersion == 1) pantheios::log_DEBUG("MAIN| -> Entering resolveAddress() for client control channel address => ",
                this->client_cAddress, " [IPv6]");

        returnString = this->resolveAddress(this->client_cAddress, this->ipVersion, 0);
    }

    //Check if resolving the address was successful
    if (returnString.compare("error") != 0 || returnString.empty()) {
        // Check if we have more then one result
        if (returnString.find('\t') != string::npos) {
            pantheios::log_ERROR("MAIN| Error: Address from client control channel has more then one IP. Please select one.");
            pantheios::log_ERROR("MAIN|        Input => ", this->client_cAddress);
            pantheios::log_ERROR("MAIN|        Found => ", returnString);
            return (-1);
        } else {
            this->client_cAddress = returnString;
            pantheios::log_NOTICE("MAIN|    -> Client control channel address had been resolved to ", this->client_cAddress);
        }
    } else {
        pantheios::log_ERROR("MAIN| ");
        pantheios::log_ERROR("MAIN| Error: Resolving Address of the client control channel failed.");
        return (-1);
    }
    // -----------------------------------------------

    // - Resolve Addresses of server control channel -
    // -----------------------------------------------
    // Try to resolve the interface if present otherwise the IP
    if (this->nodeRole == 1 && !this->server_cInterface.empty()) {
        if (this->ipVersion == 0) pantheios::log_DEBUG("MAIN| -> Entering resolveInterface() for server control interface address => ",
                this->server_cInterface, " [IPv4]");
        else if (this->ipVersion == 1) pantheios::log_DEBUG("MAIN| -> Entering resolveInterface() for server control interface address => ",
                this->server_cInterface, " [IPv6]");

        returnString = this->resolveInterface(this->server_cInterface, this->ipVersion);

        if (returnString.empty()) {
            pantheios::log_ERROR("MAIN| ");
            pantheios::log_ERROR("MAIN| Error: Resolving Address of the server control interface (", server_cInterface, ") failed.");
            return (-1);
        }
    } else {
        if (this->ipVersion == 0) pantheios::log_DEBUG("MAIN| -> Entering resolveAddress() for server control channel address => ",
                this->server_cAddress, " [IPv4]");
        else if (this->ipVersion == 1) pantheios::log_DEBUG("MAIN| -> Entering resolveAddress() for server control channel address => ",
                this->server_cAddress, " [IPv6]");

        returnString = this->resolveAddress(this->server_cAddress, this->ipVersion, 0);
    }

    //Check if resolving the address was successful
    if (returnString.compare("error") != 0 || returnString.empty()) {
        // Check if we have more then one result
        if (returnString.find('\t') != string::npos) {
            pantheios::log_ERROR("MAIN| Error: Address from server control channel has more then one IP. Please select one.");
            pantheios::log_ERROR("MAIN|        Input => ", this->server_cAddress);
            pantheios::log_ERROR("MAIN|        Found => ", returnString);
            return (-1);
        } else {
            this->server_cAddress = returnString;
            pantheios::log_NOTICE("MAIN|    -> Server control channel address had been resolved to ", this->server_cAddress);
        }
    } else {
        pantheios::log_ERROR("MAIN| ");
        pantheios::log_ERROR("MAIN| Error: Resolving Address of the server control channel failed.");
        return (-1);
    }
    // -----------------------------------------------

    // - Resolve Addresses of client measurement channel -
    // ---------------------------------------------------
    if (this->client_mAddress.length() > 0 || (this->nodeRole == 0 && !this->client_mInterface.empty())) {
        // Try to resolve the interface if present otherwise the IP
        if (!this->client_mInterface.empty()) {
            if (this->ipVersion == 0) pantheios::log_DEBUG("MAIN| -> Entering resolveInterface() for client measurement interface address => ",
                    this->client_mInterface, " [IPv4]");
            else if (this->ipVersion == 1) pantheios::log_DEBUG("MAIN| -> Entering resolveInterface() for client measurement interface address => ",
                    this->client_mInterface, " [IPv6]");

            returnString = this->resolveInterface(this->client_mInterface, this->ipVersion);

            if (returnString.empty()) {
                pantheios::log_ERROR("MAIN| ");
                pantheios::log_ERROR("MAIN| Error: Resolving Address of the client measurement interface (", client_mInterface, ") failed.");
                return (-1);
            }
        } else {
            if (this->ipVersion == 0) pantheios::log_DEBUG("MAIN| -> Entering resolveAddress() for client measurement channel address => ",
                    this->client_mAddress, " [IPv4]");
            else if (this->ipVersion == 1) pantheios::log_DEBUG("MAIN| -> Entering resolveAddress() for client measurement channel address => ",
                    this->client_mAddress, " [IPv6]");

            returnString = this->resolveAddress(this->client_mAddress, this->ipVersion, 1);
        }

        //Check if resolving the address was successful
        if (returnString.compare("error") != 0 || returnString.empty()) {
            // Check if we have more then one result
            if (returnString.find('\t') != string::npos) {
                pantheios::log_ERROR("MAIN| Error: Address from client measurement channel has more then one IP. Please select one.");
                pantheios::log_ERROR("MAIN|        Input => ", this->client_mAddress);
                pantheios::log_ERROR("MAIN|        Found => ", returnString);
                return (-1);
            } else {
                this->client_mAddress = returnString;
                pantheios::log_NOTICE("MAIN|    -> Client measurement channel address had been resolved to ", this->client_mAddress);
            }
        } else {
            pantheios::log_ERROR("MAIN| ");
            pantheios::log_ERROR("MAIN| Error: Resolving Address of the client measurement channel failed.");
            return (-1);
        }
    } else {
        this->client_mAddress = this->client_cAddress;
        pantheios::log_DEBUG("MAIN| -> Client measurement channel was set to control channel address => ", this->client_mAddress);
    }
    // -----------------------------------------------

    // - Resolve Addresses of server measurement channel -
    // ---------------------------------------------------
    if (this->server_mAddress.length() > 0 || (!this->server_mInterface.empty() && this->nodeRole == 1)) {
        // Try to resolve the interface if present otherwise the IP
        if (!this->server_mInterface.empty()) {
            if (this->ipVersion == 0) pantheios::log_DEBUG("MAIN| -> Entering resolveInterface() for server measurement interface address => ",
                    this->server_mInterface, " [IPv4]");
            else if (this->ipVersion == 1) pantheios::log_DEBUG("MAIN| -> Entering resolveInterface() for server measurement interface address => ",
                    this->server_mInterface, " [IPv6]");

            returnString = this->resolveInterface(this->server_mInterface, this->ipVersion);

            if (returnString.empty()) {
                pantheios::log_ERROR("MAIN| ");
                pantheios::log_ERROR("MAIN| Error: Resolving Address of the server measurement interface (", server_mInterface, ") failed.");
                return (-1);
            }
        } else {
            if (this->ipVersion == 0) pantheios::log_DEBUG("MAIN| -> Entering resolveAddress() for server measurement channel address => ",
                    this->server_mAddress, " [IPv4]");
            else if (this->ipVersion == 1) pantheios::log_DEBUG("MAIN| -> Entering resolveAddress() for server measurement channel address => ",
                    this->server_mAddress, " [IPv6]");

            returnString = this->resolveAddress(this->server_mAddress, this->ipVersion, 1);
        }

        //Check if resolving the address was successful
        if (returnString.compare("error") != 0 || returnString.empty()) {
            // Check if we have more then one result
            if (returnString.find('\t') != string::npos) {
                pantheios::log_ERROR("MAIN| Error: Address from server measurement channel has more then one IP. Please select one.");
                pantheios::log_ERROR("MAIN|        Input => ", this->server_mAddress);
                pantheios::log_ERROR("MAIN|        Found => ", returnString);
                return (-1);
            } else {
                this->server_mAddress = returnString;
                pantheios::log_NOTICE("MAIN|    -> server measurement channel address had been resolved to ", this->server_mAddress);
            }
        } else {
            pantheios::log_ERROR("MAIN| ");
            pantheios::log_ERROR("MAIN| Error: Resolving Address of the server measurement channel failed.");
            return (-1);
        }
    } else {
        this->server_mAddress = this->server_cAddress;
        pantheios::log_DEBUG("MAIN| -> server measurement channel was set to control channel address => ", this->server_mAddress);
    }
    // -----------------------------------------------

    // - Resolve Addresses of client safeguard channel -
    // ---------------------------------------------------
    if (this->client_sAddress.length() > 0 || (this->nodeRole == 0 && !this->client_sInterface.empty())) {
        // Try to resolve the interface if present otherwise the IP
        if (!this->client_sInterface.empty()) {
            if (this->ipVersion == 0) pantheios::log_DEBUG("MAIN| -> Entering resolveInterface() for client safeguard interface address => ",
                    this->client_sInterface, " [IPv4]");
            else if (this->ipVersion == 1) pantheios::log_DEBUG("MAIN| -> Entering resolveInterface() for client safeguard interface address => ",
                    this->client_sInterface, " [IPv6]");

            returnString = this->resolveInterface(this->client_sInterface, this->ipVersion);

            if (returnString.empty()) {
                pantheios::log_ERROR("MAIN| ");
                pantheios::log_ERROR("MAIN| Error: Resolving Address of the client safeguard interface (", client_sInterface, ") failed.");
                return (-1);
            }
        } else {
            if (this->ipVersion == 0) pantheios::log_DEBUG("MAIN| -> Entering resolveAddress() for client safeguard channel address => ",
                    this->client_sAddress, " [IPv4]");
            else if (this->ipVersion == 1) pantheios::log_DEBUG("MAIN| -> Entering resolveAddress() for client safeguard channel address => ",
                    this->client_sAddress, " [IPv6]");

            returnString = this->resolveAddress(this->client_sAddress, this->ipVersion, 1);
        }

        //Check if resolving the address was successful
        if (returnString.compare("error") != 0 || returnString.empty()) {
            // Check if we have more then one result
            if (returnString.find('\t') != string::npos) {
                pantheios::log_ERROR("MAIN| Error: Address from client safeguard channel has more then one IP. Please select one.");
                pantheios::log_ERROR("MAIN|        Input => ", this->client_sAddress);
                pantheios::log_ERROR("MAIN|        Found => ", returnString);
                return (-1);
            } else {
                this->client_sAddress = returnString;
                pantheios::log_NOTICE("MAIN|    -> Client safeguard channel address had been resolved to ", this->client_sAddress);
            }
        } else {
            pantheios::log_ERROR("MAIN| ");
            pantheios::log_ERROR("MAIN| Error: Resolving Address of the client safeguard channel failed.");
            return (-1);
        }
    } else {
        this->client_sAddress = this->client_cAddress;
        pantheios::log_DEBUG("MAIN| -> Client safeguard channel was set to control channel address => ", this->client_sAddress);
    }
    // -----------------------------------------------

    // - Resolve Addresses of server safeguard channel -
    // ---------------------------------------------------
    if (this->server_sAddress.length() > 0 || (!this->server_sInterface.empty() && this->nodeRole == 1)) {
        // Try to resolve the interface if present otherwise the IP
        if (!this->server_sInterface.empty()) {
            if (this->ipVersion == 0) pantheios::log_DEBUG("MAIN| -> Entering resolveInterface() for server safeguard interface address => ",
                    this->server_sInterface, " [IPv4]");
            else if (this->ipVersion == 1) pantheios::log_DEBUG("MAIN| -> Entering resolveInterface() for server safeguard interface address => ",
                    this->server_sInterface, " [IPv6]");

            returnString = this->resolveInterface(this->server_sInterface, this->ipVersion);

            if (returnString.empty()) {
                pantheios::log_ERROR("MAIN| ");
                pantheios::log_ERROR("MAIN| Error: Resolving Address of the server safeguard interface (", server_sInterface, ") failed.");
                return (-1);
            }
        } else {
            if (this->ipVersion == 0) pantheios::log_DEBUG("MAIN| -> Entering resolveAddress() for server safeguard channel address => ",
                    this->server_sAddress, " [IPv4]");
            else if (this->ipVersion == 1) pantheios::log_DEBUG("MAIN| -> Entering resolveAddress() for server safeguard channel address => ",
                    this->server_sAddress, " [IPv6]");

            returnString = this->resolveAddress(this->server_sAddress, this->ipVersion, 1);
        }

        //Check if resolving the address was successful
        if (returnString.compare("error") != 0 || returnString.empty()) {
            // Check if we have more then one result
            if (returnString.find('\t') != string::npos) {
                pantheios::log_ERROR("MAIN| Error: Address from server safeguard channel has more then one IP. Please select one.");
                pantheios::log_ERROR("MAIN|        Input => ", this->server_sAddress);
                pantheios::log_ERROR("MAIN|        Found => ", returnString);
                return (-1);
            } else {
                this->server_sAddress = returnString;
                pantheios::log_NOTICE("MAIN|    -> server safeguard channel address had been resolved to ", this->server_sAddress);
            }
        } else {
            pantheios::log_ERROR("MAIN| ");
            pantheios::log_ERROR("MAIN| Error: Resolving Address of the server safeguard channel failed.");
            return (-1);
        }
    } else {
        this->server_sAddress = this->server_cAddress;
        pantheios::log_DEBUG("MAIN| -> server safeguard channel was set to control channel address => ", this->server_sAddress);
    }
    // -----------------------------------------------

    pantheios::log_DEBUG("MAIN| Leaving checkSanityOfInput()");
    pantheios::log_DEBUG("MAIN| ");

    return (0);
}

// ------------

/**
 *	Checks if the master/client address fits IPv4/IPv6/DNS format and resolve the address corresponding to the IP protocol version.
 *
 * @param inputAddress	AN IPv4/IPv6/DNS Address
 * @param IPversion		0 for IPv4, 1 for IPv6
 * @param Protocol		0 for TCP, 1 for UDP
 *  @return IPv4 or IPv6 address as string on success. "error" on error
 */
string inputHandler::resolveAddress(string inputAddress, uint32_t IPversion, uint32_t Protocol) {
    struct addrinfo resolveConfig; // Configuration for getaddrinfo()
    struct addrinfo *resultList; // Result list from getaddrinfo()
    struct addrinfo *resultListFirstElement; // Remember the first Element to free the memory later
    string resultString;
    int resolveError;

    // Set struct to Zero and all its fields
    memset(&resolveConfig, 0, sizeof (resolveConfig));

    // Set IP Version
    switch (IPversion) {
        case 0:
            resolveConfig.ai_family = AF_INET;
            break; // IPv4
        case 1:
            resolveConfig.ai_family = AF_INET6;
            break; // IPv6

        default:
            resolveConfig.ai_family = AF_UNSPEC;
            break; // autoselect - input error ...
    }
    if (Protocol == 0) // TCP
        resolveConfig.ai_socktype = SOCK_STREAM;
    else // UDP
        resolveConfig.ai_socktype = SOCK_DGRAM;

    // Try to resolve the given address
    resolveError = getaddrinfo(inputAddress.c_str(), NULL, &resolveConfig, &resultList);
    if (resolveError != 0) {
        pantheios::log_ERROR("MAIN| Error: ", gai_strerror(resolveError));

        return ("error");
    }

    // Remember first Element of the list, needed to free the memory when we are done
    resultListFirstElement = resultList;

    // Iterate through the results
    while (resultList != NULL) {
        void *address;
        char ipstr[INET6_ADDRSTRLEN];

        if (resultList->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *) resultList->ai_addr;
            address = &(ipv4->sin_addr);
        } else if (resultList->ai_family == AF_INET6) { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *) resultList->ai_addr;
            address = &(ipv6->sin6_addr);
        } else {
            return ("error");
        }

        // convert the IP to a string and remember it
        inet_ntop(resultList->ai_family, address, ipstr, sizeof (ipstr));
        resultString += (string) ipstr;

        // Go to the next result, if its NULL then we are done
        resultList = resultList->ai_next;

        // Only add separator when more results are present
        if (resultList != NULL) {
            resultString += "\t";
        }
    }

    // Free allocated memory
    freeaddrinfo(resultListFirstElement);

    return (resultString);
}

string inputHandler::resolveInterface(string inputInterface, uint32_t IPversion) {
    struct ifaddrs *addressListing, *currentElement;
    struct sockaddr_in *sa_ip4;
    struct sockaddr_in6 *sa_ip6;
    string resultString("");

    // Set buffer to zero
    char buffer_ip4[32];
    bzero(buffer_ip4, sizeof (buffer_ip4) * 1);
    char buffer_ip6[32];

    // Get local interfaces
    if (getifaddrs(&addressListing) == -1) return ("error");

    // Check if one Interface matches the name and get ip(s) for it
    for (currentElement = addressListing; currentElement != NULL; currentElement = currentElement->ifa_next) {
        // Check if we have a match in the interface name
        if (((string) currentElement->ifa_name).compare(inputInterface) == 0 && (currentElement->ifa_flags & IFF_UP)) {
            // Check if IPversion is correct and collect IPs for Interface
            if (IPversion == 0 && currentElement->ifa_addr->sa_family == AF_INET) {
                sa_ip4 = (struct sockaddr_in *) (currentElement->ifa_addr);
                inet_ntop(currentElement->ifa_addr->sa_family, (void *) &(sa_ip4->sin_addr), buffer_ip4, sizeof (buffer_ip4));

                if (resultString.empty()) resultString += (string) buffer_ip4;
                else {
                    resultString += "\t";
                    resultString += (string) buffer_ip4;
                }
            } else if (IPversion == 1 && currentElement->ifa_addr->sa_family == AF_INET6) {
                sa_ip6 = (struct sockaddr_in6 *) (currentElement->ifa_addr);
                inet_ntop(currentElement->ifa_addr->sa_family, (void *) &(sa_ip6->sin6_addr), buffer_ip6, sizeof (buffer_ip6));

                if (resultString.empty()) resultString += (string) buffer_ip6;
                else {
                    resultString += "\t";
                    resultString += (string) buffer_ip6;
                }
            }
        }
    }
    freeifaddrs(addressListing);

    return (resultString);
}

measurementMethodClass* inputHandler::create_measurementMethodClassById(int id) {
    vector<measurementMethodClass*> vect = get_measurementMethods();
    int j = -1;
    for (unsigned int i = 0; i < vect.size(); i++) {
        if (vect.at(i)->get_Id() == id) {
            j = i;
        } else {
            delete vect.at(i);
        }
    }
    if (j != -1) return vect.at(j);
    else return NULL;
}

vector<measurementMethodClass*> inputHandler::get_measurementMethods() {
    vector<measurementMethodClass*> vList(0);
    //	vList.push_back(new measureBasic());
    //	vList.push_back(new measureAssolo());
    //	vList.push_back(new MeasureMobile());
    //	vList.push_back(new measureNew());
    //	vList.push_back(new MeasureAlgo());
    //std::cout << "\n Size of measureTBUS = " << sizeof(measureTBUS) <<"\n" << flush;
    //std::cout << "\n New measureTBUS()\n\n" << flush;
    vList.push_back(new measureTBUS());
    //std::cout << "\n New measureTBUS() done \n\n" << flush;
    return vList;
}

