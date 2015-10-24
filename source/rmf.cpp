//============================================================================
// Name        : rmf.cpp
// Author      : Sebastian Wilken, Christian Lange, Tobias Krauthoff, Franz Kary, Norbert Goebel
// Description : Rate Measurement Framework
//============================================================================
// Namespace
using namespace std;

//#define GOOGLEPROFILECPU 
//#define GOOGLEPROFILEHEAP

// External logging framework
int pantheois_logLevel = 3;
#include "pantheios/pantheiosHeader.h"
#include "pantheios/pantheiosFrontEnd.h"

// google performance tools
#ifdef GOOGLEPROFILECPU
#include <gperftools/profiler.h>
#endif /* GOOGLEPROFILECPU */

#ifdef GOOGLEPROFILEHEAP
#include <gperftools/heap-profiler.h>
#endif /* GOOGLEPROFILEHEAP */

// Includes
//#include <gperftools/heap-profiler.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string>
#include <sched.h>
#include <unistd.h> //usleep

#include "rmfMakros.h"  //32 or 64bit
#include "inputHandler.h"
#include "networkHandler.h"
#include "timeHandler.h"
#include "systemOptimization.h"


// Global classes
class inputHandler inputH;
class networkHandler network;
class systemOptimization systemOpt;

static void *threadedMeasurmentSender(void *ptr);
static void *threadedMeasurmentReceiver(void *ptr);
static void *threadedSafeguardSender(void *ptr);
static void *threadedSafeguardReceiver(void *ptr);
static void *threadedReceiverDataDistributer(void *ptr);
static void *threadedSenderDataDistributer(void *ptr);

void handleSigpipe(int);
void handleSigint(int);

/**
 * Main program
 *
 * @param argc	Commandline Parameter Count
 * @param argv	Commandline Parameters
 *
 * @return
 */
int main(int argc, char *argv[]) {

#ifdef IS64BIT
    std::cout << "\n\nCompiled for 64-BIT\n\n\n";
#endif
#ifdef IS32BIT
    std::cout << "\n\nCompiled for 32-BIT\n\n\n";
#endif            
#ifdef GOOGLEPROFILECPU
    std::cout << "Profiler Start\n";
    ProfilerStart("rmf-profile");
#endif /* GOOGLEPROFILECPU */

#ifdef GOOGLEPROFILEHEAP
    HeapProfilerStart("test");
#endif /* GOOGLEPROFILEHEAP */

#ifdef GOOGLEPROFILEHEAP
    HeapProfilerDump("test");
#endif /* GOOGLEPROFILEHEAP */

    class timeHandler timing;

    // Initialize the Logging Framework, get filename and loglevel from agrv
    if (inputH.activateLogging(argc, argv) == -1) {
        exit(-1);
    }
    // From this point on we are able to use the Pantheios logging framework.

    // Install a signal handler to catch SIGPIPE signal (remote TCP socket closed) and handle it
    signal(SIGPIPE, handleSigpipe);

    // Install a signal handler to catch SIGINT signal (crtl-c)
    signal(SIGINT, handleSigint);

#ifdef GOOGLEPROFILEHEAP
    HeapProfilerDump("test");
#endif /* GOOGLEPROFILEHEAP */

    // Parse the Parameter from argv and save it into the input class
    if (inputH.parseParameter(argc, argv) == -1) {
        std::cout << "ERROR parsing parameters!\n";
        exit(-1);
    }

#ifdef GOOGLEPROFILEHEAP
    HeapProfilerDump("test");
#endif /* GOOGLEPROFILEHEAP */

    // Check if the member variables which we set above are sane
    if (inputH.checkSanityOfInput() == -1) {
        exit(-1);
    }

    // Enable logging file for client after we know what we are going to try, server will follow later
    if (inputH.get_nodeRole() == 0)
        inputH.createLogfileName();

    // Set or auto detect the timing method
    if (timing.setTimingMethod(inputH.get_timingMethod()) == -1) {
        exit(-1);
    }

    // Set the scheduling method of the CPU for this process and other stuff
    if (systemOpt.activateMemoryLocking(inputH.get_memorySwapping()) == -1) {
        exit(-1);
    }
    if (systemOpt.setMainProcessCpuAffinty(inputH.get_cpuAffinity()) == -1) {
        exit(-1);
    }
    if (systemOpt.activateRealtimeScheduling(inputH.get_processScheduling()) == -1) {
        exit(-1);
    }
    if (systemOpt.setCPU_FrequencyAndGovernor(inputH.get_scalingGovernor()) == -1) {
        exit(-1);
    }

    //do {// ... while( input.get_nodeRole() == 1 )

    // Reset all values of this class back to default. Needed if we are the server and the loop restarted
    network.initValues();

    // Create or connect to control channel
    if (network.establishControlChannel(inputH.get_nodeRole(),
            inputH.get_client_cAddress(),
            inputH.get_server_cAddress(),
            inputH.get_client_cPort(),
            inputH.get_server_cPort(),
            inputH.get_client_cInterface(),
            inputH.get_server_cInterface(),
            inputH.get_ipVersion()) == -1) {
        exit(-1);
    }

    // Synchronization of client and servers basic framework parameters
    if (network.syncBasicParameter(inputH.get_nodeRole(),
            inputH.get_measurementMethodClass()->get_Id(),
            inputH.get_measuredLink(),
            inputH.get_experimentDuration(),
            inputH.get_safeguardChannel(),
            inputH.get_client_mPort(),
            inputH.get_client_sPort(),
            inputH.get_server_mPort(),
            inputH.get_server_sPort(),
            inputH.get_client_mAddress(),
            inputH.get_client_sAddress(),
            inputH.get_server_mAddress(),
            inputH.get_server_sAddress(),
            inputH.get_inputFileNameFormat()) == -1) {
        exit(-1);
    }


    // Reset logging file name for server after knowing which measurement method will be used
    if (inputH.get_nodeRole() == 1)
        inputH.createLogfileName();


    // Create Measurement UDP Channel
    if (network.establishMeasurementChannel(inputH.get_nodeRole(),
            inputH.get_client_mAddress(),
            inputH.get_server_mAddress(),
            inputH.get_client_mPort(),
            inputH.get_server_mPort(),
            inputH.get_client_mInterface(),
            inputH.get_server_mInterface(),
            inputH.get_ipVersion()) == -1) {
        exit(-1);
    }

    // Create Safeguard channel if activated
    if (inputH.get_safeguardChannel() == 1) {
        if (network.establishSafeguardChannel(inputH.get_nodeRole(),
                inputH.get_client_sAddress(),
                inputH.get_server_sAddress(),
                inputH.get_client_sPort(),
                inputH.get_server_sPort(),
                inputH.get_client_sInterface(),
                inputH.get_client_sGateway(),
                inputH.get_server_sInterface(),
                inputH.get_ipVersion()) == -1) {
            exit(-1);
        }
        if (inputH.amIServer()) {
            if (inputH.get_client_sAddress().compare(inputH.get_client_mAddress()) != 0 &&
                    network.getMeasurementChannelForeignIP().compare(network.getSafeguadChannelForeignIP()) == 0) {
                pantheios::log_ERROR("-------------------------------------------------------------------------------------------------------");
                pantheios::log_ERROR("-                                                                                                     -");
                pantheios::log_ERROR("-  Error! MeasurementChannel and SafeguardChannel share the same foreignIP = ", network.getMeasurementChannelForeignIP());
                pantheios::log_ERROR("-         while the config suggests the usage of two unequal IPs!                                     -");
                pantheios::log_ERROR("-         MeasurementChannel IP in sync:       ", inputH.get_client_mAddress());
                pantheios::log_ERROR("-         MeasurementChannel IP in hole punch: ", network.getMeasurementChannelForeignIP());
                pantheios::log_ERROR("-         SafeguardChannel   IP in sync:       ", inputH.get_client_sAddress());
                pantheios::log_ERROR("-         SafeguardChannel   IP in hole punch: ", network.getSafeguadChannelForeignIP());
                pantheios::log_ERROR("-                                                                                                     -");
                pantheios::log_ERROR("-  Emergency Stop!                                                                                    -");
                pantheios::log_ERROR("-                                                                                                     -");
                pantheios::log_ERROR("-------------------------------------------------------------------------------------------------------");
                network.set_emergencySTOP("MeasurementChannel and SafeguardChannel share the same foreignIP");
                exit(-2);
            }
        }
    }


    // Threading
    pthread_t mSendingThread, mReceivingThread, sSendingThread, sReceivingThread, receiverDataDistributerThread, senderDataDistributerThread;

    // Init Thread attributes and set thread scheduling method
    pthread_attr_t generalThreadAttributes;
    pthread_attr_init(&generalThreadAttributes);
    systemOpt.activateRealtimeThreadScheduling(inputH.get_processScheduling(), &generalThreadAttributes, "All PThreads");

    /* Thread Number for systemOpt.setThreadCpuAffinity()
     * --------------------------------------------------
     * 0 => mReceivingThread
     * 1 => mSendingThread
     * 2 => receiverDataDistributer
     * 3 => senderDataDistributer
     * 4 => sSendingThread
     * 5 => sReceivingThread
     * --------------------------------------------------
     */

    // Create Threads we need
    pthread_create(&mReceivingThread, &generalThreadAttributes, threadedMeasurmentReceiver, NULL);
    pthread_create(&mSendingThread, &generalThreadAttributes, threadedMeasurmentSender, NULL);
    pthread_create(&receiverDataDistributerThread, &generalThreadAttributes, threadedReceiverDataDistributer, NULL);
    pthread_create(&senderDataDistributerThread, &generalThreadAttributes, threadedSenderDataDistributer, NULL);
    if (inputH.get_safeguardChannel() == 1) {
        pthread_create(&sSendingThread, &generalThreadAttributes, threadedSafeguardSender, NULL);
        pthread_create(&sReceivingThread, &generalThreadAttributes, threadedSafeguardReceiver, NULL);
    }

    // Check sync and finish initialization
    if (network.finishInit(inputH.get_nodeRole()) == -1) {
        exit(-1);
    }

    // All Thread are now up and running adjust scheduling and cpu affinity
    systemOpt.setThreadCpuAffinity(inputH.get_cpuAffinity(), 0, mReceivingThread, "mReceivingThread");
    systemOpt.setThreadCpuAffinity(inputH.get_cpuAffinity(), 1, mSendingThread, "mSendingThread");
    systemOpt.setThreadCpuAffinity(inputH.get_cpuAffinity(), 2, receiverDataDistributerThread, "receiverDataDistributerThread");
    systemOpt.setThreadCpuAffinity(inputH.get_cpuAffinity(), 3, senderDataDistributerThread, "senderDataDistributerThread");
    if (inputH.get_safeguardChannel() == 1) {
        systemOpt.setThreadCpuAffinity(inputH.get_cpuAffinity(), 4, sSendingThread, "sSendingThread");
        systemOpt.setThreadCpuAffinity(inputH.get_cpuAffinity(), 5, sReceivingThread, "sReceivingThread");
    }

    systemOpt.activateRealtimeThreadPriority(0, inputH.get_processScheduling(), mReceivingThread, "mReceivingThread");
    systemOpt.activateRealtimeThreadPriority(1, inputH.get_processScheduling(), mSendingThread, "mSendingThread");
    systemOpt.activateRealtimeThreadPriority(2, inputH.get_processScheduling(), receiverDataDistributerThread, "receiverDataDistributerThread");
    systemOpt.activateRealtimeThreadPriority(3, inputH.get_processScheduling(), senderDataDistributerThread, "senderDataDistributerThread");
    if (inputH.get_safeguardChannel() == 1) {
        systemOpt.activateRealtimeThreadPriority(4, inputH.get_processScheduling(), sSendingThread, "sSendingThread");
        systemOpt.activateRealtimeThreadPriority(5, inputH.get_processScheduling(), sReceivingThread, "sReceivingThread");
    }
    // Framework is now ready



    // Select measurement Method and begin initialization
#ifdef GOOGLEPROFILEHEAP
    HeapProfilerDump("test");
#endif /* GOOGLEPROFILEHEAP */


    int returnInit = inputH.get_measurementMethodClass()->initMeasurement();

    pantheios::log_INFORMATIONAL("MAIN| Before testing returnInit.");
    // If no thread has died, monitor them while the experiment is running
    if (returnInit != 0) {
        network.set_emergencySTOP("Error initialising the measurement.");
    }

    pantheios::log_INFORMATIONAL("MAIN| Before joining mSendingThread.");
    // Wait for all Thread to be terminated
    pthread_join(mSendingThread, NULL);

    pantheios::log_INFORMATIONAL("MAIN| Before joining mReceivingThread.");
    // pthread_join(mReceivingThread, NULL);
    //wait 5 seconds to join mReceivingThread
    int mReceivingThreadJoinError;
    timespec fivesec = timing.ns_to_timespec(timing.getCurrentTimeNs() + 5000000000LL);
    mReceivingThreadJoinError = pthread_timedjoin_np(mReceivingThread, NULL, &fivesec);

    pantheios::log_INFORMATIONAL("MAIN| Before joining senderDataDistributorThread.");
    pthread_join(senderDataDistributerThread, NULL);
    pantheios::log_INFORMATIONAL("MAIN| Before joining receiverDataDistributorThread.");
    pthread_join(receiverDataDistributerThread, NULL);

    pantheios::log_INFORMATIONAL("MAIN| Before joining safeguard channel.");
    if (inputH.get_safeguardChannel() == 1) {
        pthread_join(sSendingThread, NULL);
        pthread_join(sReceivingThread, NULL);
    }

    pantheios::log_INFORMATIONAL("MAIN| Before closing Sockets");
    // Close all Sockets
    network.closeSockets();

    if (mReceivingThreadJoinError != 0) {
        pantheios::log_INFORMATIONAL("MAIN| mReceivingThread waited join returned with ", pantheios::integer(mReceivingThreadJoinError));
        //mReceivingThread join did not work in the first attempt
        pantheios::log_INFORMATIONAL("MAIN| Before joining mReceivingThread.");
        pthread_join(mReceivingThread, NULL);
    }
    pantheios::log_INFORMATIONAL("MAIN| Before the end");


    if (inputH.get_nodeRole() == 1) {
        inputH.reset();
        timing.sleepFor(1000000000LL); //sleep 1 s

        // Disable logfile, server will set it again after "MEASURE_BASIC_SYNC", data will be logged then
        pantheios_be_file_setFilePath(NULL);
    }


    //} while (inputH.get_nodeRole() == 1);

    pantheios::log_NOTICE("MAIN| -----------------------");
    pantheios::log_NOTICE("MAIN| Experiment finished.");
    pantheios::log_NOTICE("MAIN| ");
#ifdef GOOGLEPROFILEHEAP
    HeapProfilerDump("test");
    HeapProfilerStop();
#endif /* GOOGLEPROFILEHEAP */

#ifdef GOOGLEPROFILECPU
    pantheios::log_NOTICE("MAIN| ProfilerStop");
    ProfilerStop();
#endif /* GOOGLEPROFILECPU */
    return (0);
}


// Threads
//---------------------------------------------

static void *threadedMeasurmentSender(void *in) {
    int returnValue = network.threadedMeasurmentSender();
    if (returnValue == -1) {
        pantheios::log_ERROR("MAIN| Measurement sending thread stop with an error.");
        network.set_emergencySTOP("Error creating Measurement Sender Thread");
        return ((void*) - 1);
    }
    pantheios::log_DEBUG("MAIN| Measurement sending thread terminated nicely.");
    return ((void*) 0);
}

static void *threadedMeasurmentReceiver(void *in) {
    int returnValue = network.threadedMeasurmentReceiver();
    if (returnValue == -1) {
        pantheios::log_ERROR("MAIN| Measurement receiver thread stop with an error.");
        network.set_emergencySTOP("Error creating Measurement Receiver Thread.");
        return ((void*) - 1);
    }
    pantheios::log_DEBUG("MAIN| Measurement receiver thread terminated nicely.");
    return ((void*) 0);
}

static void *threadedSafeguardSender(void *in) {
    int returnValue = network.threadedSafeguardSender();
    if (returnValue == -1) {
        pantheios::log_ERROR("MAIN| Safeguard sending thread stop with an error.");
        network.set_emergencySTOP("Error creating Safeguard Sending Thread.");
        return ((void*) - 1);
    }

    pantheios::log_DEBUG("MAIN| Safeguard sending thread terminated nicely.");
    return ((void*) 0);
}

static void *threadedSafeguardReceiver(void *in) {
    int returnValue = network.threadedSafeguardReceiver();
    if (returnValue == -1) {
        pantheios::log_ERROR("MAIN| Safeguard receiver thread stop with an error.");
        network.set_emergencySTOP("Error creating Safeguard Receiver Thread.");
        return ((void*) - 1);
    }
    pantheios::log_DEBUG("MAIN| Safeguard receiver thread terminated nicely.");
    return ((void*) 0);
}

static void *threadedReceiverDataDistributer(void *ptr) {
    int returnValue = network.threadedReceiverDataDistributer();
    if (returnValue == -1) {
        pantheios::log_ERROR("MAIN| Receiver Data Distributer thread stop with an error.");
        network.set_emergencySTOP("Error creating Receiver Data Distributor");
        return ((void*) - 1);
    }
    pantheios::log_DEBUG("MAIN| Receiver Data Distributer thread terminated nicely.");
    return ((void*) 0);
}

static void *threadedSenderDataDistributer(void *ptr) {
    int returnValue = network.threadedSenderDataDistributer();
    if (returnValue == -1) {
        pantheios::log_ERROR("MAIN| Sender Data Distributer thread stop with an error.");
        network.set_emergencySTOP("Error creating Sender Data Distributor.");
        return ((void*) - 1);
    }
    pantheios::log_DEBUG("MAIN| Sender Data Distributer terminated nicely.");
    return ((void*) 0);
}
//---------------------------------------------


// Signals
//---------------------------------------------

void handleSigpipe(int signum) {
    pantheios::log_ERROR("MAIN| SIGPIPE Signal. A remote TCP connection closed. Stopping Experiment.");
    network.set_emergencySTOP("SIGPIPE signal received.");
}

void handleSigint(int signum) {
    pantheios::log_ERROR("MAIN| SIGINT Signal. Cleaning up and exit.");
    network.set_emergencySTOP("SIGINT signal received.");

    if (inputH.get_scalingGovernor() == 1)
        systemOpt.reset_ScalingGovenor();

    usleep(5000000); // Wait 5 seconds before the server restarts for cleanup

    exit(-1);
}
//---------------------------------------------
