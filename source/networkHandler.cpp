/* networkHandler.cpp */
#include "networkHandler.h"

// External logging framework
#include "pantheios/pantheiosHeader.h"
//some usings to make the code look shorter
using pantheios::log_ERROR;
using pantheios::log_NOTICE;
using pantheios::log_INFORMATIONAL;
using pantheios::log_DEBUG;
using pantheios::integer;
using pantheios::real;
using pantheios::boolean;

#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h> //addrinfo
#include <unistd.h> //close

pthread_mutex_t networkHandler_mSendingQueue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t networkHandler_mSentQueue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t networkHandler_mReceivingQueue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t networkHandler_sSendingQueue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t networkHandler_sSentQueue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t networkHandler_sReceivingQueue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t networkHandler_dynamicPayload = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t networkHandler_safeguardDynamicPayload = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t networkHandler_measurementDeviceQueue = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t networkHandler_cond_measurementDeviceQueue = PTHREAD_COND_INITIALIZER;

pthread_mutex_t networkHandler_mDDSLock = PTHREAD_MUTEX_INITIALIZER; // Mutex for locking the Sender-Datadistributer Thread. Will be unlocked if there is new Data in the queue. 
pthread_mutex_t networkHandler_mDDRLock = PTHREAD_MUTEX_INITIALIZER; // Mutex for locking the Receiver-Datadistributer Thread. Will be unlocked if there is new Data in the queue

bool mSentQueue_ready_for_log = false;
pthread_cond_t networkHandler_cond_mSentQueue_1_ready_for_log = PTHREAD_COND_INITIALIZER;
bool mReceivingQueue_ready_for_log = false;
pthread_cond_t networkHandler_cond_mReceivingQueue_has_data = PTHREAD_COND_INITIALIZER;
bool sSendingQueue_has_data = false;
pthread_cond_t networkHandler_cond_sSendingQueue_has_data = PTHREAD_COND_INITIALIZER;
bool sSendingQueue_has_room = true;
pthread_cond_t networkHandler_cond_sSendingQueue_has_room = PTHREAD_COND_INITIALIZER;
bool sReceivingQueue_has_data = false;
pthread_cond_t networkHandler_cond_sReceivingQueue_has_data = PTHREAD_COND_INITIALIZER;
bool mSendingQueue_has_data = false;
pthread_cond_t networkHandler_cond_mSendingQueue_has_data = PTHREAD_COND_INITIALIZER;
bool mSendingQueueIsEmpty = true;
pthread_cond_t networkHandler_cond_mSendingQueueIsEmpty = PTHREAD_COND_INITIALIZER;


pthread_mutex_t networkHandler_sSafeguardSender = PTHREAD_MUTEX_INITIALIZER; //used as a fake mutek for pthread_cond_timedwait
pthread_cond_t networkHandler_cond_sSafeguardEmergencySend = PTHREAD_COND_INITIALIZER; //used to request the immediate sending of a safeguard message

networkHandler::networkHandler() {
    initValues();
}

networkHandler::~networkHandler() {
    mSendingQueue.deleteQueue();
    mSentQueue.deleteQueue();
    mReceivingQueue.deleteQueue();
    sSendingQueue.deleteQueue();
    sReceivingQueue.deleteQueue();
    if (routingControl != NULL) {
        delete routingControl;
    }
    if (routingSafeguard != NULL)delete routingSafeguard;
    if (routingMeasurement != NULL)delete routingMeasurement;
    closeSockets();
}

/**
 * Initializes the object variables either to their null values
 * or to the default ones.
 *
 * @return 0 on success
 */
int networkHandler::initValues() {
    log_DEBUG("MAIN| Entering initValues()");
    clientSocketFD_Control_TCP = 0;
    clientSocketFD_Measure_UDP = 0;
    clientSocketFD_Safe_UDP = 0;
    serverListenSocketFD_Control_TCP = 0;
    serverListenSocketFD_Measure_UDP = 0;
    serverListenSocketFD_Safe_UDP = 0;
    log_DEBUG("MAIN| -> Setting all Sockets to initValue");
    flag_emergencySTOP = false;
    flag_mSendingThreadReady = false;
    flag_senderDataDistributerReady = false;
    flag_mReceivingThreadReady = false;
    flag_receiverDataDistributerReady = false;
    flag_sSendingThreadReady = false;
    flag_sReceivingThreadReady = false;
    flag_mReceivingThreadDone = false;
    flag_mSendingThreadDone = false;
    flag_receiverDataDistributerDone = false;
    flag_senderDataDistributerDone = false;
    flag_sSendingThreadDone = false;
    flag_sReceivingThreadDone = false;
    flag_finishedFillingSendingQueue = false;
    flag_sendingDone = false;
    flag_measureReceivingDone = false;
    flag_safeguardReceivingDone = false;
    //DEPRECATED flag_receivingQueueToUse = 1;

    MeasurementChannelForeignIP = "";
    SafeguadChannelForeignIP = "";
    mSentQueue_ready_for_log = false;
    mReceivingQueue_ready_for_log = false;
    sSendingQueue_has_data = false;
    sSendingQueue_has_room = true;
    sReceivingQueue_has_data = false;
    log_DEBUG("MAIN| -> Set all flags to default value");
    // FIXME langech
    // It may be better to safe these values in the network object.
    // It may also be a good idea to set these values to zero in this function
    // and to the real value in the sync method
    maxWaitSecondsRead = inputH.get_maxWaitForSocketRead();
    maxWaitSecondsWrite = inputH.get_maxWaitForSocketWrite();
    // ErrorCounter
    readErrorCounter_safeguardUDP = 0;
    writeErrorCounter_safeguardUDP = 0;
    flag_mSendingThreadCurrentlyWaitingForQueue = false;

    routingControl = NULL;
    routingMeasurement = NULL;
    routingSafeguard = NULL;

    //as a standard setting we do not throttle the sending rate of the measurement channel
    maxSendingRateMeasurementChannel = 100 * 1000 * 1000; //100mbit/s
    throttleSendingRateMeasurementChannel = false;

    //	initSendingMaxQueueSizeCheck();
    if (inputH.get_maxDeviceQueueSizePercentage() > -1) {
        string interface;
        if (inputH.get_nodeRole() == 0) { // Client
            interface = inputH.get_client_mInterface();
        } else {
            interface = inputH.get_server_mInterface();
        }
        if (!interface.empty()) {
            int maxLength = getMaxDeviceQueueSizeForInterface(interface.c_str());
            if (maxLength > -1) {
                mSendingMaxQueueSize = ((float) inputH.get_maxDeviceQueueSizePercentage() / 100.0) * maxLength;
                log_NOTICE("Networkhandler Init | Using maximum device queue length: ", integer(mSendingMaxQueueSize));
            }
        } else {
            //TODO: move it to the input handling
            log_ERROR("Networkhandler Init | No Interfaceparameter given to calculate the maximum device queue length for the measurement channel. We won't use it in this run.");
        }
    } else {
        mSendingMaxQueueSize = inputH.get_maxDeviceQueueSize();
        log_DEBUG("Networkhandler Init | Using maximum device queue length: ", integer(mSendingMaxQueueSize));
    }

    log_DEBUG("MAIN| Leaving initValues()");
    return 0;
}

/**
 * This function tries to close all possible socket connections.
 * It does NOT check the return values.
 */
void networkHandler::closeSockets() {
    log_DEBUG("MAIN| Entering closeSockets()");
    if (inputH.amIServer()) {
        if (serverListenSocketFD_Measure_UDP > 0) {
            usleep(3000000L); //sleep 3 seconds to allow the client to close the connection properly
        }
        log_NOTICE("Closing serverListenSocketFD_Control_TCP ret=", integer(close(serverListenSocketFD_Control_TCP)));
        log_NOTICE("Shutdown serverListenSocketFD_Measure_UDP ret=", integer(shutdown(serverListenSocketFD_Measure_UDP, SHUT_RDWR)));
        log_NOTICE("Shutdown serverListenSocketFD_Safe_UDP ret=", integer(shutdown(serverListenSocketFD_Safe_UDP, SHUT_RDWR)));
        log_NOTICE("Closing serverListenSocketFD_Measure_UDP ret=", integer(close(serverListenSocketFD_Measure_UDP)));
        log_NOTICE("Closing serverListenSocketFD_Safe_UDP ret=", integer(close(serverListenSocketFD_Safe_UDP)));
        log_NOTICE("Closing serverSocketFD_Control_TCP ret=", integer(close(serverSocketFD_Control_TCP)));
    } else {
        log_NOTICE("Shutdown clientSocketFD_Measure_UDP ret=", integer(shutdown(clientSocketFD_Measure_UDP, SHUT_RDWR))); //TODO: check if this really solves the hanging recv 
        log_NOTICE("Shutdown clientSocketFD_Safe_UDP ret=", integer(shutdown(clientSocketFD_Safe_UDP, SHUT_RDWR)));
        log_NOTICE("Closing clientSocketFD_Control_TCP ret=", integer(close(clientSocketFD_Control_TCP)));
        log_NOTICE("Closing clientSocketFD_Measure_UDP ret=", integer(close(clientSocketFD_Measure_UDP))); //TODO: check if this really solves the hanging recv 
        log_NOTICE("Closing clientSocketFD_Safe_UDP ret=", integer(close(clientSocketFD_Safe_UDP)));
    }

    log_DEBUG("MAIN| Leaving closeSockets()");
}

/**
 * Sends a message to client or server depending on node role.
 * The message can't be greater then MAX_CONTROLCHANNEL_MESSAGE_LENGTH
 * defined in networkThreadStructs.h
 *
 * @param nodeRole	0 server, 1 client
 * @param message	a string which will be send to the other node
 * @param seconds	when the method is called
 * @param caller	thread who called
 * @return	0 on success, -1 on error, -2 if socket was not ready to write
 */
int networkHandler::writeToControlChannel(const uint32_t &nodeRole,
        const string &message, const uint32_t &seconds, const string &caller) {
    // FIXME langech
    // this is a tcp connection, so there is no limit on the message length
    // but this helps us to control the buffer size
    if (message.length() > MAX_CONTROL_CHANNEL_MESSAGE_LENGTH) {
        log_ERROR(caller,
                "| Error: message length in writeToControlChannel too big.");
        return -1;
    }
    if (nodeRole == 1) { // server
        if (checkIfSocketIsReadyToWrite(serverSocketFD_Control_TCP, seconds, caller) < 1) {
            log_ERROR(caller,
                    "| Error: clientListenSocketFD_Control_TCP not ready to write.");
            return -2;
        }
        // FIXME langech
        // use send instead of write
        // check the return value. It may happen that we did not send enough bytes.
        if (write(serverSocketFD_Control_TCP, message.c_str(), message.length()) == -1) {
            log_ERROR(caller,
                    "| Error: Write() on clientListenSocketFD_Control_TCP failed.");
            return -1;
        }
        log_NOTICE(caller, "| -> Sent message to client: \"", message, "\"");
    } else if (nodeRole == 0) { // client
        if (clientSocketFD_Control_TCP == -1) {
            log_ERROR(caller,
                    "| Error: clientSocketFD_Control_TCP not initialized ?");
            return -1;
        }
        if (checkIfSocketIsReadyToWrite(clientSocketFD_Control_TCP, seconds, caller) < 1) {
            log_ERROR(caller,
                    "| Error: clientSocketFD_Control_TCP not ready to write.");
            return -2;
        }
        // FIXME langech
        // use send instead of write
        // check the return value. It may happen that we did not send enough bytes.
        if (write(clientSocketFD_Control_TCP, message.c_str(), message.length()) == -1) {
            log_ERROR(caller,
                    "| Error: Write() on clientSocketFD_Control_TCP failed.");
            return -1;
        }
        log_NOTICE(caller, "| -> Sent message to server: \"", message, "\"");
    } else {
        log_ERROR(caller,
                "| Error: Unknown node role in writeToControlChannel()");
        return -1;
    }
    return 0;
}

/**
 * Read Messages from Control Channel
 *
 * @param nodeRole	Are we readding messages as client or server
 * @param seconds	Wait this time and then exit with error
 * @param caller	Will output this as Threadname in Logging
 *
 * @return			received Message on success, "error_read_EOF" on EOF of this socket, "error_read_rdy" on socket was not ready to read
 */

// FIXME langech
// We need to test the return value of the read call
// cause it is possible to get less or more bytes then awaited on
// the tcp channel cause it is a stream

string networkHandler::readFromControlChannel(const uint32_t &nodeRole,
        const uint32_t &seconds, const string &caller, const bool logerror) {
    char buffer[MAX_CONTROL_CHANNEL_MESSAGE_LENGTH];
    memset(&buffer, 0, MAX_CONTROL_CHANNEL_MESSAGE_LENGTH);
    int receivedMessageLength = 0;
    string receivedMessage = "error";
    if (nodeRole == 1) { // server
        if (serverSocketFD_Control_TCP == -1) {
            if (logerror) {
                log_ERROR(caller,
                        "| Error: clientListenSocketFD_Control_TCP not initialized ?");
            }
            return "error";
        }
        if (checkIfSocketIsReadyToRead(serverSocketFD_Control_TCP, seconds, caller) < 1) {
            if (logerror) {
                log_ERROR(caller,
                        "| Error: clientListenSocketFD_Control_TCP not ready to read.");
            }
            return "error_read_rdy";
        }
        receivedMessageLength = read(serverSocketFD_Control_TCP, buffer, MAX_CONTROL_CHANNEL_MESSAGE_LENGTH - 1);
        if (receivedMessageLength < 0) {
            // FIXME langech
            // why no return?
            if (logerror) {
                log_ERROR(caller, "| Error: Read() on serverSocketFD_Control_TCP failed.");
            }
        } else if (receivedMessageLength == 0) {
            if (logerror) {
                log_ERROR(caller, "| Error: Read() on serverSocketFD_Control_TCP = EOF.");
            }
            network.set_emergencySTOP("Read() on serverSocketFD_Control_TCP with length=0");
            return "error_read_EOF";
        } else {
            receivedMessage = (string) buffer;
            log_NOTICE(caller, "| -> Received message from client: \"", receivedMessage, "\"");
        }
    } else if (nodeRole == 0) { // client
        // Check if socket is ready
        if (clientSocketFD_Control_TCP == -1) {
            if (logerror) {
                log_ERROR(caller, "| Error: clientSocketFD_Control_TCP not initialized ?");
            }
            return "error";
        }
        if (checkIfSocketIsReadyToRead(clientSocketFD_Control_TCP, seconds, caller) < 1) {
            if (logerror) {
                log_ERROR(caller, "| Error: clientSocketFD_Control_TCP not ready.");
            }
            return "error_read_rdy";
        }
        receivedMessageLength = read(clientSocketFD_Control_TCP, buffer, MAX_CONTROL_CHANNEL_MESSAGE_LENGTH - 1);
        if (receivedMessageLength < 0) {
            // FIXME langech
            // why no return?
            if (logerror) {
                log_ERROR(caller, "| Error: Read() on clientSocketFD_Control_TCP failed.");
            }
        } else if (receivedMessageLength == 0) {
            if (logerror) {
                log_ERROR(caller, "| Error: Read() on clientSocketFD_Control_TCP = EOF.");
            }
            network.set_emergencySTOP("Read() on clientSocketFD_Control_TCP with length = 0");
            return ("error_read_EOF");
        } else {
            receivedMessage = (string) buffer;
            log_NOTICE(caller, "| -> Received message from server: \"", receivedMessage, "\"");
        }
    } else {
        if (logerror) {
            log_ERROR(caller, "| Error: Unknown node role in readFromControlChannel()");
        }
        return "error";
    }
    return receivedMessage;
}

/**
 * Extracts an integer value out of a parameter string.
 *
 * @param parameter, reference to the parameter string
 * @param retval, pointer to integer return value
 *
 * @return 0, if everything is ok
 * @return -1, if an error occured
 */
int networkHandler::ParseInteger(const std::string &parameter, int *retval) {
    const char *parameter_cstring = parameter.c_str();
    int parameter_length = parameter.length();
    int i = 0;
    while (isdigit(parameter_cstring[i]) && (i < parameter_length))
        ++i;
    if (i != parameter_length) {
        log_ERROR("MEME| ERROR: Not an integer value.");
        return -1; // not an integer value
    }
    *retval = atoi(parameter_cstring);
    return 0; // integer value found
}

/**
 * Sends an integer message including the general parameters to the server
 *
 * @ return 0 on success, -1 on error
 */
int networkHandler::SendSyncMessage(const uint32_t &measurementMethod,
        const uint32_t &measuredLink, const uint32_t &experimentDuration,
        const uint32_t &safeguardChannel, const uint32_t &client_mPort,
        const uint32_t &client_sPort, const uint32_t &server_mPort,
        const uint32_t &server_sPort, const string &client_mAddress,
        const string &client_sAddress, const string &server_mAddress,
        const string &server_sAddress, const string &inputFilenameFormat) {
    // Generate magic numbers
    // FIXME langech
    // There should be a better place for this
    timeHandler timing(inputH.get_timingMethod());
    timing.probeTime();
    srand(static_cast<unsigned int> (timing.getLastProbedTime().tv_sec));
    uint32_t measurement_magic = rand() % 4294967294u + 1;
    uint32_t safeguard_magic = rand() % 4294967294u + 1;
    inputH.set_measurement_magic(measurement_magic);
    inputH.set_safeguard_magic(safeguard_magic);
    stringstream syncData;
    syncData << "BASIC_SYNC|" << measurementMethod << "|" <<
            measuredLink << "|" << experimentDuration << "|" << safeguardChannel <<
            "|" << client_mPort << "|" << client_sPort << "|" << server_mPort <<
            "|" << server_sPort << "|" << sizeof (uint32_t) << "|" << client_mAddress <<
            "|" << client_sAddress << "|" << measurement_magic << "|" <<
            safeguard_magic << "|" << inputFilenameFormat << "|";
    if (writeToControlChannel(0, syncData.str(), 3, "MAIN") == -1) {
        log_ERROR("MAIN| ERROR: Sending basic sync data failed.");
        return -1;
    }
    return 0;
}

/**
 * Tries to receive a confirmation message including the size of an uint32_t
 * on the server side.
 *
 * @param return_string, pointer to a string object that should contain the
 * received message after a successfull operation.
 *
 * @return 0 on success, -1 on error
 */
int networkHandler::WaitForSyncConfirmation(string *return_string) {
    // FIXME langech
    // It may be a good idea to include a length field to the sync string
    // otherwise we may not receive all data
    string receivedMessage = readFromControlChannel(0, 0, "MAIN");
    int attemps = 0;
    while (receivedMessage.substr(0, 11).compare("BASIC_SYNS|") != 0) {
        if (receivedMessage.compare("error_read_EOF") == 0) {
            log_ERROR("MAIN| Error: Control channel socket reached EOF.");
            return -1;
        }
        if (attemps > 20) {
            log_ERROR("MAIN| Error: No BASIC_SYNS message received from client after 60 seconds: ");
            return -1;
        }
        if (clientSocketFD_Control_TCP == -1) {
            log_ERROR("MAIN| Error: clientSocketFD_Control_TCP not initialized ?");
            return -1;
        }
        if (checkIfSocketIsReadyToRead(clientSocketFD_Control_TCP, 3, "MAIN") < 1)
            log_ERROR("MAIN| -> Waiting 3 seconds for server to reply with BASIC_SYNS.");
        receivedMessage = readFromControlChannel(0, 0, "MAIN");
    }
    *return_string = receivedMessage;
    return 0;
}

/**
 * Parses the integer size parameter of the synchronization string.
 * Sets the parameter in the input object on success.
 *
 * @param received_message, reference to the received string message
 *
 * @retun 0 on success, -1 on error
 */
int networkHandler::ParseServerIntegerSize(const string &received_message) {
    if (received_message.size() < 12)
        return -1;
    if (received_message.substr(0, 11).compare("BASIC_SYNS|") != 0)
        return -1;
    if (received_message.substr(11, received_message.length()).find("|") == string::npos)
        return -1;
    uint32_t beforeLastSeparator = received_message.substr(11, received_message.length()).find("|");
    if (beforeLastSeparator == string::npos)
        return -1;
    int server_integer_size;
    string inputstring = received_message.substr(11, beforeLastSeparator);
    ParseInteger(inputstring, &server_integer_size);
    log_DEBUG("MAIN| -> Setting server uint32_t size to ", inputstring);
    if (inputH.set_uint32_t_size(static_cast<uint32_t> (server_integer_size)) == -1) {
        log_DEBUG("   -> failed ");
        return -1;
    }
    return 0;
}

/**
 * Parses the measurement method of the synchronization string.
 * Sets the parameter in the output object on success.
 *
 * @param received_message, reference to the received string message
 *
 * @return 0 on success, -1 on error
 */
int networkHandler::ParseMethod(const string &received_message) {
    uint32_t received_method = static_cast<uint32_t> (received_message[11] - 48);
    inputH.set_measurementMethodClass(inputH.create_measurementMethodClassById(received_method));
    return 0;
}

/**
 * Parses the measurement method of the synchronization string.
 * Sets the parameter in the input object on success.
 *
 * @param received_message, reference to the received string message
 *
 * @return 0 on success, -1 on error
 */
int networkHandler::ParseMeasuredLink(const string &received_message) {
    uint32_t received_link = static_cast<uint32_t> (received_message[13] - 48);
    switch (received_link) {
        case 0:
            log_DEBUG("MAIN| Setting measured link to \"up\"");
            if (inputH.set_measuredLink(0) == -1) {
                log_DEBUG("   -> failed ");
                return -1;
            }
            break;
        case 1:
            log_DEBUG("MAIN| Setting measured link to \"down\"");
            if (inputH.set_measuredLink(1) == -1) {
                log_DEBUG("   -> failed ");
                return -1;
            }
            break;
        case 2:
            log_DEBUG("MAIN| Setting measured link to \"both\"");
            if (inputH.set_measuredLink(2) == -1) {
                log_DEBUG("   -> failed ");
                return -1;
            }
            break;
        default:
            log_ERROR("MAIN| ERROR: Received measurement link is not valid.");
            return -1;
    }
    return 0;
}

/**
 * Parses the experiment duration of the synchronization string.
 * Sets the parameter in the input object.
 *
 * @param received, pointer to the string containing the parameter
 *
 * @return 0 on success, -1 on error
 */
int networkHandler::ParseExperimentDuration(string *received) {
    int experiment_duration;
    size_t beforeLastSeparator = (*received).find("|");
    if (beforeLastSeparator == string::npos)
        return -1;
    if (ParseInteger((*received).substr(0, beforeLastSeparator),
            &experiment_duration))
        return -1;
    log_DEBUG("MAIN| Setting experiment duration to \"",
            integer(experiment_duration), "\"");
    if (inputH.set_experimentDuration(static_cast<uint32_t> (experiment_duration)) == -1) {
        log_DEBUG("   -> failed ");
        return -1;
    }
    *received = (*received).substr(beforeLastSeparator + 1);
    return 0;
}

/**
 * Parses the safeguard channel status of the synchronization string.
 * Sets the parameter in the input object.
 *
 * @param received, pointer to the string containing the parameter
 *
 * @return 0 on success, -1 on error.
 */
int networkHandler::ParseSafeguardStatus(string *received) {
    int safeguard;
    size_t beforeLastSeparator = (*received).find("|");
    if (beforeLastSeparator == string::npos)
        return -1;
    if (ParseInteger((*received).substr(0, beforeLastSeparator),
            &safeguard))
        return -1;
    log_DEBUG("MAIN| Setting safeguard status to \"",
            integer(safeguard), "\"");
    if (inputH.set_safeguardChannel(static_cast<uint32_t> (safeguard)) == -1) {
        log_DEBUG("MAIN|    -> failed ");
        return -1;
    }
    *received = (*received).substr(beforeLastSeparator + 1);
    return 0;
}

/**
 * Parses the client measurement channel port number of the synchronization
 * string.
 * Sets the parameter in the input object.
 *
 * @param received, pointer to the string containing the parameter
 *
 * @return 0 on success, -1 on error.
 */
int networkHandler::ParseClientMeasureChannelPort(string *received) {
    int port_cm;
    size_t beforeLastSeparator = (*received).find("|");
    if (beforeLastSeparator == string::npos)
        return -1;
    if (ParseInteger((*received).substr(0, beforeLastSeparator),
            &port_cm))
        return -1;
    log_DEBUG("MAIN| Setting client measurement channel port to \"",
            integer(port_cm), "\"");
    if (inputH.set_client_mPort(static_cast<uint32_t> (port_cm)) == -1) {
        log_DEBUG("MAIN|    -> failed ");
        return -1;
    }
    *received = (*received).substr(beforeLastSeparator + 1);
    return 0;
}

/**
 * Parses the client safeguard channel port number of the synchronization
 * string.
 * Sets the parameter in the input object.
 *
 * @param received, pointer to the string containing the parameter
 *
 * @return 0 on success, -1 on error.
 */
int networkHandler::ParseClientSafeguardChannelPort(string *received) {
    int port_cs;
    size_t beforeLastSeparator = (*received).find("|");
    if (beforeLastSeparator == string::npos)
        return -1;
    if (ParseInteger((*received).substr(0, beforeLastSeparator),
            &port_cs))
        return -1;
    log_DEBUG("MAIN| Setting client safeguard channel port to \"",
            integer(port_cs), "\"");
    if (inputH.set_client_sPort(static_cast<uint32_t> (port_cs)) == -1) {
        log_DEBUG("MAIN|    -> failed ");
        return -1;
    }
    *received = (*received).substr(beforeLastSeparator + 1);
    return 0;
}

/**
 * Parses the server measurement channel port number of the synchronization
 * string.
 * Sets the parameter in the input object.
 *
 * @param received, pointer to the string containing the parameter
 *
 * @return 0 on success, -1 on error.
 */
int networkHandler::ParseServerMeasureChannelPort(string *received) {

    int port_sm;
    size_t beforeLastSeparator = (*received).find("|");
    if (beforeLastSeparator == string::npos)
        return -1;
    if (ParseInteger((*received).substr(0, beforeLastSeparator),
            &port_sm))
        return -1;
    log_DEBUG("MAIN| Setting server measurement channel port to \"",
            integer(port_sm), "\"");
    if (inputH.set_server_mPort(static_cast<uint32_t> (port_sm)) == -1) {
        log_DEBUG("MAIN|    -> failed ");
        return -1;
    }
    *received = (*received).substr(beforeLastSeparator + 1);
    return 0;
}

/**
 * Parses the server safeguard channel port number of the synchronization
 * string.
 * Sets the parameter in the input object.
 *
 * @param received, pointer to the string containing the parameter
 *
 * @return 0 on success, -1 on error.
 */
int networkHandler::ParseServerSafeguardChannelPort(string *received) {
    int port_ss;
    size_t beforeLastSeparator = (*received).find("|");
    if (beforeLastSeparator == string::npos)
        return -1;
    if (ParseInteger((*received).substr(0, beforeLastSeparator),
            &port_ss))
        return -1;
    log_DEBUG("MAIN| Setting server measurement channel port to \"",
            integer(port_ss), "\"");
    if (inputH.set_server_sPort(static_cast<uint32_t> (port_ss)) == -1) {
        log_DEBUG("MAIN|    -> failed ");
        return -1;
    }
    *received = (*received).substr(beforeLastSeparator + 1);
    return 0;
}

/**
 * Parses the client uint32_t size of the synchronization string.
 * Sets the parameter in the input object.
 *
 * @param received, pointer to the string containing the parameter
 *
 * @return 0 on success, -1 on error.
 */
int networkHandler::ParseClientIntegerSize(string *received) {
    int client_integer_size;
    size_t beforeLastSeparator = (*received).find("|");
    if (beforeLastSeparator == string::npos)
        return -1;
    if (ParseInteger((*received).substr(0, beforeLastSeparator),
            &client_integer_size))
        return -1;
    log_DEBUG("MAIN| Setting server measurement channel port to \"",
            integer(client_integer_size), "\"");
    if (inputH.set_uint32_t_size(static_cast<uint32_t> (client_integer_size)) == -1) {
        log_DEBUG("MAIN|    -> failed ");
        return -1;
    }
    *received = (*received).substr(beforeLastSeparator + 1);
    return 0;
}

/**
 * Parses the client measurement channel address of the synchronization string.
 * Sets the parameter in the input object.
 *
 * @param received, pointer to the string containing the parameter
 *
 * @return 0 on success, -1 on error.
 */
int networkHandler::ParseClientMeasureChannelAddress(string *received) {
    size_t beforeLastSeparator = (*received).find("|");
    if (beforeLastSeparator == string::npos)
        return -1;
    string client_measure_address = (*received).substr(0, beforeLastSeparator);
    log_DEBUG("MAIN| Setting client measurement address to \"",
            client_measure_address, "\"");
    inputH.set_client_mAddress(client_measure_address);
    *received = (*received).substr(beforeLastSeparator + 1);
    return 0;
}

/**
 * Parses the client safeguard channel address of the synchronization string.
 * Sets the parameter in the input object.
 *
 * @param received, pointer to the string containing the parameter
 *
 * @return 0 on success, -1 on error.
 */
int networkHandler::ParseClientSafeguardChannelAddress(string *received) {
    size_t beforeLastSeparator = (*received).find("|");
    if (beforeLastSeparator == string::npos)
        return -1;
    string client_safeguard_address = (*received).substr(0, beforeLastSeparator);
    log_DEBUG("MAIN| Setting client measurement address to \"",
            client_safeguard_address, "\"");
    inputH.set_client_sAddress(client_safeguard_address);
    *received = (*received).substr(beforeLastSeparator + 1);
    return 0;
}

/**
 * Parses the measurement magic number of the synchronization string.
 * Sets the parameter in the input object.
 *
 * @param received, pointer to the string containing the parameter
 *
 * @return 0 on success, -1 on error.
 */
int networkHandler::ParseMeasurementMagic(string *received) {
    int measure_magic;
    size_t beforeLastSeparator = (*received).find("|");
    if (beforeLastSeparator == string::npos)
        return -1;
    if (ParseInteger((*received).substr(0, beforeLastSeparator),
            &measure_magic))
        return -1;
    log_DEBUG("MAIN| Setting measurement magic to \"",
            integer(measure_magic), "\"");
    if (inputH.set_measurement_magic(static_cast<uint32_t> (measure_magic)) == -1) {
        log_DEBUG("MAIN|    -> failed ");
        return -1;
    }
    *received = (*received).substr(beforeLastSeparator + 1);
    return 0;
}

/**
 * Parses the safeguard magic number of the synchronization string.
 * Sets the parameter in the input object.
 *
 * @param received, pointer to the string containing the parameter
 *
 * @return 0 on success, -1 on error.
 */
int networkHandler::ParseSafeguardMagic(string *received) {
    int safeguard_magic;
    size_t beforeLastSeparator = (*received).find("|");
    if (beforeLastSeparator == string::npos)
        return -1;
    if (ParseInteger((*received).substr(0, beforeLastSeparator),
            &safeguard_magic))
        return -1;
    log_DEBUG("MAIN| Setting safeguardment magic to \"",
            integer(safeguard_magic), "\"");
    if (inputH.set_safeguard_magic(static_cast<uint32_t> (safeguard_magic)) == -1) {
        log_DEBUG("MAIN|    -> failed ");
        return -1;
    }
    *received = (*received).substr(beforeLastSeparator + 1);
    return 0;
}

/**
 * Parses the filenema format of the synchronization string.
 * Sets the parameter in the input object.
 *
 * @param received, pointer to the string containing the parameter
 *
 * @return 0 on success, -1 on error.
 */
int networkHandler::ParseFilenameFormat(string *received) {
    size_t beforeLastSeparator = (*received).find("|");
    if (beforeLastSeparator == string::npos)
        return -1;
    string filename_format = (*received).substr(0, beforeLastSeparator);
    log_DEBUG("MAIN| Setting filename format to \"",
            filename_format, "\"");
    inputH.set_inputFileNameFormat(filename_format);
    *received = (*received).substr(beforeLastSeparator + 1);
    return 0;
}

/**
 * Waits for message from client containing "BASIC_SYNC" as the first part of
 * the message.
 *
 * @param pointer to string which should contain the message on successfull
 * operation
 *
 * @return 0 on success, -1 on error
 */
int networkHandler::WaitForSyncMessage(string *return_string) {
    // FIXME langech
    // It may be a good idea to include a length field to the sync string
    // otherwise we may not receive all data
    string receivedMessage;
    receivedMessage = readFromControlChannel(1, 10, "MAIN");
    int attemps = 0;
    while (receivedMessage.substr(0, 10).compare("BASIC_SYNC") != 0) {
        if (receivedMessage.compare("error_read_EOF") == 0) {
            log_ERROR("MAIN| Error: Control channel socket reached EOF.");
            return -1;
        }
        if (attemps > 20) {
            log_ERROR("MAIN| Error: No BASIC_SYNS message received from client after 60 seconds: ");
            return -1;
        }
        if (serverSocketFD_Control_TCP == -1) {
            log_ERROR("MAIN| Error: clientListenSocketFD_Control_TCP not initialized ?");
            return -1;
        }
        if (checkIfSocketIsReadyToRead(serverSocketFD_Control_TCP, 3, "MAIN") < 1)
            log_ERROR("MAIN| -> Waiting 3 seconds for client to reply with BASIC_SYNS.");
        receivedMessage = readFromControlChannel(1, 0, "MAIN");
    }
    log_DEBUG(
            "MAIN| -> Received synchronization message from client: ",
            receivedMessage);
    *return_string = receivedMessage;
    return 0;
}

/**
 * Method used to parse the incoming synchronization string on the server side.
 *
 * @param received_message, reference of the synchronization string
 *
 * @return 0 on success, -1 on error
 */
int networkHandler::ParseSyncMessage(const string &received_message) {
    if (ParseMethod(received_message))
        return -1;
    if (ParseMeasuredLink(received_message))
        return -1;
    string substring = received_message.substr(15,
            received_message.length());
    if (ParseExperimentDuration(&substring))
        return -1;
    if (ParseSafeguardStatus(&substring))
        return -1;
    if (ParseClientMeasureChannelPort(&substring))
        return -1;
    if (ParseClientSafeguardChannelPort(&substring))
        return -1;
    if (ParseServerMeasureChannelPort(&substring))
        return -1;
    if (ParseServerSafeguardChannelPort(&substring))
        return -1;
    if (ParseClientIntegerSize(&substring))
        return -1;
    if (ParseClientMeasureChannelAddress(&substring))
        return -1;
    if (ParseClientSafeguardChannelAddress(&substring))
        return -1;
    if (ParseMeasurementMagic(&substring))
        return -1;
    if (ParseSafeguardMagic(&substring))
        return -1;
    if (ParseFilenameFormat(&substring))
        return -1;
    return 0;
}

/**
 * Method to send a synchronization confirmation from the server to the client
 * including the uint32_t size of the server.
 */
int networkHandler::SendSyncConfirmation() {
    stringstream syncData;
    syncData << "BASIC_SYNS|" << sizeof (uint32_t) << "|";
    if (writeToControlChannel(1, syncData.str(), 3, "MAIN") == -1) {
        log_ERROR("MAIN| Error: Sending basic sync data failed.");
        return -1;
    }
    return 0;
}

/**
 * Synchronise global parameter of the Framework
 *
 * @param nodeRole
 * @param measurementMethod
 * @param measuredLink
 * @param experimentDuration
 * @param safeguardChannel
 * @param client_mPort
 * @param client_sPort
 * @param server_mPort
 * @param server_sPort
 * @param client_mAddress
 * @param client_sAddress
 * @param server_mAddress
 * @param server_sAddress
 * @param inputFilenameFormat
 *
 * @return 0 on success, --1 on error
 */
int networkHandler::syncBasicParameter(const uint32_t &nodeRole,
        const uint32_t &measurementMethod, const uint32_t &measuredLink,
        const uint32_t &experimentDuration, const uint32_t &safeguardChannel,
        const uint32_t &client_mPort, const uint32_t &client_sPort,
        const uint32_t &server_mPort, const uint32_t &server_sPort,
        const string &client_mAddress, const string &client_sAddress,
        const string &server_mAddress, const string &server_sAddress,
        const string &inputFilenameFormat) {
    log_DEBUG("MAIN| Entering syncBasicParameter()");
    string received_message;
    if (nodeRole == 0) { // client
        if (SendSyncMessage(measurementMethod, measuredLink, experimentDuration,
                safeguardChannel, client_mPort, client_sPort, server_mPort,
                server_sPort, client_mAddress, client_sAddress, server_mAddress,
                server_sAddress, inputFilenameFormat))
            return -1;
        if (WaitForSyncConfirmation(&received_message))
            return -1;
        if (ParseServerIntegerSize(received_message))
            return -1;
        return 0;
    } else if (nodeRole == 1) { // server
        if (WaitForSyncMessage(&received_message))
            return -1;
        if (ParseSyncMessage(received_message))
            return -1;
        if (SendSyncConfirmation())
            return -1;
    } else {
        log_ERROR("MAIN| Error: Unknown node role in syncBasicParameter()");
        return -1;
    }
    log_DEBUG("MAIN| Leaving syncBasicParameter()");
    return 0;

}

/**
 * enable or disable throttling of the sending rate [Byte/s]
 * @param maxRate [Byte/s]
 * @param enable
 */
void networkHandler::setMeasurementChannelSendingRateThrottle(int64_t maxRate, bool enable) {
    this->maxSendingRateMeasurementChannel = maxRate;
    this->throttleSendingRateMeasurementChannel = enable;
    if (enable) {
        log_NOTICE("TMSE| Throttling enabled - sending is capped at ", integer(maxRate), " Bytes/s");
    } else {
        log_NOTICE("TMSE| Throttling disabled - sending packets as fast as possible");
    }
}

/**
 * Threaded method to send measurement packets
 *
 * @return 0 on success, -1 on error
 */
int networkHandler::threadedMeasurmentSender() {
    log_DEBUG("TMSE| Starting. ProcessId: ", integer(syscall(SYS_gettid)), " CPU: ", integer(sched_getcpu()));
    log_DEBUG("TMSE| Entering threadedMeasurmentSender()");
    static timeHandler timing(inputH.get_timingMethod());
    static struct timespec time_to_sleep;

    uint32_t maxWriteAttemptsOnSocket = inputH.get_maxWriteAttemptsOnSocket();
    uint32_t consecutiveWriteFails = 0;

    //used for statistics on sleeping times caused by queuelength control on the device (if used at all))
    uint64_t sleepcount = 0LL;
    uint64_t sleeptimeNs = 0LL;
    uint64_t queuesizesum = 0LL;

    //used for throttling
    static timeHandler timingSendrate(inputH.get_timingMethod());
    int64_t nextSendrateSendNs = 0LL;
    int64_t throttlecount = 0LL;
    int64_t throttletimeNs = 0LL;
    int64_t packetcount = 0LL;
    int64_t bytesSend = 0LL;

    //error handling
    int errsv = 0;

    TBUSFixedPayload fixPay;
    TBUSDynamicPayload dynPay;
    uint32_t fixPaySize, dynPaySize, packetSize;
    uint32_t headersize = getHeaderSizeEthernetIpUdp();

    int *socket = NULL;
    if (!inputH.amIServer()) // Client
        socket = &clientSocketFD_Measure_UDP;
    else // Server
        socket = &serverListenSocketFD_Measure_UDP;
    uint32_t maxPacketSize = inputH.get_measurementMethodClass()->getMaxFixedPayloadSize();
    char sendData[maxPacketSize]; //create a buffer for the senddata
    memset(sendData, 0, maxPacketSize);

    // Signal for finishInit that this thread is ready to go
    flag_mSendingThreadReady = true;
    int64_t lastSentNs = 0;
    int64_t nextSendNs = 0;
    timingSendrate.probeTime(); //get the initial sendtime for sendrate throttling

    while (true) {
        if (flag_emergencySTOP) {
            pthread_mutex_unlock(&networkHandler_mSendingQueue);
            pthread_mutex_unlock(&networkHandler_mSentQueue); // unlock all queues so the threadedSenderDataDistributer can empty the queue
            log_DEBUG("TMSE| Leaving threadedMeasurmentSender(flag_emergencySTOP)");
            flag_sendingDone = true;
            flag_mSendingThreadDone = true;
            break;
        }
        pthread_mutex_lock(&networkHandler_mSendingQueue);
        if (mSendingQueue.empty()) {
            if (flag_finishedFillingSendingQueue) {
                pthread_mutex_unlock(&networkHandler_mSendingQueue); // release all mutexes (TSDD needs to empty the logging queues)
                pthread_mutex_unlock(&networkHandler_mSentQueue);
                flag_sendingDone = true;
                break;
            } else {
                // sleep till we get a signal that new data has arrived use timedwait to prevent endless sleep
                time_to_sleep = timeHandler::ns_to_timespec(timing.getCurrentTimeNs() + 1000000000LL); //prepare 1s wait
                log_DEBUG("TMSE| Waiting for data in sending queue until ", timeHandler::timespecToString(time_to_sleep), ".");
                int ret = pthread_cond_timedwait(&networkHandler_cond_mSendingQueue_has_data, &networkHandler_mSendingQueue, &time_to_sleep);
                log_DEBUG("TMSE| Finished waiting for data in sending queue. Ret= ", integer(ret));
                pthread_mutex_unlock(&networkHandler_mSendingQueue);
                continue;
            }
        } else {
            // fetch the fixed payload (containting the sendtime
            mSendingQueue.pop(&fixPay);
            if (mSendingQueue.empty()) { //inform the fillsendingQueue function that the queue is empty and thus the next chirp can be enqueued
                pthread_cond_signal(&networkHandler_cond_mSendingQueueIsEmpty);
            }
            pthread_mutex_unlock(&networkHandler_mSendingQueue);
            packetcount++;
            log_DEBUG("TMSE| SEND popped ", fixPay.toStringSend());
            fixPaySize = fixPay.getPayloadSize();
            nextSendNs = fixPay.getSendtimeNs();
            if (throttleSendingRateMeasurementChannel && nextSendrateSendNs > lastSentNs && nextSendNs <= lastSentNs) {
                throttlecount++;
                throttletimeNs += (nextSendrateSendNs - lastSentNs);
                nextSendNs = nextSendrateSendNs;
            }
            packetSize = fixPay.getPacketLength();
            if (fixPaySize > maxPacketSize) { //just a really basic test without much content
                log_ERROR("TMSE| Error: Packetsize is greater than the max Packet size: ", integer(fixPaySize), " > ", integer(maxPacketSize));
            }

            if (nextSendNs > lastSentNs) {
                timing.sleepUntil(nextSendNs);
            }
            //waitloop - waiting for free space in the sending queue to reduce dropped packets in our devices sending queue
            bool waited = false;
            while (mSendingMaxQueueSize > -1 && ((int) getMeasurementDeviceQueueSize()) >= mSendingMaxQueueSize) {
                timing.start();
                waited = true;
                timing.sleepFor(500000LL); //sleep 0.5 ms while the queue is fill
            }
            if (waited) {
                sleepcount++;
                sleeptimeNs += timing.stop();
                queuesizesum += getMeasurementDeviceQueueSize();
                log_DEBUG("TMSE| Slept for ", integer(timing.getstopped()), "ns for a slot in the DeviceQueue ", integer(getMeasurementDeviceQueueSize()));
            }

            //generate the packet at the last possible moment
            pthread_mutex_lock(&networkHandler_dynamicPayload);
            dynPay = dynamicPayload;
            pthread_mutex_unlock(&networkHandler_dynamicPayload);

            dynPaySize = dynPay.getPayloadSize();
            lastSentNs = timing.getCurrentTimeNs();
            dynPay.setSendtimeNs(lastSentNs);
            dynPay.insertPayloadInPacket(&sendData[fixPaySize]); //add dynamicPayload Data in sendbuffer

            int paddingLength = packetSize - headersize - fixPaySize - dynPaySize; //packetsize includes ip + udp headers
            if (paddingLength < 0) {
                log_ERROR("TMSE| Error: Packetpaddinglength is < 0: ", integer(paddingLength), ". The Measurementmethod tries to send a packet, which is to small.");
            } else {
                memset(&sendData[fixPaySize + dynPaySize], 0xF, paddingLength);
            }
            fixPay.setSendtimeNs(lastSentNs); //add the real application level sendtime at the last possible moment
            fixPay.insertPayloadInPacket(&sendData[0]);

            int dataSizeWritten = 0;
            int dataSizeToWrite = packetSize - headersize;

            dataSizeWritten = write(*socket, sendData, dataSizeToWrite);
            errsv = errno; //store the error code of write
            if (dataSizeWritten >= 0) {
                if (dataSizeWritten != dataSizeToWrite) {
                    log_ERROR("TMSE| Error: Write() on Measure_UDP didn't send all Data! Written ", integer(dataSizeWritten), " vs. toWrite ", integer(dataSizeToWrite));
                    consecutiveWriteFails++;
                } else {
                    consecutiveWriteFails = 0;
                }
                bytesSend += dataSizeWritten;
            } else {
                if (!flag_emergencySTOP) {
                    log_ERROR("TMSE| Error: Write() on Measure_UDP failed with error=", integer(errsv), " ", strerror(errsv));
                    log_ERROR("TMSE| Packet dropped at sender:", fixPay.toStringSend());
                    consecutiveWriteFails++;

                    if (consecutiveWriteFails > maxWriteAttemptsOnSocket || errsv == EBADF || errsv == ENOTCONN || errsv == ENOTSOCK) {// bad filedescriptor
                        {
                            network.set_emergencySTOP("Too many consecutive write fails on UDP Measurement Channel.");
                        }
                    }
                }
                continue;
            }

            //calculate when the next packet should be send to not exceed sending rate with our measurement packets
            if (throttleSendingRateMeasurementChannel) {
                assert(maxSendingRateMeasurementChannel != 0LL);
                nextSendrateSendNs = timingSendrate.getLastProbedTimeNs() + (packetSize + headersize) * BILLION / maxSendingRateMeasurementChannel;
                timingSendrate.probeTime();
            }

            // Push logging packet to queue
            payloadStruct toLog;
            toLog.dynPay = dynPay;
            toLog.fixPay = fixPay;
            pthread_mutex_lock(&networkHandler_mSentQueue);
            mSentQueue.push(toLog);
            pthread_cond_signal(&networkHandler_cond_mSentQueue_1_ready_for_log);
            pthread_mutex_unlock(&networkHandler_mSentQueue);
        }
    }
    flag_sendingDone = true;
    flag_mSendingThreadDone = true;
    log_DEBUG("TMSE| Leaving threadedMeasurmentSender()");
    if (packetcount > 0) {
        log_NOTICE("TMSE| ", integer(packetcount), " packets sent with ", integer(bytesSend), " bytes.");
    }
    if (sleepcount > 0) {
        log_NOTICE("TMSE| Waited ", integer(sleepcount), " times for ", integer(sleeptimeNs / sleepcount),
                "ns in average. Queusizeaverage after sleeping ", real(1.0 * queuesizesum / sleepcount), " packets.");
    }
    if (throttlecount > 0) {
        log_NOTICE("TMSE| Throttling to ", integer(maxSendingRateMeasurementChannel), "[Byte/s consumed] ", integer(throttletimeNs / 1000000LL), "ms by ",
                integer(throttlecount), " waitings. Avrage throttletime/wait ", real(throttletimeNs / throttlecount / 1000000.0), "ms");
    }
    return 0;
}

/**
 * Threaded Method to receive measurement packets
 *
 * @return	0 on success, -1 on error
 */
int networkHandler::threadedMeasurmentReceiver() {
    log_DEBUG("TMRE| Starting. ProcessId: ", integer(syscall(SYS_gettid)), " CPU: ", integer(sched_getcpu()));
    log_DEBUG("TMRE| Entering threadedMeasurmentReceiver()");
    static timeHandler timing(inputH.get_timingMethod());
    TBUSFixedPayload fixPay;
    TBUSDynamicPayload dynPay;

    int ret = 0;
    int readChars;
    char buffer[MAX_MEASUREMENT_CHANNEL_MESSAGE_LENGTH];
    uint32_t fixPaySize = fixPay.getPayloadSize();
    uint32_t dynPaySize = dynPay.getPayloadSize();
    int minPayloadSize = fixPaySize + dynPaySize;
    int headersize = getHeaderSizeEthernetIpUdp();
    int64_t packetsReceived = 0LL;
    int64_t bytesReceived = 0LL;
    uint32_t maxReadAttemptsOnSocket = inputH.get_maxReadAttemptsOnSocket();
    uint32_t readErrorCounter_measureUDP = 0;

    int * socket = NULL;
    string socketname;

    //get socket_to_use and socketname from noderole
    if (inputH.get_nodeRole() == 1) { //server
        socket = &serverListenSocketFD_Measure_UDP;
        socketname = "serverListenSocketFD_Measure_UDP";
    } else if (inputH.get_nodeRole() == 0) { //client
        socket = &clientSocketFD_Measure_UDP;
        socketname = "clientSocketFD_Measure_UDP";
    } else {
        log_ERROR("TMRE| Only node Roles 0 or 1 are supported!");
        return -1;
    }

    // Set to start value
    flag_measureReceivingDone = false;
    if (DEBUG_RECEIVING_QUEUE_LOCKS) log_DEBUG("TMRE| -> Before\tLOCK\t\tnetworkHandler_mReceivingQueue_1");
    // Signal for finishInit that this thread is ready to go
    flag_mReceivingThreadReady = true; 
    while (!flag_measureReceivingDone && !flag_emergencySTOP) {
        readChars = 0;
        memset(&buffer, 0, MAX_MEASUREMENT_CHANNEL_MESSAGE_LENGTH);

        // Wait for data will wake every 1 second to check if the experiment is done
        while (!flag_measureReceivingDone && !flag_emergencySTOP && readErrorCounter_measureUDP <= maxReadAttemptsOnSocket) {
            // Check if socket is alive and if no emergency happened
            if (*socket == -1) {
                log_ERROR("TMRE| Error: ", socketname, " not initialized ?");
                flag_measureReceivingDone = true;
                ret = -3;
                break;
            }
            // Check if socket is ready to read 
            if (checkIfSocketIsReadyToRead(*socket, maxWaitSecondsRead, "TMRE") < 1) {
                readErrorCounter_measureUDP++;
            } else {
                readErrorCounter_measureUDP = 0;
                break;
            }
        }

        if (readErrorCounter_measureUDP > maxReadAttemptsOnSocket) {
            log_ERROR("TMRE| Error: Too many unsuccessful read attempts (", integer(readErrorCounter_measureUDP), ") on ", socketname, ", exiting.");
            flag_measureReceivingDone = true;
            network.set_emergencySTOP("Too many unsuccessful read attempts on "+socketname);
            ret = -2;
            //break;
        }

        //only continue if we are ready to receive something from the socket
        if (!flag_measureReceivingDone && !flag_emergencySTOP) {
            readChars = recv(*socket, buffer, MAX_MEASUREMENT_CHANNEL_MESSAGE_LENGTH - 1, 0);
            timing.probeTime(); //note receive time
            if (readChars < 0) {
                readErrorCounter_measureUDP++;
                log_ERROR("TMRE| Error: Reading ", socketname, " failed (<0).Count=", integer(readErrorCounter_measureUDP));
                continue;
            } else if (readChars == 0) {
                log_ERROR("TMRE| Error: Reading ", socketname, " failed (=0).");
                continue;
            } else if (readChars < minPayloadSize) {
                log_ERROR("TMRE| Error: Received measurement packet is to short! ", integer(readChars), " < ", integer(minPayloadSize));
                continue;
            }
            //some statistics
            packetsReceived++;
            bytesReceived += readChars;
            //extract payload
            struct payloadStruct received;
            received.fixPay = TBUSFixedPayload(&buffer[0], timing.getLastProbedTimeNs());
            received.dynPay = TBUSDynamicPayload(&buffer[fixPaySize], timing.getLastProbedTimeNs());

            //check if packetsize is the same as given in the fixed payload
            if ((int) received.fixPay.getPacketLength() != readChars + headersize) {
                log_ERROR("TMRE| Error: Received measurement packet has wrong size! ", integer(received.fixPay.getPacketLength()), " != ",
                        integer(readChars + headersize), " ", received.fixPay.toStringReceived());
                continue;
            } else {
                log_DEBUG("TMRE| Received packet with fixPay = ", received.fixPay.toStringReceived(),
                        " and dynPay = ", received.dynPay.toStringReceived(), " and length = ", integer(received.fixPay.getPacketLength()));
                // Lock and fill the receivingqueue 
                pthread_mutex_lock(&networkHandler_mReceivingQueue);
                //push the packet to the receiving queue
                if (mReceivingQueue.push(received) == -1) {
                    log_NOTICE("TMRE| Receiving queue is full! Dropping packet with fixPay = ", fixPay.toStringReceived(),
                            " and dynPay = ", dynPay.toStringReceived());
                }
                pthread_mutex_unlock(&networkHandler_mReceivingQueue);
                pthread_cond_signal(&networkHandler_cond_mReceivingQueue_has_data); // Send a signal, that there is new data in the receiving queue
            }
        }
    }
    if (flag_emergencySTOP) {
        log_DEBUG("TMRE| Leaving threadedMeasurmentReceiver(flag_emergencySTOP)");
        ret = -1;
    } else {
        log_DEBUG("TMRE| Leaving threadedMeasurmentReceiver()");
    }
    flag_measureReceivingDone = true;
    flag_mReceivingThreadDone = true;
    if (packetsReceived > 0) {
        log_NOTICE("TMRE| ", integer(packetsReceived), " packets received with ", integer(bytesReceived), " bytes.");
    }
    return ret;
}

int networkHandler::threadedReceiverDataDistributer() {
    log_DEBUG("TRDD| Starting. ProcessId: ", integer(syscall(SYS_gettid)), " CPU: ", integer(sched_getcpu()));
    log_DEBUG("TRDD| Entering threadedReceiverDataDistributer()");
    struct payloadStruct toProcess;
    //uint32_t measureMethod = input.get_measurementMethod();
    class timeHandler timing(inputH.get_timingMethod());
    // Signal for finishInit that this thread is ready to go
    flag_receiverDataDistributerReady = true;

    while (!inputH.get_measurementMethodClass()->syncingDone() && !flag_emergencySTOP) {
        timing.sleepFor(100000000LL); //sleep 100ms
    }
    log_NOTICE("TRDD| Finished Waiting for syncingDone");

    while (true) {
        if (flag_emergencySTOP) {
            pthread_mutex_unlock(&networkHandler_mReceivingQueue);
            log_DEBUG("TRDD| -> After\tUNLOCK\t\tnetworkHandler_mReceivingQueue_1");
            // Send the Signal that the Safeguard channel shuts down
            this->set_safeguardReceivingDone(true);
            flag_receiverDataDistributerDone = true;
            flag_safeguardReceivingDone = true;
            log_DEBUG("TRDD| Leaving threadedReceiverDataDistributer(flag_emergencySTOP)");
            inputH.get_measurementMethodClass()->setLoggingDone(true, "TRDD");
            break;
        }
        if (flag_mReceivingThreadDone) {
            // If both queues are empty exit otherwise empty queues
            log_DEBUG("TRDD| -> Before\tLOCK\t\tnetworkHandler_mReceivingQueue_1");
            pthread_mutex_lock(&networkHandler_mReceivingQueue);
            if (mReceivingQueue.empty()) {
                inputH.get_measurementMethodClass()->setLoggingDone(true, "TRDD");
                pthread_mutex_unlock(&networkHandler_mReceivingQueue);
                log_DEBUG("TRDD| -> After\tUNLOCK\t\tnetworkHandler_mReceivingQueue_1");
                break;
            }
            pthread_mutex_unlock(&networkHandler_mReceivingQueue);
            log_DEBUG("TRDD| -> After\tUNLOCK\t\tnetworkHandler_mReceivingQueue_1");
        }
        pthread_mutex_lock(&networkHandler_mReceivingQueue);
        if (mReceivingQueue.empty()) {
            pthread_mutex_unlock(&networkHandler_mReceivingQueue);
            struct timespec time_to_sleep;
            time_to_sleep = timeHandler::ns_to_timespec(timing.getCurrentTimeNs() + 10000000LL); //sleep for 10 ms
            pthread_mutex_lock(&networkHandler_mDDRLock);
            pthread_cond_timedwait(&networkHandler_cond_mReceivingQueue_has_data, &networkHandler_mDDRLock, &time_to_sleep);
            pthread_mutex_unlock(&networkHandler_mDDRLock);
            continue;
        } else { // there is data in queue_1
            mReceivingQueue.pop(&toProcess);
        }
        pthread_mutex_unlock(&networkHandler_mReceivingQueue);

        inputH.get_measurementMethodClass()->fastDataCalculator(toProcess);
        inputH.get_measurementMethodClass()->dataFileWriterMeasureReceiver(toProcess);
    }
    // Send the Signal that the Safeguard channel shuts down
    this->set_safeguardReceivingDone(true);
    flag_receiverDataDistributerDone = true;
    flag_safeguardReceivingDone = true;
    //FIXME Unflexible solution
    inputH.get_measurementMethodClass()->setLoggingDone(true, "TRDD");

    log_DEBUG("TRDD| Leaving threadedReceiverDataDistributer()");
    log_DEBUG("TRDD| ");
    return 0;
}

/**
 * This method checks if the Framework ready to load the measurement method
 *
 * @param nodeRole

 * @return	0 on success, -1 on error
 */
int networkHandler::finishInit(uint32_t nodeRole) {
    log_DEBUG("MAIN| Entering finishInit()");
    int deathcounter = 0;
    class timeHandler timing;
    timing.setTimingMethod(inputH.get_timingMethod());
    while (!flag_mSendingThreadReady) {
        timing.probeTime();
        timing.sleepFor(1000000000LL); // sleep 1 second  Until(timing.getLastProbedTimeNs() + BILLION); //sleep 1 second
        deathcounter++;
        if (deathcounter > 10)
            break;
    }
    if (deathcounter > 10) {
        log_ERROR("MAIN| -> mSendingThread not ready after 10 seconds. abort");
        return -1;
    } else
        log_DEBUG("MAIN| -> mSendingThread ready.");
    while (!flag_senderDataDistributerReady) {
        timing.probeTime();
        timing.sleepFor(1000000000LL); //timing.sleepUntil(timing.getLastProbedTimeNs() + BILLION); //sleep 1 second
        deathcounter++;
        if (deathcounter > 10)
            break;
    }
    if (deathcounter > 10) {
        log_ERROR("MAIN| -> senderDataDistributer not ready after 10 seconds. abort");
        return -1;
    } else
        log_DEBUG("MAIN| -> senderDataDistributer ready.");
    while (!flag_mReceivingThreadReady) {
        timing.probeTime();
        timing.sleepFor(1000000000LL); //timing.sleepUntil(timing.getLastProbedTimeNs() + BILLION); //sleep 1 second
        deathcounter++;
        if (deathcounter > 10)
            break;
    }
    if (deathcounter > 10) {
        log_ERROR("MAIN| -> mReceivingThread not ready after 10 seconds. abort");
        return -1;
    } else
        log_DEBUG("MAIN| -> mReceivingThread ready.");
    while (!flag_receiverDataDistributerReady) {
        timing.probeTime();
        timing.sleepFor(1000000000LL); //timing.sleepUntil(timing.getLastProbedTimeNs() + BILLION); //sleep 1 second
        deathcounter++;
        if (deathcounter > 10)
            break;
    }
    if (deathcounter > 10) {
        log_ERROR("MAIN| -> receiverDataDistributer not ready after 10 seconds. abort");
        return -1;
    } else
        log_DEBUG("MAIN| -> receiverDataDistributer ready.");
    // Only wait for these thread when safeguard is activated
    if (inputH.get_safeguardChannel() == 1) {
        while (!flag_sSendingThreadReady) {
            timing.probeTime();
            timing.sleepFor(1000000000LL); //timing.sleepUntil(timing.getLastProbedTimeNs() + BILLION); //sleep 1 second
            deathcounter++;
            if (deathcounter > 10)
                break;
        }
        if (deathcounter > 10) {
            log_ERROR("MAIN| -> sSendingThread not ready after 10 seconds. abort");
            return -1;
        } else
            log_DEBUG("MAIN| -> sSendingThread ready.");
        while (!flag_sReceivingThreadReady) {
            timing.probeTime();
            timing.sleepFor(1000000000LL); //timing.sleepUntil(timing.getLastProbedTimeNs() + BILLION); //sleep 1 second
            deathcounter++;
            if (deathcounter > 10)
                break;
        }
        if (deathcounter > 10) {
            log_ERROR("MAIN| -> sReceivingThread not ready after 10 seconds. abort");
            return -1;
        } else
            log_DEBUG("MAIN| -> sReceivingThread ready.");
    }
    log_DEBUG("MAIN| Leaving finishInit()");
    return 0;
}

/**
 * Threaded method to send safeguard packets
 * sends dynPay Packets via safeguard channel every 50ms or if signaled of an important change
 * which requests an immediate send
 * it fetches the packet from the measurement method proir to sending
 *
 * @return	0 on success, -1 on error
 */
int networkHandler::threadedSafeguardSender() {
    log_DEBUG("TSSE| Starting. ProcessId: ", integer(syscall(SYS_gettid)), " CPU: ", integer(sched_getcpu()));
    log_DEBUG("TSSE| Entering threadedSafeguardSender()");
    static timeHandler timing(inputH.get_timingMethod());
    //static struct timespec time_to_sleep;
    uint32_t maxWriteAttemptsOnSocket = inputH.get_maxWriteAttemptsOnSocket();
    lastSafeguardChannelSendNs_ = timing.getCurrentTimeNs();
    timespec nextSend;

    TBUSDynamicPayload dynPay, lastDynPay;
    measurementMethodClass* mes = inputH.get_measurementMethodClass();
    int ret_pcw;

    int *socket = NULL;
    string socketName;
    if (inputH.get_nodeRole() == 0) { // Client 
        socket = &clientSocketFD_Safe_UDP;
        socketName = "clientSocketFD_Safe_UDP";
    } else { // Server
        socket = &serverListenSocketFD_Safe_UDP;
        socketName = "serverListenSocketFD_Safe_UDP";
    }
    // Signal for finishInit that this thread is ready to go
    flag_sSendingThreadReady = true;
    while (true) {
        nextSend = timeHandler::ns_to_timespec(lastSafeguardChannelSendNs_ + SAFEGUARDCHANNEL_SENDING_INTERVAL_NORMAL);
        //sleep until nextSafeguardSend or the SafeguardEmergencySend condition is hit
        //log_NOTICE("TSSE| Before pthread_cond_timedwait");
        pthread_mutex_lock(&networkHandler_sSafeguardSender);
        ret_pcw = pthread_cond_timedwait(&networkHandler_cond_sSafeguardEmergencySend, &networkHandler_sSafeguardSender, &nextSend);
        pthread_mutex_unlock(&networkHandler_sSafeguardSender);
        //log_NOTICE("TSSE| After pthread_cond_timedwait ",integer(ret_pcw));

        if (writeErrorCounter_safeguardUDP > maxWriteAttemptsOnSocket) {
            log_ERROR("TSSE| Error: To many unsuccessful write attempts (", integer(writeErrorCounter_safeguardUDP), ") on serverListenSocketFD_Safe_UDP, exiting.");
            flag_mSendingThreadDone = true;
            return -1;
        }
        if (flag_sendingDone || flag_safeguardReceivingDone || flag_measureReceivingDone) {
            break;
        }
        if (*socket == -1) {
            log_ERROR("TSSE| Error: ", socketName, " not initialized ?");
            flag_sSendingThreadDone = true;
            return -1;
        }
        if (checkIfSocketIsReadyToWrite(*socket, maxWaitSecondsWrite, "TSSE") < 1) {
            log_ERROR("TSSE| Error: ", socketName, " not ready to write.");
            continue;
        }
        if (flag_emergencySTOP) {
            log_DEBUG("TSSE| Leaving threadedSafeguardSender(flag_emergencySTOP)");
            flag_sSendingThreadDone = true;
            break;
        }
        //fetch the current Dynpay
        pthread_mutex_lock(&networkHandler_safeguardDynamicPayload);
        dynPay = safeguardDynamicPayload;
        pthread_mutex_unlock(&networkHandler_safeguardDynamicPayload);

        timing.probeTime();
        //check if dynPay is newer than the last dynamicPayload or if an emergency send should be done
        if (ret_pcw != ETIMEDOUT || dynPay.isNewerThan(lastDynPay)) {
            static int64_t lastConsoleLog = 0L;
            char sendData[dynPay.payloadsize_];
            dynPay.setSendtimeNs(timing.getLastProbedTimeNs());
            dynPay.insertPayloadInPacket(&sendData[0]);
            lastDynPay = dynPay;
            lastSafeguardChannelSendNs_ = timing.getLastProbedTimeNs();
            if (ret_pcw != ETIMEDOUT) {
                log_NOTICE("TSSE| Safeguard packet send     ", dynPay.toLogSmall(), "  (emergency)");
                lastConsoleLog = timing.getLastProbedTimeNs();
            } else if (timing.getLastProbedTimeNs() - lastConsoleLog >= CONSOLE_LOG_THROTTLE) {
                log_NOTICE("TSSE| Safeguard packet send     ", dynPay.toLogSmall());
                lastConsoleLog = timing.getLastProbedTimeNs();
            }
            if (write(*socket, sendData, dynPay.payloadsize_) < 0) {
                log_ERROR("TSSE| Error: Write() on ", socketName, " failed: ", dynPay.toStringSend());
                writeErrorCounter_safeguardUDP++;
                continue;
            } else {
                mes->dataFileWriterSafeguardSender(dynPay);
            }
        } else {
            log_DEBUG("TSSE| Dynpay not newer than previous and etimedout ", dynPay.toStringSend());
        }
    }
    flag_sSendingThreadDone = true;
    log_DEBUG("TSSE| Leaving threadedSafeguardSender()");
    return 0;
}

/**
 * Threaded method to receive safeguard packets
 *
 * @return	0 on success, -1 on error
 */
int networkHandler::threadedSafeguardReceiver() {
    log_DEBUG("TSRE| Starting. ProcessId: ", integer(syscall(SYS_gettid)), " CPU: ", integer(sched_getcpu()));
    log_DEBUG("TSRE| Entering threadedSafeguardReceiver()");

    static timeHandler timing(inputH.get_timingMethod());
    uint32_t maxWriteAttemptsOnSocket = inputH.get_maxWriteAttemptsOnSocket();

    int *socket = NULL;
    string socketName;
    if (inputH.get_nodeRole() == 0) { // Client 
        socket = &clientSocketFD_Safe_UDP;
        socketName = "clientSocketFD_Safe_UDP";
    } else { // Server
        socket = &serverListenSocketFD_Safe_UDP;
        socketName = "serverListenSocketFD_Safe_UDP";
    }

    int readChars;
    char buffer[MAX_SAFEGUARD_CHANNEL_MESSAGE_LENGTH];
    // Signal for finishInit that this thread is ready to go
    flag_sReceivingThreadReady = true;
    while (true) {
        if (readErrorCounter_safeguardUDP > maxWriteAttemptsOnSocket) {
            log_ERROR("TSRE| Error: To many unsuccessful read attempts (", integer(readErrorCounter_safeguardUDP), ") on clientSocketFD_Safe_UDP, exiting.");
            pthread_mutex_unlock(&networkHandler_sReceivingQueue);
            log_DEBUG("TSRE| -> After\tUNLOCK\t\tnetworkHandler_sReceivingQueue");
            flag_mSendingThreadDone = true;
            return -1;
        }
        // Wait for data will wake every 1 second to check if the experiment is done
        while (true) {
            // Check if socket is ready and no emergency happened
            if (*socket == -1) {
                log_ERROR("TSRE| Error: ", socketName, " not initialized ?");
                flag_sReceivingThreadDone = true;
                return -1;
            }
            if (flag_emergencySTOP) {
                log_DEBUG("TSRE| Leaving threadedSafeguardReceiver(flag_emergencySTOP)");
                log_DEBUG("TSRE| ");
                flag_sReceivingThreadDone = true;
                break;
            }
            if (checkIfSocketIsReadyToRead(*socket, maxWaitSecondsRead, "TSRE") < 1) {
                if (flag_safeguardReceivingDone)
                    break;
            } else
                break;
        }
        // if we are done shutdown receiver thread
        if (flag_safeguardReceivingDone)
            break;
        memset(&buffer, 0, MAX_SAFEGUARD_CHANNEL_MESSAGE_LENGTH);
        readChars = read(*socket, buffer, MAX_SAFEGUARD_CHANNEL_MESSAGE_LENGTH - 1);
        if (readChars < 0) {
            log_ERROR("TSRE| Error: Reading ", socketName, " failed.");
            readErrorCounter_safeguardUDP++;
            continue;
        } else if (readChars != (int) TBUSDynamicPayload::payloadsize_) {//TODO do we have to check if the packet is smaller than smallest ethernet frame?
            log_ERROR("TSRE| Error: Safeguard Packet with wrong length receivd. Expected ", integer(TBUSDynamicPayload::payloadsize_), " received ", integer(readChars));
            continue;
        }

        TBUSDynamicPayload dynPay{&buffer[0], timing.getCurrentTimeNs()};
        inputH.get_measurementMethodClass()->processSafeguardPacket(dynPay);

        if (flag_emergencySTOP) {
            log_DEBUG("TSRE| Leaving threadedSafeguardReceiver(flag_emergencySTOP)");
            log_DEBUG("TSRE| ");
            flag_sReceivingThreadDone = true;
            return -1;
        }
    }
    flag_sReceivingThreadDone = true;
    log_DEBUG("TSRE| Leaving threadedSafeguardReceiver()");
    return 0;
}

/**
 * Check if a socket is ready to read, it will wait seconds before it return with an error
 *
 * @param socketToCheck
 * @param seconds
 * @param caller		Threadname in Logging
 *
 * @return 1 on success, -1 or 0 on error
 */
int networkHandler::checkIfSocketIsReadyToRead(int socketToCheck, uint32_t seconds, string caller) {
    if (DEBUG_CHECK_IF_SOCKET_IS_READY) log_DEBUG(caller, "| Entering checkIfSocketIsReadyToRead()");
    int statusOfSocket = 0;
    timeval wait;
    wait.tv_sec = seconds;
    wait.tv_usec = 0;
    fd_set read_sockets_set; // Will only contain socketToCheck
    FD_ZERO(&read_sockets_set); // Initialize Set
    FD_SET(socketToCheck, &read_sockets_set); // Register socketToCheck in set
    statusOfSocket = select(socketToCheck + 1, &read_sockets_set, (fd_set*) NULL, (fd_set*) NULL, &wait);
    // Return ">=0" => Number of Socket which are ready to read ||  Return "-1" => Error
    if (statusOfSocket == 1) {
        if (FD_ISSET(socketToCheck, &read_sockets_set)) { // Will return "1" if the socket is ready to read
            if (DEBUG_CHECK_IF_SOCKET_IS_READY) log_DEBUG(caller, "| Leaving checkIfSocketIsReadyToRead(true)");
            return 1;
        }
    }
    if (DEBUG_CHECK_IF_SOCKET_IS_READY) log_DEBUG(caller, "| Leaving checkIfSocketIsReadyToRead(false)");
    return statusOfSocket; //Failsafe
}


/**
 * Check if a socket is write to read, it will wait seconds before it return with an error
 *
 * @param socketToCheck
 * @param seconds
 * @param caller			Threadname in Logging
 *
 * @return 1 on success, -1 or 0 on error
 */
// FIXME langech
// This function returns "true", even if the requested socket is not ready to read
// cause the value of statusOfSocket can be 1

int networkHandler::checkIfSocketIsReadyToWrite(int socketToCheck, uint32_t seconds, string caller) {
    if (DEBUG_CHECK_IF_SOCKET_IS_READY) log_DEBUG(caller, "| Entering checkIfSocketIsReadyToWrite()");
    int statusOfSocket = 0;
    timeval wait;
    wait.tv_sec = seconds;
    wait.tv_usec = 0;
    fd_set write_sockets_set; // Will only contain socketToCheck
    FD_ZERO(&write_sockets_set); // Initialize Set
    FD_SET(socketToCheck, &write_sockets_set); // Register socketToCheck in set
    if (socketToCheck < 0) {
        if (DEBUG_CHECK_IF_SOCKET_IS_READY) log_ERROR("MAIN| ERROR: Socket not properly initialized.");
        return -1;
    }
    statusOfSocket = select(socketToCheck + 1, (fd_set*) NULL, &write_sockets_set, (fd_set*) NULL, &wait);
    // Return ">=0" => Number of Socket which are ready to read ||  Return "-1" => Error
    if (statusOfSocket == 1) {
        if (FD_ISSET(socketToCheck, &write_sockets_set)) { // Will return "1" if the socket is ready to write
            if (DEBUG_CHECK_IF_SOCKET_IS_READY) log_DEBUG(caller, "| Leaving checkIfSocketIsReadyToWrite(true)");
            return 1;
        }
    }
    if (DEBUG_CHECK_IF_SOCKET_IS_READY)log_DEBUG(caller, "| Leaving checkIfSocketIsReadyToWrite(false)");
    return statusOfSocket; //Failsafe
}

/**
 * Threaded method to log and transfer packets to calculation
 *
 * @return 0 on success, -1 on error
 */
int networkHandler::threadedSenderDataDistributer() {
    log_DEBUG("TSDD| Starting. ProcessId: ", integer(syscall(SYS_gettid)), " CPU: ", integer(sched_getcpu()));

    log_DEBUG("TSDD| Entering threadedSenderDataDistributer()");
    static class timeHandler timing(inputH.get_timingMethod());
    struct payloadStruct toProcess;
    static measurementMethodClass* measurementMethod = inputH.get_measurementMethodClass();
    //uint32_t measureMethod = input.get_measurementMethod();
    flag_senderDataDistributerReady = true;
    while (true) {
        if (flag_emergencySTOP) { // release all mutexes and return with error
            pthread_mutex_unlock(&networkHandler_mSentQueue);
            flag_senderDataDistributerDone = true;
            log_DEBUG("TSDD| Leaving threadedSenderDataDistributer(flag_emergencySTOP)");
            return -1;
        }
        if (flag_sendingDone) { //sending done and queues empy
            pthread_mutex_lock(&networkHandler_mSentQueue);
            if (mSentQueue.empty()) {
                pthread_mutex_unlock(&networkHandler_mSentQueue);
                break;
            }
            pthread_mutex_unlock(&networkHandler_mSentQueue);
        }
        pthread_mutex_lock(&networkHandler_mSentQueue);
        if (mSentQueue.empty()) {
            pthread_mutex_unlock(&networkHandler_mSentQueue);
            timing.probeTime();
            struct timespec time_to_sleep = timing.getLastProbedTime() + 500000000L; //sleep 500ms
            pthread_mutex_lock(&networkHandler_mDDSLock);
            pthread_cond_timedwait(&networkHandler_cond_mSendingQueue_has_data, &networkHandler_mDDSLock, &time_to_sleep);
            pthread_mutex_unlock(&networkHandler_mDDSLock);
            continue;
        } else {
            mSentQueue.pop(&toProcess);
            pthread_mutex_unlock(&networkHandler_mSentQueue);
            measurementMethod->dataFileWriterMeasureSender(toProcess);
        }
    }
    flag_senderDataDistributerDone = true;
    log_DEBUG("TSDD| Leaving threadedSenderDataDistributer()");
    return 0;
}

/**
 * Set the Flag finishedFillinfSendingQueue
 *
 * @param input
 * @return 0 on success
 *
 */
int networkHandler::set_finishedFillingSendingQueue(bool input) {
    log_DEBUG("MEME| Entering set_finishedFillingSendingQueue()");
    if (input < 0 || input > 1)
        return -1;
    flag_finishedFillingSendingQueue = input;
    log_DEBUG("MEME| Leaving set_finishedFillingSendingQueue()");
    return 0;
}

/**
 * Return status of the flag finishedFillingSendingQueue
 *
 * @return 0 false, 1 true
 */
bool networkHandler::get_finishedFillingSendingQueue() {
    log_DEBUG("MEME| Entering get_finishedFillingSendingQueue()");
    bool status;
    status = flag_finishedFillingSendingQueue;
    log_DEBUG("MEME| Leaving get_finishedFillingSendingQueue()");
    return status;
}

/**
 * @FIXME Krauthoff: needed for calculation result only
 * Get value of the flag sendingDone
 *
 * @return 0 false, 1 true
 */
bool networkHandler::set_sendingDone(bool flag) {
    log_DEBUG("MEME| Entering set_sendingDone()");
    flag_sendingDone = flag;
    log_DEBUG("MEME| Leaving set_sendingDone()");
    return flag;
}

/**
 * Get value of the flag sendingDone
 *
 * @return 0 false, 1 true
 */
bool networkHandler::get_sendingDone() {
    log_DEBUG("MEME| Entering get_sendingDone()");
    bool status;
    status = flag_sendingDone;
    log_DEBUG("MEME| Leaving get_sendingDone()");
    return status;
}

/**
 * This Function set the flag flag_safeguardReceivingDone to the input
 *
 * @param input
 *
 * @return 0 on success
 */
int networkHandler::set_safeguardReceivingDone(bool input) {
    log_DEBUG("MEME| Entering set_safeguardReceivingDone(), setting to ", integer(input));
    flag_safeguardReceivingDone = input;
    return 0;
}

/**
 * This Function set the flag flag_measureReceivingDone to the input
 *
 * @param input
 *
 * @return 0 on success
 */
int networkHandler::set_measureReceivingDone(bool input) {
    log_DEBUG("MEME| Entering set_measureReceivingDone()");
    if (input < 0 || input > 1)
        return -1;
    flag_measureReceivingDone = input;
    log_DEBUG("MEME| Leaving set_measureReceivingDone()");
    return 0;
}

/**
 * This Function gets the flag flag_measureReceivingDone to the input
 *
 * @return 0 false, 1 true
 */
bool networkHandler::get_measureReceivingDone() {
    log_DEBUG("MEME| Entering get_measureReceivingDone()");
    bool status;
    status = flag_measureReceivingDone;
    log_DEBUG("MEME| Leaving get_measureReceivingDone()");
    return status;
}

/**
 * This Function gets the flag flag_safeguardReceivingDone to the input
 *
 * @return 0 false, 1 true
 */
bool networkHandler::get_safeguardReceivingDone() {
    log_DEBUG("MEME| Entering get_safeguardReceivingDone()");
    bool temp;
    temp = flag_safeguardReceivingDone;
    log_DEBUG("MEME| Leaving get_safeguardReceivingDone()");
    log_DEBUG("MEME| ");
    return temp;
}

/**
 * Sets the dynamic payload for the backchannel and the safeguardchannel
 * if safeguardEmergencySend is set it will also trigger an emediate send via safeguardchannel
 * @param dynPay
 * @param safeguardEmergencySend
 * @return 
 */
int networkHandler::setDynamicPayload(TBUSDynamicPayload & dynPay, bool safeguardEmergencySend) {
    if (DEBUG_SET_DYNAMIC_PAYLOAD) log_DEBUG("MEME| Entering dynamic_payload_setData()");
    if (dynPay.getPayloadSize() < MAX_MEASUREMENT_CHANNEL_MESSAGE_LENGTH) {
        if (DEBUG_SET_DYNAMIC_PAYLOAD) log_DEBUG("MEME| -> Before\tLOCK\t\tnetworkHandler_dynamicPayload with size ", integer(dynPay.getPayloadSize()));
        pthread_mutex_lock(&networkHandler_dynamicPayload);
        dynamicPayload = dynPay;
        pthread_mutex_unlock(&networkHandler_dynamicPayload);
        pthread_mutex_lock(&networkHandler_safeguardDynamicPayload);
        safeguardDynamicPayload = dynPay;
        pthread_mutex_unlock(&networkHandler_safeguardDynamicPayload);
        if (DEBUG_SET_DYNAMIC_PAYLOAD) log_DEBUG("MEME| Dynamic Payload set ", dynPay.toStringSend());
        if (safeguardEmergencySend) {
            //trigger the EmergencySend Condition
            pthread_cond_signal(&networkHandler_cond_sSafeguardEmergencySend);
        }
        if (DEBUG_SET_DYNAMIC_PAYLOAD) log_DEBUG("MEME| -> After\tUNLOCK\t\tnetworkHandler_dynamicPayload with payloadsize of ", integer(dynamicPayload.getPayloadSize()));
    } else {
        log_ERROR("Error: setDynamicPayload failed -- to large.");
        return -1;
    }
    if (DEBUG_SET_DYNAMIC_PAYLOAD) log_DEBUG("MEME| Leaving dynamic_payload_setData()");
    return 0;
}


/**
 *	Deletes all elements in the safeguard receiving Queue in a threadsafe way
 *
 * @return 0 on success. -1 on empty queue
 */
int networkHandler::sReceivingQueue_clean() {
    log_DEBUG("MEME| Entering sReceivingQueue_clean()");
    log_DEBUG("MEME| -> Before\tLOCK\t\tnetworkHandler_sReceivingQueue");
    pthread_mutex_lock(&networkHandler_sReceivingQueue);
    while (!sReceivingQueue.empty())
        sReceivingQueue.pop();
    pthread_mutex_unlock(&networkHandler_sReceivingQueue);
    log_DEBUG("MEME| -> After\tUNLOCK\t\tnetworkHandler_sReceivingQueue");
    log_DEBUG("MEME| Leaving sReceivingQueue_clean()");
    return 0;
}

/**
 *	Return the count of elements of the safeguard receiving Queue in a threadsafe way. Max return count is bound by input.maxQueueSize.
 *
 * @return count of elements in queue
 */
int networkHandler::sReceivingQueue_size() {
    int count;
    log_DEBUG("MEME| -> Before\tLOCK\t\tnetworkHandler_sReceivingQueue");
    pthread_mutex_lock(&networkHandler_sReceivingQueue);
    count = sReceivingQueue.size();
    pthread_mutex_unlock(&networkHandler_sReceivingQueue);
    log_DEBUG("MEME| -> After\tUNLOCK\t\tnetworkHandler_sReceivingQueue");
    return count;
}

/**
 *	Check if the safeguard receiving Queue is empty in a threadsafe way
 *
 * @return 0 if queue has elements. 1 if queue is empty
 */
bool networkHandler::sReceivingQueue_isEmpty() {
    bool status;
    log_DEBUG("MEME| -> Before\tLOCK\t\tnetworkHandler_sReceivingQueue");
    pthread_mutex_lock(&networkHandler_sReceivingQueue);
    status = sReceivingQueue.empty();
    pthread_mutex_unlock(&networkHandler_sReceivingQueue);
    log_DEBUG("MEME| -> After\tUNLOCK\t\tnetworkHandler_sReceivingQueue");
    return status;
}


/**
 *	Checks if the sending Queue is empty and returns afterwards.
 *  If the queue is not empty the thread sleeps until it is signaled to wake up again (on an empty queue)
 *
 * @return 0 if no wait was necessary
 *         1 if the queue was not empty and the thread waited some time
 */
int networkHandler::waitForEmptySendingQueue() {

    static class timeHandler timing(inputH.get_timingMethod());
    pthread_mutex_lock(&networkHandler_mSendingQueue);
    if (mSendingQueue.empty()) {
        pthread_mutex_unlock(&networkHandler_mSendingQueue);
        return 0;
    } else {
        log_NOTICE("MEME| fillsending is waiting for an empty sendingqueue to enqueue the next chirp");
        timing.start();
        //pthread_cond_timedwait(&networkHandler_cond_mSendingQueue_has_data, &networkHandler_mDDSLock, &time_to_sleep);
        pthread_cond_wait(&networkHandler_cond_mSendingQueueIsEmpty, &networkHandler_mSendingQueue);
        pthread_mutex_unlock(&networkHandler_mSendingQueue);
        log_NOTICE("MEME| fillsending waited ", integer(timing.stop() / 1000000LL), " ms for an empty sendingqueue to enqueue the next chirp");
        return 1;
    }
}

/**
 *	Pushes a new "packet" into the measurement sending Queue in a threadsafe way.
 *	If the queue is full (bound by input.maxQueueSize) wait for signal "mSentQueue_has_room = true" as long as the sending queue is not empty.
 *	If the sending queue is empty enqueue packet asap. This can cause the total loss of the packet data, since it can only be logged on the receiver side.
 *
 * @param fixedPay - the fixed Payload of this measurement packet (the dynamic payload will be included 
 *                   just before the packet is sent
 * @return  the current size of the sending queue after enqueing finisheds
 */
int networkHandler::mSendingQueue_enqueue(TBUSFixedPayload & fixedPay) {
    static class timeHandler timing(inputH.get_timingMethod());
    int queue_size = 0;
    while (true) {
        pthread_mutex_lock(&networkHandler_mSendingQueue);
        queue_size = mSendingQueue.size();
        if (mSendingQueue.isFull()) {
            pthread_mutex_unlock(&networkHandler_mSendingQueue);
            log_NOTICE("Sleep! max_queue_size=", integer(mSendingQueue.getMaxSize()), "current=", integer(queue_size));
            timing.sleepFor(10000000); //sleep for 10 ms
        } else {
            mSendingQueue.push(fixedPay);
            if (queue_size == 1) { //inform the threadedSender about an element in the queue
                pthread_cond_signal(&networkHandler_cond_mSendingQueue_has_data);
            }
            pthread_mutex_unlock(&networkHandler_mSendingQueue);
            break;
        }
    }
    return queue_size + 1;
}

/**
 * returns the sizeof the mSendingQueue to prevent sending if the queue is not empty yet
 * @return 
 */
int networkHandler::getCurrentmSendingQueueSize() {
    pthread_mutex_lock(&networkHandler_mSendingQueue);
    return mSendingQueue.size();
    pthread_mutex_unlock(&networkHandler_mSendingQueue);
}


/**
 * Check the status of all thread and return the number of the terminated threads
 *
 * @param caller	Thread name in logging
 *
 * @return	sum of terminated thread numbers
 */
int networkHandler::getThreadStatus(string caller) {
    //log_DEBUG(caller, "| Entering getThreadStatus()");
    /* Thread Number
     * --------------------------------------------------
     *  1 => mReceivingThread
     *  2 => mSendingThread
     *  4 => receiverDataDistributer
     *  8 => senderDataDistributer
     * 16 => sSendingThread
     * 32 => sReceivingThread
     * --------------------------------------------------
     */
    int returnValue = 0;
    if (flag_mReceivingThreadDone == true)
        returnValue += 1;
    if (flag_mSendingThreadDone == true)
        returnValue += 2;
    if (flag_receiverDataDistributerDone == true)
        returnValue += 4;
    if (flag_senderDataDistributerDone == true)
        returnValue += 8;
    if (flag_sSendingThreadDone == true)
        returnValue += 16;
    if (flag_sReceivingThreadDone == true)
        returnValue += 32;
    //log_DEBUG(caller, "| Leaving getThreadStatus()");
    //log_DEBUG(caller, "| ");
    return returnValue;
}

/**
 * Push the red button for selfdestruction ....
 *
 * @return 0
 */
int networkHandler::set_emergencySTOP(string reason) {
    if (!flag_emergencySTOP) {
        flag_emergencySTOP = true;
        log_NOTICE("flag_emergencySTOP set. Reason: ", reason);
        if (inputH.amIServer()) {
            if (serverListenSocketFD_Measure_UDP > 0) {
                log_NOTICE("Will close any open sockets in 3 seconds");
                usleep(3000000);
            } else {
                //close the server faster if it is not connected in a current measurement
            }
        }
        log_NOTICE("Will close sockets now");
        closeSockets();
    }
    return 0;
}

/**
 * Dependent on the role nodeRole (client/server) the control channel we be created (server) or a connection to it will be established (client)
 * @param role				0 client, 1 server
 * @param clientIP			IPv4 or IPv6 client address as string
 * @param serverIP			IPv4 or IPv6 server address as string
 * @param clientPort		Control channel port
 * @param serverPort		Control channel port
 * @param clientInterface
 * @param serverInterface
 * @param ipVersion			0 IPv4, 1 IPv6
 * @return 0 on success, -1 on error
 */
int networkHandler::establishControlChannel(uint32_t role, string clientIP, string serverIP, uint32_t clientPort, uint32_t serverPort, string clientInterface, string serverInterface, uint32_t ipVersion) {
    log_DEBUG("MAIN| Entering establishControlChannel()");
    int socket_retval = 0;
    int channel_retval = 0;
    switch (role) {
        case 0: // Client
            if (clientInterface.length() != 0)
                log_NOTICE(
                    "MAIN| Establishing connection to control channel with Client[",
                    clientIP, ":", integer(clientPort), "] on \"",
                    clientInterface, "\" to Server[", serverIP, ":",
                    integer(serverPort), "]"
                    );
            else
                log_NOTICE(
                    "MAIN| Establishing connection to control channel with Client[",
                    clientIP, ":", integer(clientPort), "] to Server[",
                    serverIP, ":", integer(serverPort), "]"
                    );
            socket_retval = createSocket(clientIP, clientPort, &clientSocketFD_Control_TCP, ipVersion, 1, clientInterface);
            channel_retval = connectToTcpChannel(serverIP, serverPort, &clientSocketFD_Control_TCP, ipVersion);
            break;
        case 1: // Server
            if (serverInterface.length() != 0) {
                log_NOTICE(
                        "MAIN| Control channel will be created with Server[",
                        serverIP, ":", integer(serverPort), "] on \"",
                        serverInterface, "\" probably to Client[", clientIP, ":",
                        integer(clientPort), "]"
                        );
            } else {
                log_NOTICE(
                        "MAIN| Control channel will be created with Server[",
                        serverIP, ":", integer(serverPort), "] probably to Client[",
                        clientIP, ":", integer(clientPort), "]"
                        );
            }
            socket_retval = createSocket(serverIP, serverPort, &serverListenSocketFD_Control_TCP, ipVersion, 1, serverInterface);
            channel_retval = createTcpChannel(&serverListenSocketFD_Control_TCP, &serverSocketFD_Control_TCP);
            break;
        default: // wrong value
            log_ERROR("MAIN| ERROR: Invalid role value.");
            return -1;
    }
    if (socket_retval || channel_retval) {
        log_ERROR("MAIN| ERROR: Could not establish control channel.");
        return -1;
    } else {
        log_DEBUG("MAIN| Leaving establishControlChannel()");
        log_DEBUG("MAIN| ");
        return 0;
    }
}

/**
 * Create depending on noderole the connection to the measurement channel
 *
 * @param role				0 client, 1 server
 * @param clientIP			IPv4 or IPv6 client address as string
 * @param serverIP			IPv4 or IPv6 server address as string
 * @param clientPort		Control channel port
 * @param serverPort		Control channel port
 * @param clientInterface
 * @param serverInterface
 * @param ipVersion			0 IPv4, 1 IPv6
 * @return 0 on success, -1 on error
 */
int networkHandler::establishMeasurementChannel(uint32_t role, string clientIP, string serverIP, uint32_t clientPort, uint32_t serverPort, string clientInterface, string serverInterface, uint32_t ipVersion) {
    log_DEBUG("MAIN| Entering establishMeasurementChannel()");
    int socket_retval = 0;
    int channel_retval = 0;
    switch (role) {
        case 0: // client
            if (clientInterface.length() != 0) {
                log_NOTICE(
                        "MAIN| Establishing connection to measurement channel with Client[",
                        clientIP, ":", integer(clientPort), "] on \"",
                        clientInterface, "\" to Server[", serverIP, ":",
                        integer(serverPort), "]"
                        );
            } else {
                log_NOTICE(
                        "MAIN| Establishing connection to measurement channel with Client[",
                        clientIP, ":", integer(clientPort), "] to Server[",
                        serverIP, ":", integer(serverPort), "]"
                        );
            }
            socket_retval = createSocket(clientIP, clientPort, &clientSocketFD_Measure_UDP, ipVersion, 0, clientInterface);
            channel_retval = connectToUdpChannel(serverIP, serverPort, &clientSocketFD_Measure_UDP, &clientSocketFD_Control_TCP, ipVersion);
            break;
        case 1: // server
            if (serverInterface.length() != 0) {
                log_NOTICE(
                        "MAIN| Measurement channel will be created with Server[",
                        serverIP, ":", integer(serverPort), "] on \"",
                        serverInterface, "\" to Client[", clientIP, ":",
                        integer(clientPort), "]"
                        );
            } else {
                log_NOTICE(
                        "MAIN| Measurement channel will be created with Server[",
                        serverIP, ":", integer(serverPort), "] to Client[",
                        clientIP, ":", integer(clientPort), "]"
                        );
            }
            socket_retval = createSocket(serverIP, serverPort, &serverListenSocketFD_Measure_UDP, ipVersion, 0, serverInterface);
            channel_retval = createUdpChannel(&serverListenSocketFD_Measure_UDP, &serverSocketFD_Control_TCP, MeasurementChannelForeignIP);
            break;
        default: // wrong value
            log_ERROR("MAIN| ERROR: Invalid role value.");
            return -1;
    }
    if (socket_retval || channel_retval) {
        log_ERROR("MAIN| ERROR: Could not establish measurement channel.");
        return -1;
    } else {
        log_DEBUG("MAIN| Leaving establishMeasurementChannel()");
        return 0;
    }
}

/**
 * Connect depending on the noderole to the safeguard channel
 *
 * @param role				0 client, 1 server
 * @param clientIP			IPv4 or IPv6 client address as string
 * @param serverIP			IPv4 or IPv6 server address as string
 * @param clientPort		Control channel port
 * @param serverPort		Control channel port
 * @param clientInterface
 * @param serverInterface
 * @param ipVersion			0 IPv4, 1 IPv6
 * @return 0 on success, -1 on error
 */
int networkHandler::establishSafeguardChannel(uint32_t role, string clientIP, string serverIP, uint32_t clientPort, uint32_t serverPort, string clientInterface, string clientGateway, string serverInterface, uint32_t ipVersion) {
    log_DEBUG("MAIN| Entering establishSafeguardChannel()");
    int socket_retval = 0;
    int channel_retval = 0;
    switch (role) {
        case 0: // client
            if (clientInterface.length() != 0) {
                log_NOTICE(
                        "MAIN| Establishing connection to Safeguard channel with Client[",
                        clientIP, ":", integer(clientPort), "] on \"",
                        clientInterface, "\" to Server[", serverIP, ":",
                        integer(serverPort), "]"
                        );
            } else {
                log_NOTICE(
                        "MAIN| Establishing connection to Safeguard channel with Client[",
                        clientIP, ":", integer(clientPort), "] to Server[",
                        serverIP, ":", integer(serverPort), "]"
                        );
            }

            if (clientInterface.length() > 0 && clientGateway.length() > 0) {
                log_NOTICE(
                        "MAIN| Creating Routingpath for Safeguard channel for device ",
                        "[", clientInterface, "]",
                        " with default gateway ",
                        "[", clientGateway, "]"
                        );
                routingSafeguard = new routingHandler::routingHandler(clientInterface, clientGateway, serverIP, serverPort, "udp");
            }

            socket_retval = createSocket(clientIP, clientPort, &clientSocketFD_Safe_UDP, ipVersion, 0, clientInterface);
            channel_retval = connectToUdpChannel(serverIP, serverPort, &clientSocketFD_Safe_UDP, &clientSocketFD_Control_TCP, ipVersion);
            break;
        case 1: // server
            if (serverInterface.length() != 0) {
                log_NOTICE(
                        "MAIN| Safeguard channel will be created with Server[",
                        serverIP, ":", integer(serverPort), "] on \"",
                        serverInterface, "\" to Client[", clientIP, ":",
                        integer(clientPort), "]"
                        );
            } else {
                log_NOTICE(
                        "MAIN| Safeguard channel will be created with Server[",
                        serverIP, ":", integer(serverPort), "] to Client[",
                        clientIP, ":", integer(clientPort), "]"
                        );
            }
            socket_retval = createSocket(serverIP, serverPort, &serverListenSocketFD_Safe_UDP, ipVersion, 0, serverInterface);
            channel_retval = createUdpChannel(&serverListenSocketFD_Safe_UDP, &serverSocketFD_Control_TCP, SafeguadChannelForeignIP);
            break;
        default: // wrong value
            log_ERROR("MAIN| ERROR: Invalid role value.");
            return -1;
    }
    if (socket_retval || channel_retval) {
        log_ERROR("MAIN| ERROR: Could not establish safeguard channel.");
        return -1;
    } else {
        log_DEBUG("MAIN| Leaving establishSafeguardChannel()");
        log_DEBUG("MAIN| ");
        return 0;
    }
}

/**
 * Create socket and bind it to an address:port
 *
 * @param ip
 * @param port
 * @param *socket_fd
 * @param ip_version
 * @param type (0=UDP, 1=TCP)
 *
 * @return	0 on success, -1 on error
 */
int networkHandler::createSocket(string ip, uint32_t port, int *socket_fd, uint32_t ip_version, uint32_t type, string device) {
    log_DEBUG("MAIN| Entering createSocket()");
    // Error variables
    int option = 1;
    // Prepare address structs
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof (hints));
    // FIXME langech
    // workaround, cause we need the portnumber as a string
    // would be better to store it directly this way in the object
    stringstream ssOut;
    string sOut;
    string typeString = "";
    ssOut.str("");
    ssOut << port;
    sOut = ssOut.str();
    if (ip_version == 0) { //IPv4
        hints.ai_family = AF_INET;
    } else {
        hints.ai_family = AF_INET6;
    }
    if (type == 0) { // UDP
        hints.ai_socktype = SOCK_DGRAM;
        typeString = "UDP";
    } else {
        hints.ai_socktype = SOCK_STREAM;
        typeString = "TCP";
    }

    if (getaddrinfo(ip.c_str(), sOut.c_str(), &hints, &res) != 0) {
        log_ERROR("MAIN| ERROR: Could not create sockaddr structure for ", typeString, " socket.");
        freeaddrinfo(res);
        return -1;
    }
    // Create socket and bind
    if ((*socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
        log_ERROR("MAIN| ERROR: Call to socket() failed for ", typeString, " socket.");
    }
    if (type == 1 && type == 0) {
        if (setsockopt(*socket_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &option, sizeof (option)) == -1) {
            log_ERROR("MAIN| ERROR: setsockopt() failedfor ", typeString, " socket.");
            freeaddrinfo(res);
            return -1;
        }
    }

    //bindtodevice macht probleme beim routingumsetzen
    /*
    if (device.length() > 0){
        struct ifreq ifr;
        memset(&ifr, 0, sizeof ifr);
        snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", device.c_str());
        if(setsockopt(*socket_fd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr))== -1){
            log_ERROR("MAIN| ERROR: Bind on device failed.");
            freeaddrinfo(res);
            return -1;
        } 
    }
     */
    //}else{
    //if (bind(*socket_fd, res->ai_addr, res->ai_addrlen) == -1) { // <= worked for c++ <11
    int count = 0;
    bool connected = false;
    int errsv = -1;
    int bindret = 0;

    while (!connected && count < 100 && !flag_emergencySTOP) {
        bindret = ::bind(*socket_fd, res->ai_addr, res->ai_addrlen); //<= works for c++11
        errsv = errno;
        if (bindret == -1) {
            log_ERROR("MAIN| ERROR: Bind for ", typeString, " socket failed with errno=", integer(errsv), " (", strerror(errsv), "), retrying in 3 seconds");
            count++;
            usleep(3000000L); //sleep 3s
        } else {
            connected = true;
        }
    }
    //}
    freeaddrinfo(res);
    if (!connected) {
        log_ERROR("Binding the ", typeString, " socket failed for 100 times in a row, quitting");
        return -1;
    } else {
        log_NOTICE("MAIN| -> Bound ", typeString, " socket to [", ip, ":", sOut, "]", integer(bindret));
    }
    log_DEBUG("MAIN| Leaving createSocket().");
    return 0;
}

/**
 * Creates a UDP Channel. 
 * Includes a UDP NAT hole punch mechanism.
 * This is the IP version agnostic method.
 *
 * @param *udp_socket_fd        pointer to udp socket for whom we will create the channel
 * @param *tcp_socket_fd     pointer to tcp socket to receive confirmation message from client
 *
 * @return 				        0 on success, -1 on error
 */
int networkHandler::createUdpChannel(int *udp_socket_fd, int *tcp_socket_fd, string & foreignIP) {
    log_DEBUG("MAIN| Entering createUdpChannel()");
    // test input parameter
    if (*udp_socket_fd <= 0) {
        log_ERROR("MAIN| ERROR: Parameter udp_socket_fd has an invalid value.");
        return -1;
    }
    if (*tcp_socket_fd <= 0) {
        log_ERROR("MAIN| ERROR: Parameter tcp_socket_fd has an invalid value.");
        return -1;
    }
    // initialize variables
    // initialize timestruct for select call
    timeval wait;
    wait.tv_sec = 10;
    wait.tv_usec = 0;
    // initialize descriptor set for select call
    fd_set select_sockets_set;
    FD_ZERO(&select_sockets_set);
    FD_SET(*udp_socket_fd, &select_sockets_set);
    int read_chars = 0;
    unsigned char receive_buffer[MAX_MEASUREMENT_CHANNEL_MESSAGE_LENGTH];
    unsigned char send_buffer[8];
    uint32_t magic_number;
    uint32_t PUNCH_HEAD = 1337;
    uint32_t CONFIRM_HEAD = 7331;
    uint32_t REFUSE_HEAD = 7777;
    uint32_t MAGIC_NUMBER = inputH.get_measurement_magic();
    // Prepare send_buffer by placing the magic in the right place
    send_buffer[4] = (MAGIC_NUMBER >> 24) & 0xff;
    send_buffer[5] = (MAGIC_NUMBER >> 16) & 0xff;
    send_buffer[6] = (MAGIC_NUMBER >> 8) & 0xff;
    send_buffer[7] = (MAGIC_NUMBER >> 0) & 0xff;
    char fromstr[64];
    string ip;
    int port = 0;
    struct sockaddr_in *addr_ip4;
    struct sockaddr_in6 *addr_ip6;
    // Set up struct sockaddr_storage
    struct sockaddr_storage from;
    socklen_t fromlen = sizeof (struct sockaddr_storage);
    memset(&from, 0, fromlen);
    bool received_punch = false;
    // wait for NAT hole punch packet
    log_NOTICE("MAIN| ->  Waiting for a NAT hole punch packet.");
    while (!received_punch) {
        if (select((*udp_socket_fd) + 1, &select_sockets_set, (fd_set*) NULL, (fd_set*) NULL, &wait) > 0) {
            if (FD_ISSET(*udp_socket_fd, &select_sockets_set)) {
                read_chars = recvfrom(*udp_socket_fd, receive_buffer, MAX_MEASUREMENT_CHANNEL_MESSAGE_LENGTH - 1, 0, (struct sockaddr *) &from, &fromlen);
                // our packet consists of 8 bytes
                // - 4 byte uint32_t header number
                // - 4 byte uint32_t measurement_magic
                if (read_chars == 8) {
                    uint32_t head = (receive_buffer[0] << 24) |
                            (receive_buffer[1] << 16) |
                            (receive_buffer[2] << 8) |
                            (receive_buffer[3]);
                    if (head == PUNCH_HEAD) {
                        magic_number = (receive_buffer[4] << 24) |
                                (receive_buffer[5] << 16) |
                                (receive_buffer[6] << 8) |
                                (receive_buffer[7]);
                        if (magic_number == MAGIC_NUMBER) {
                            log_NOTICE("MAIN| -> Received a correct NAT magic number packet.");
                            // everything seems ok
                            if (from.ss_family == AF_INET) { //IPv4 packet
                                // select sockaddr_storage as ipv4 sockaddr_in
                                addr_ip4 = (struct sockaddr_in *) &from;
                                // read out ip and port
                                inet_ntop(from.ss_family, &(addr_ip4->sin_addr), fromstr, 64);
                                port = ntohs(addr_ip4->sin_port);
                                // set the default connection of the UDP socket
                                // one shot should be ok cause it is just a UDP socket
                                if (connect(*udp_socket_fd, (struct sockaddr *) addr_ip4, sizeof (struct sockaddr_in)) != 0) {
                                    log_ERROR("MAIN| ERROR: Could not connect UDP socket.");
                                    return -1;
                                }
                            } else if (from.ss_family == AF_INET6) { //IPv6 packet
                                // select sockaddr_storage as ipv6 sockaddr_in6
                                addr_ip6 = (struct sockaddr_in6 *) &from;
                                // read out ip and port
                                inet_ntop(from.ss_family, &(addr_ip6->sin6_addr), fromstr, 64);
                                port = ntohs(addr_ip6->sin6_port);
                                // set the default connection of the UDP socket
                                // one shot should be ok cause it is just a UDP socket
                                if (connect(*udp_socket_fd, (struct sockaddr *) addr_ip6, sizeof (struct sockaddr_in6)) != 0) {
                                    log_ERROR("MAIN| ERROR: Could not connect UDP socket.");
                                    return -1;
                                }
                            }
                            // connection established print msg and leave loop
                            ip = (string) fromstr;
                            if (ip.compare(foreignIP) == 0) {
                                log_NOTICE("MAIN| -> UDP socket successfully connected to ", ip, ":", integer(port), ".");
                            } else {
                                log_NOTICE("MAIN| -> UDP socket successfully connected to ", ip, ":", integer(port), " - seems to be behind a NAT.");
                                foreignIP = ip;
                            }
                            received_punch = true;
                        } else { // magic_number != MEASUREMENT_MAGIC
                            log_DEBUG("MAIN| FAILURE: Received a NAT magic number packet, but the magic number was not the expected one.");
                            log_DEBUG("Magic: ", integer(magic_number), " ,Expected: ", integer(MAGIC_NUMBER));
                        }
                    } else // head != PUNCH_MAGIC_HEAD
                        log_DEBUG("MAIN| FAILURE: Received a NAT magic number packet, but the header number was not the expected one.");
                } else // read_chars != 8
                    log_DEBUG("MAIN| FAILURE: Received a packet with the wrong size (Received ", integer(read_chars), "). Waiting for a NAT magic number packet.");
            } else // FD_ISSET() == false
                log_DEBUG("MAIN| FAILURE: FD_ISSET returned false. Weird, cause we are just observing one file descriptor.");
        } else { // select returned value < 0
            log_ERROR("MAIN| Error: No NAT hole punch packet received. Connection could not be established.");
            break;
        }
    } // end while(received_punch)
    if (!received_punch) //No punch received or error
    {
        // send error msg to client
        // TODO i think we do not need to send a REFUSE packet
        // the client will know when he does not receive an answer via UDP
        send_buffer[0] = (REFUSE_HEAD >> 24) & 0xff;
        send_buffer[1] = (REFUSE_HEAD >> 16) & 0xff;
        send_buffer[2] = (REFUSE_HEAD >> 8) & 0xff;
        send_buffer[3] = (REFUSE_HEAD >> 0) & 0xff;
        return -1;
    }
    // if we get here, we must have received a correct NAT hole punch
    // we will now send answer packets till we get a confirmation via tcp
    // send answer and wait for confirmation via TCP
    // set to zero, so the first packet will be send immediately
    wait.tv_sec = 0;
    wait.tv_usec = 0;
    FD_ZERO(&select_sockets_set);
    FD_SET(*tcp_socket_fd, &select_sockets_set);
    int received_confirm = false;
    int timeout_counter = 0;
    // prepare answer packet
    // we just need to change the header
    // the magic is still correct
    send_buffer[0] = (CONFIRM_HEAD >> 24) & 0xff;
    send_buffer[1] = (CONFIRM_HEAD >> 16) & 0xff;
    send_buffer[2] = (CONFIRM_HEAD >> 8) & 0xff;
    send_buffer[3] = (CONFIRM_HEAD >> 0) & 0xff;
    log_DEBUG("MAIN| Sending answer packets through UDP, while waiting on a confirmation packet via TCP.");
    // send answer packet till we get a confirmation via TCP
    while (!received_confirm) {
        int select_retval = select((*tcp_socket_fd) + 1, &select_sockets_set, (fd_set*) NULL, (fd_set*) NULL, &wait);
        if (select_retval > 0) {
            if (FD_ISSET(*tcp_socket_fd, &select_sockets_set)) {
                read_chars = recv(*tcp_socket_fd, receive_buffer, MAX_MEASUREMENT_CHANNEL_MESSAGE_LENGTH - 1, 0);
                if (read_chars == 8) {
                    uint32_t head = (receive_buffer[0] << 24) |
                            (receive_buffer[1] << 16) |
                            (receive_buffer[2] << 8) |
                            (receive_buffer[3]);
                    if (head == CONFIRM_HEAD) {
                        magic_number = (receive_buffer[4] << 24) |
                                (receive_buffer[5] << 16) |
                                (receive_buffer[6] << 8) |
                                (receive_buffer[7]);
                        if (magic_number == MAGIC_NUMBER) {
                            // we received our confirmation
                            // leave the loop
                            received_confirm = true;
                        } else // magic_number != MEASUREMENT_MAGIC_NUMBER
                            log_DEBUG("MAIN| FAILURE: Received a NAT confirmation packet, but the magic number was not the expected one.");
                    } else // head != CONFIRM_HEAD
                        log_DEBUG("MAIN| FAILURE: Received a NAT magic number packet, but the header number was not the expected one.");
                } else // read_chars != 8
                    log_DEBUG("MAIN| FAILURE: Received a packet with the wrong size (Received ", integer(read_chars), "). Waiting for a NAT magic number packet.");
            } else
                log_DEBUG("MAIN| FAILURE: FD_ISSET returned false. Weird, cause we are just observing one file descriptor.");
        } else if (select_retval == 0) {
            // timeout
            if (timeout_counter++ > 9) {
                // too many retries
                log_ERROR("MAIN| ERROR: Did not receive a confirmation packet.");
                return-1;
            }
            // time to send another confirmation packet
            // TODO react on partial sending
            if (send(*udp_socket_fd, &send_buffer, 8, 0) < 8) {
                log_DEBUG("MAIN| Failure: Could not send the confirmation packet.");
            } else
                log_DEBUG("MAIN| -> Send a confirmation packet.");
            // reset timer
            wait.tv_sec = 1;
            wait.tv_usec = 0;
            // reset bitmap
            // see connectToUdpChannel
            // there is a nice comment on this
            FD_SET(*tcp_socket_fd, &select_sockets_set);
        } else { // select error
            log_ERROR("MAIN| Error: Select returned an error. Time to quit.");
            return -1;
        }
    }
    // if we get here, we got a confirmation
    log_DEBUG("MAIN| Leaving createUdpChannel()");
    return 0;
}

/**
 * Creates a TCP Channel on the server. Just one connection is allowed.
 *
 * @param *listen_socket_fd     pointer to socket designated to listen for an incomming connection
 * @param *accept_socket_fd     pointer to socket designated to accept an incomming connection
 * @param ip_version            IPv4 = 0, IPv6 = 1
 * @param type                  UDP = 0, TCP = 1
 *
 * @return 				        0 on success, -1 on error
 */
int networkHandler::createTcpChannel(int *listen_socket_fd, int *accept_socket_fd) {
    log_DEBUG("MAIN| Entering createTcpChannel()");
    int errsv = -1;
    // Test the parameters
    if (*listen_socket_fd == -1) {
        log_ERROR("MAIN| ERROR: Parameter listen_socket_fd has an invalid value.");
        return -1;
    }
    // Initialize variables
    // Set up struct sockaddr_storage
    struct sockaddr_storage from;
    socklen_t from_len = sizeof (struct sockaddr_storage);
    memset(&from, 0, from_len);
    // Define sockaddr_in struct pointer
    struct sockaddr_in *addr_ip4;
    struct sockaddr_in6 *addr_ip6;
    uint32_t port = 0;
    string ip;
    char fromstr[64];
    // Listen on listen_socket for incoming connections
    if (listen(*listen_socket_fd, 3) == -1) {
        log_ERROR("MAIN| ERROR: Listening on listening socket failed.");
        return -1;
    }
    bool connected = false;
    while (!connected && !flag_emergencySTOP) {
        log_NOTICE("MAIN| -> Waiting for new connections on listening socket.");
        // Wait for a new connection and then accept it - only one connection for control channel client<->server.
        *accept_socket_fd = accept(*listen_socket_fd, (struct sockaddr *) &from, &from_len);
        errsv = errno;
        if (*accept_socket_fd <= 0) {
            log_ERROR("MAIN| ERROR: Accepting connection on control channel failed with errno = ", integer(errsv), " (", strerror(errsv), ")");
            log_NOTICE("MAIN| -> Still waiting for new connections on listening socket.");
        } else {
            connected = true;
        }
    }

    if (from.ss_family == AF_INET) { //IPv4 packet
        // select sockaddr_storage as ipv4 sockaddr_in
        addr_ip4 = (struct sockaddr_in *) &from;
        // read out ip and port
        inet_ntop(from.ss_family, &(addr_ip4->sin_addr), fromstr, 64);
        port = ntohs(addr_ip4->sin_port);
    } else if (from.ss_family == AF_INET6) { //IPv6 packet
        // select sockaddr_storage as ipv6 sockaddr_in6
        addr_ip6 = (struct sockaddr_in6 *) &from;
        // read out ip and port
        inet_ntop(from.ss_family, &(addr_ip6->sin6_addr), fromstr, 64);
        port = ntohs(addr_ip6->sin6_port);
    }
    ip = (string) fromstr;
    log_NOTICE("MAIN| -> Accepted a new connection from ", ip, ":", integer(port), ".");
    log_NOTICE("MAIN| -> Server welcome socket closed.");
    close(*listen_socket_fd);

    // Set Client Addresses for server depending on connected client, only if nothing was entered at start
    if (inputH.get_client_cAddress().compare("0.0.0.0") == 0) {
        inputH.set_client_cAddress((string) ip);
        inputH.set_client_mAddress((string) ip);
        inputH.set_client_sAddress((string) ip);
    }
    log_DEBUG("MAIN| Leaving createTcpChannel()");
    return 0;
}

/**
 * Tries to establish a connection to a UDP server
 * including UDP hole punching and confirmation
 *
 * @param serverIP		IPv4 or IPv6 server address as string
 * @param serverPort	Port as unsigned int32
 * @param ip_version		0 for IPv4, 1 for IPv6
 *
 * @return	0 on success, -1 on error
 */
int networkHandler::connectToUdpChannel(string ip, uint32_t port, int *udp_socket_fd, int *tcp_socket_fd, uint32_t ip_version) {
    log_DEBUG("MAIN| Entering connectToUdpChannel()");
    timeHandler timing(inputH.get_timingMethod());
    // Test parameters
    if (*udp_socket_fd <= 0) {
        log_ERROR("MAIN| ERROR: Parameter udp_socket_fd has an invalid value.");
        return -1;
    }
    if (*tcp_socket_fd <= 0) {
        log_ERROR("MAIN| ERROR: Parameter tcp_socket_fd has an invalid value.");
        return -1;
    }
    // Initialize variables
    timeval wait;
    wait.tv_sec = 0;
    wait.tv_usec = 0;
    // initialize descriptor set for select call
    fd_set select_sockets_set;
    FD_ZERO(&select_sockets_set);
    FD_SET(*udp_socket_fd, &select_sockets_set);
    int read_chars = 0;
    unsigned char receive_buffer[MAX_MEASUREMENT_CHANNEL_MESSAGE_LENGTH];
    unsigned char send_buffer[8];
    memset(&receive_buffer, 0, sizeof (receive_buffer));
    memset(&send_buffer, 0, sizeof (send_buffer));
    uint32_t magic_number;
    uint32_t PUNCH_HEAD = 1337;
    uint32_t CONFIRM_HEAD = 7331;
    uint32_t MAGIC_NUMBER = inputH.get_measurement_magic();
    // Prepare send_buffer
    send_buffer[0] = (PUNCH_HEAD >> 24) & 0xff;
    send_buffer[1] = (PUNCH_HEAD >> 16) & 0xff;
    send_buffer[2] = (PUNCH_HEAD >> 8) & 0xff;
    send_buffer[3] = (PUNCH_HEAD >> 0) & 0xff;
    send_buffer[4] = (MAGIC_NUMBER >> 24) & 0xff;
    send_buffer[5] = (MAGIC_NUMBER >> 16) & 0xff;
    send_buffer[6] = (MAGIC_NUMBER >> 8) & 0xff;
    send_buffer[7] = (MAGIC_NUMBER >> 0) & 0xff;
    // FIXME langech
    // workaround, cause we need the portnumber as a string
    // would be better to store it directly this way in the object
    stringstream ssOut;
    string sOut;
    ssOut.str("");
    ssOut << port;
    sOut = ssOut.str();
    // Prepare address structs
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof (hints));
    if (ip_version == 0) //IPv4
        hints.ai_family = AF_INET;
    else
        hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(ip.c_str(), sOut.c_str(), &hints, &res) != 0) {
        log_ERROR("MAIN| ERROR: Could not create sockaddr structure.");
        freeaddrinfo(res);
        return -1;
    }
    int conret = -1;
    int errsv;
    int trys = 0;
    // connect UDP socket to default ip:port
    while (conret != 0 && !flag_emergencySTOP && trys < 10) {
        conret = connect(*udp_socket_fd, res->ai_addr, res->ai_addrlen);
        errsv = errno;
        if (conret != 0) {
            log_ERROR("MAIN| ERROR: Could not connect UDP socket to ", ip, ":", integer(port), " with error ", strerror(errsv), ".");
            trys++;
            if (trys < 10) {
                log_NOTICE("MAIN| Retrying to connect UDP socket in 1 second");
                timing.sleepFor(1000000000LL); //sleep 1 second}
            }
        }
    }
    freeaddrinfo(res);

    if (conret != 0) {
        log_ERROR("MAIN| ERROR: Giving up after 10 trys to connect UDP socket.");
        return -1;
    } else {
        log_NOTICE("MAIN| -> Connected UDP socket to ", ip, ":", sOut, ".");
    }
    int timeout_counter = 0;
    bool received_confirm = false;
    int select_retval = 0;
    // Send UDP hole punches and wait for confirmation
    log_NOTICE("MAIN| -> Sending UDP hole punch packet to ", ip, ":", sOut, ".");
    while (!received_confirm) {
        select_retval = select((*udp_socket_fd) + 1, &select_sockets_set, (fd_set*) NULL, (fd_set*) NULL, &wait);
        if (select_retval > 0) {
            if (FD_ISSET(*udp_socket_fd, &select_sockets_set)) {
                read_chars = recv(*udp_socket_fd, receive_buffer, MAX_MEASUREMENT_CHANNEL_MESSAGE_LENGTH - 1, 0);
                if (read_chars == 8) {
                    uint32_t head = (receive_buffer[0] << 24) |
                            (receive_buffer[1] << 16) |
                            (receive_buffer[2] << 8) |
                            (receive_buffer[3]);
                    if (head == CONFIRM_HEAD) {
                        magic_number = (receive_buffer[4] << 24) |
                                (receive_buffer[5] << 16) |
                                (receive_buffer[6] << 8) |
                                (receive_buffer[7]);
                        if (magic_number == MAGIC_NUMBER) {
                            // we received our confirmation
                            // leave the loop
                            received_confirm = true;
                        } else // magic_number != MEASUREMENT_MAGIC_NUMBER
                            log_NOTICE("MAIN| FAILURE: Received a NAT confirmation packet, but the magic number was not the expected one.");
                    } else // head != CONFIRM_HEAD
                        log_NOTICE("MAIN| FAILURE: Received a NAT magic number packet, but the header number was not the expected one.");
                } else // read_chars != 8
                    log_NOTICE("MAIN| FAILURE: Received a packet with the wrong size (Received ", integer(read_chars), "). Waiting for a NAT magic number packet.");
            } else
                log_NOTICE("MAIN| FAILURE: FD_ISSET returned false. Weird, cause we are just observing one file descriptor.");
        } else if (select_retval == 0) {
            // timeout
            if (timeout_counter++ > 9) {
                log_ERROR("MAIN| ERROR: Did not get an answer from the server.");
                return -1;
            }
            // time to send another punch
            if (send(*udp_socket_fd, &send_buffer, 8, 0) < 8)
                log_NOTICE("MAIN| FAILURE: Could not send UDP punch.");
            else
                log_NOTICE("MAIN| -> UDP hole punch packet sent to server.");
            // reset timer
            wait.tv_sec = 1;
            wait.tv_usec = 0;
            // reinitialize Set
            // cause this fu***ng select call clears the bitmap each time
            // this trap did cost me half a day ...
            FD_SET(*udp_socket_fd, &select_sockets_set);
        } else { // select error
            log_ERROR("MAIN| ERROR: Select returned an error. Time to quit.");
            return -1;
        }
    }
    // we received an answer from the server
    // time to reply with a TCP confirmation msg
    send_buffer[0] = (CONFIRM_HEAD >> 24) & 0xff;
    send_buffer[1] = (CONFIRM_HEAD >> 16) & 0xff;
    send_buffer[2] = (CONFIRM_HEAD >> 8) & 0xff;
    send_buffer[3] = (CONFIRM_HEAD >> 0) & 0xff;
    if (send(*tcp_socket_fd, &send_buffer, 8, 0) < 8) {
        log_ERROR("MAIN| Failed to send the confirmation message using the control channel.");
        return -1;
    }
    log_NOTICE("MAIN| -> Successfully connected UDP channel with server[", ip, ":", integer(port), "].");
    log_DEBUG("MAIN| Leaving connectToUDPChannel()");
    return 0;
}

/**
 * Tries to establish a connection to a TCP server
 *
 * @param ip		            IPv4 or IPv6 server address as string
 * @param port	                Port as unsigned int32
 * @param *connect_socket_fd    Socket to use
 * @param ip_version	        0 for IPv4, 1 for IPv6
 *
 * @return	0 on success, -1 on error
 */
int networkHandler::connectToTcpChannel(string ip, uint32_t port, int *connect_socket_fd, uint32_t ip_version) {
    log_DEBUG("MAIN| Entering connectToTcpChannel()");
    // Testing parameters
    if (*connect_socket_fd <= 0) {
        log_ERROR("MAIN| ERROR: Parameter connect_socket_fd has an invalid value.");
        return -1;
    }
    // Initializing variables
    class timeHandler timing;
    timing.setTimingMethod(inputH.get_timingMethod());
    // FIXME langech
    // workaround, cause we need the portnumber as a string
    // would be better to store it directly this way in the object
    stringstream ssOut;
    string sOut;
    ssOut.str("");
    ssOut << port;
    sOut = ssOut.str();
    // Prepare address structs
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof (hints));
    if (ip_version == 0) //IPv4
        hints.ai_family = AF_INET;
    else
        hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(ip.c_str(), sOut.c_str(), &hints, &res) != 0) {
        log_ERROR("MAIN| ERROR: Could not create sockaddr structure.");
        freeaddrinfo(res);
        return -1;
    }

    int count = 0;
    bool connected = false;
    int errsv = -1;
    int connect_ret = 0;

    while (!connected && count < 100 && !flag_emergencySTOP) {
        connect_ret = connect(*connect_socket_fd, res->ai_addr, res->ai_addrlen);
        errsv = errno;
        if (connect_ret == -1) {
            log_ERROR("MAIN| ERROR: Could not connect TCP socket to ", ip, ":", integer(port), " with errno=",
                    integer(errsv), " (", strerror(errsv), "), retrying in 3 seconds.");
            count++;
            usleep(3000000L); //sleep 3s
        } else {
            connected = true;
        }
    }
    freeaddrinfo(res);
    if (!connected) {
        log_ERROR("MAIN| ERROR: Could not connect TCP socket to ", ip, ":", integer(port), " with errno="
                , integer(errsv), " (", strerror(errsv), "), giving up!");
        return -1;
    } else {
        log_NOTICE("MAIN| -> Connected to TCP channel with server[", ip, ":", integer(port), "].");
    }
    log_DEBUG("MAIN| Leaving connectToTcpChannel()");
    return 0;
}

uint32_t networkHandler::getHeaderSizeUDP() {
    return HEADERSIZE_UDP;
}

uint32_t networkHandler::getHeaderSizeIP() {
    if (inputH.get_ipVersion() == 0) {
        return HEADERSIZE_IP4;
    } else {
        return HEADERSIZE_IP6;
    }
}

uint32_t networkHandler::getHeaderSizeEthernetIpUdp() {
    return getHeaderSizeUDP() + getHeaderSizeIP() + getHeaderSizeEthernet();
}

int networkHandler::unlockMeasurementDeviceSend() {
    //log_DEBUG("TMSE| Devicequeue test.");
    if (mSendingMaxQueueSize > -1 && flag_mSendingThreadCurrentlyWaitingForQueue) {
        int ok = testMeasurementDeviceQueueSize();
        if (ok) {
            log_DEBUG("TMSE| Devicequeue is ready to send.");
            pthread_mutex_lock(&networkHandler_measurementDeviceQueue);
            pthread_cond_signal(&networkHandler_cond_measurementDeviceQueue);
            pthread_mutex_unlock(&networkHandler_measurementDeviceQueue);
        } else {
            log_DEBUG("TMSE| Devicequeue is not ready to send.");
        }
        if (ok) {
            sched_yield();
        }
    }
    return 0;
}

int networkHandler::testMeasurementDeviceQueueSize() {
    if (mSendingMaxQueueSize > -1) {
        return ((int) getMeasurementDeviceQueueSize()) <= mSendingMaxQueueSize;
    }
    return 0;
}

uint32_t networkHandler::getMeasurementDeviceQueueSize() {
    FILE *fp;
    fp = fopen("/proc/queueLengthDevice", "r");
    if (fp != NULL) {
        char read[20];
        int counter = 0;
        while ((read[counter++] = fgetc(fp)) != EOF) {
        }
        read[counter] = 0;

        uint32_t value;
        sscanf(read, "%d", &value);
        fclose(fp);
        log_DEBUG("TMSE| Queuesize: ", integer(value));
        return value;
    } else {
        log_DEBUG("TMSE| Could not open Queusize file!");
        return -1;
    }
}

int networkHandler::getMaxDeviceQueueSizeForInterface(const char* if_name) {
    struct ifreq ifr;
    size_t if_name_len = strlen(if_name);
    memcpy(ifr.ifr_name, if_name, if_name_len); //copy string to ifr struct
    ifr.ifr_name[if_name_len] = 0; // ending string

    int fd = socket(AF_UNIX, SOCK_DGRAM, 0); // create tmp socket to call ioctl
    if (fd == -1) {
        return -1;
    }
    if (ioctl(fd, SIOCGIFTXQLEN, &ifr) == -1) {
        close(fd);
        return -1;
    }
    close(fd);

    return ifr.ifr_qlen;
}

/**
 * 
 * @return 0 if both IP String representations match (which will happen, too, 
 *         if the channels are not yet created, thus check after safeguardchannel creation!
 */
string networkHandler::getMeasurementChannelForeignIP() const {
    return MeasurementChannelForeignIP;
}

string networkHandler::getSafeguadChannelForeignIP() const {
    return SafeguadChannelForeignIP;
}

int64_t networkHandler::getLastSafeguardChannelSendNs_() const {
    return lastSafeguardChannelSendNs_;
}

uint32_t networkHandler::getHeaderSizeEthernet() {
    return HEADERSIZE_ETHERNET;
}

circularQueue<payloadStruct>& networkHandler::getReceiveQueue() {
    return mReceivingQueue;
}

circularQueue<TBUSDynamicPayload>& networkHandler::getSafeguardReceivingQueue() {
    return sReceivingQueue;
}

circularQueue<TBUSDynamicPayload>& networkHandler::getSafeguardSendQueue() {
    return sSendingQueue;
}

circularQueue<TBUSFixedPayload>& networkHandler::getSendingQueue() {
    return mSendingQueue;
}

circularQueue<payloadStruct>& networkHandler::getSentQueue() {
    return mSentQueue;
}

bool networkHandler::get_emergencySTOP() {
    return flag_emergencySTOP;
}


