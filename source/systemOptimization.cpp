/* systemOptimization.cpp */

#include "systemOptimization.h"

// External logging framework
#include "pantheios/pantheiosHeader.h"

#include <pthread.h>
#include <sched.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/resource.h>
#include <stdio.h>
#include <errno.h>

systemOptimization::systemOptimization()
{
	 this->systemCpuCount = sysconf( _SC_NPROCESSORS_ONLN );
	 this->systemMemory   = sysconf( _SC_PAGE_SIZE ) * sysconf( _SC_PHYS_PAGES );

	 systemScalingGovernor = "";
}

systemOptimization::~systemOptimization()
{
	this->reset_ScalingGovenor();
}

int systemOptimization::reset_ScalingGovenor()
{pantheios::log_DEBUG("MAIN| Entering reset_ScalingGovenor()");

	if( this->systemScalingGovernor.length() > 0 && geteuid() == 0 )
	{
		string setString("echo " + this->systemScalingGovernor + " > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
		if ( system(setString.c_str()) != 0)
			pantheios::log_DEBUG("MAIN| Could not reset Kernel scaling governor to \"", this->systemScalingGovernor.c_str(), "\".");
		else
			pantheios::log_DEBUG("MAIN| Resetting Kernel scaling governor to \"", this->systemScalingGovernor.c_str(), "\".");
	}


	pantheios::log_DEBUG("MAIN| Leaving reset_ScalingGovenor()");
	pantheios::log_DEBUG("MAIN| ");

	return(0);
}

/**
 * Set the system scaling govenor to performance
 *
 * @param enablePerformance	true or false
 *
 * @return 0
 */
int systemOptimization::setCPU_FrequencyAndGovernor(uint32_t enablePerformance)
{pantheios::log_DEBUG("MAIN| Entering setCPU_FrequencyAndGovernor()");

	if( enablePerformance == 1 )
	{
		// Check if the Governor is supported
		struct stat status;
		string path = "/sys/devices/system/cpu/cpu0/cpufreq";

		if (  stat( path.c_str(), &status ) == 0 )
		{
			FILE *consoleFeedback;
			string supportedGovernors = "";
			char buffer[512];

			consoleFeedback = popen("cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors", "r");
			if( consoleFeedback != NULL ){
				if( fgets(buffer, 512, consoleFeedback) != NULL )
					supportedGovernors = (string) buffer;
				pclose(consoleFeedback);
			}
			pantheios::log_DEBUG("MAIN| -> Kernel supports the following scaling governors: ", supportedGovernors.c_str());

			// Check if performance governor is supported
			if(  supportedGovernors.find("performance") != string::npos )
			{
				// Remember old Governor, reset in destructor
				consoleFeedback = popen("cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", "r");
				if( consoleFeedback != NULL ){
					if( fgets(buffer, 512, consoleFeedback) != NULL ) {
						// delete the last new line char
						string tmp = (string) buffer;
						tmp = tmp.substr(0, tmp.size()-1); 
						this->systemScalingGovernor = tmp; 
					}
					pclose(consoleFeedback);
				}
				// Check if we already use the performance governor
				if( this->systemScalingGovernor.compare("performance") != 0 )
				{
					if( geteuid() == 0 ) // only root can to this
					{
						if (system("echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"))
							pantheios::log_DEBUG("MAIN| -> Kernel scaling governor set to \"performance\".");
						else
							pantheios::log_DEBUG("MAIN| -> Kernel scaling governor could net set to \"performance\".");
					}
					else
						pantheios::log_DEBUG("MAIN| -> Missing root privileges to set CPU scaling governor. System uses: ", systemScalingGovernor.c_str() );
				}
				else
					pantheios::log_DEBUG("MAIN| -> Kernel uses already the \"performance\" scaling governor.");
			}
			else
				pantheios::log_DEBUG("MAIN| -> Kernel doesn't support the \"performance\" scaling governor.");
		}
		else
			pantheios::log_DEBUG("MAIN| -> Kernel doesn't seem to support the scaling governors.");
	}
	else
		pantheios::log_DEBUG("MAIN| -> Kernel scaling governor will use system default.");

	pantheios::log_DEBUG("MAIN| Leaving setCPU_FrequencyAndGovernor()");
	pantheios::log_DEBUG("MAIN| ");

	return(0);
}


/**
 * Activate the given realtime scheduler for the main process
 *
 * @param schedMethod	1 SCHED_RR, 2 SCHED_FIFO
 *
 * @return 0
 */
int systemOptimization::activateRealtimeScheduling(uint32_t schedMethod)
{
    pantheios::log_DEBUG("MAIN| Entering activateRealtimeScheduling()");

	// Default Scheduler uses Priorities from -20 to 20, Smaller is better
	// Realtime Scheduler uses Priorities from 1 to 99, Bigger is better. Will be displayed in Htop or nice as negative value

	// Kernel processes scheduling which the will CPU use
	// https://www.kernel.org/doc/man-pages/online/pages/man2/sched_getscheduler.2.html

	struct sched_param schedParam;
    int method;

    switch(schedMethod)
    {
        case 1:
            method=SCHED_RR;
            break;
        case 2:
            method=SCHED_FIFO;
            break;
        default:
            method=SCHED_OTHER;
    }

    if (geteuid() == 0) // only root can do this
    {
        schedParam.sched_priority = sched_get_priority_max(SCHED_RR); // Should be 1 to 99

		if( schedParam.sched_priority == -1 || schedParam.sched_priority < 20)
			schedParam.sched_priority = 1;	// failsafe ...
		else
            // This value needs to be equal to the highest priority that any thread gets later one.
            // Otherwise, the main thread will starve on a single processor machine
			schedParam.sched_priority = 4;

		if( sched_setscheduler(0, method, &schedParam) < 0 ){
            //TODO langech
            //append requested method to messages
		    pantheios::log_ERROR("MAIN| -> Setting kernel CPU scheduling policy failed");
			switch(errno){
				case EINVAL:
					pantheios::log_ERROR("The scheduling policy is not one of the recognized policies, param is NULL, or param does not make sense for the policy.");
					break;
				case EPERM:
					pantheios::log_ERROR("The calling thread does not have appropriate privileges.");
					break;
				case ESRCH:
					pantheios::log_ERROR("The thread whose ID is pid could not be found.");
					break;
			}
		}

		else
			pantheios::log_DEBUG("MAIN| -> Setting kernel CPU scheduling policy succeded");
    }
	else
    {
        if( schedMethod > 0 )
		    pantheios::log_ERROR("MAIN| -> Missing root privileges to set process scheduling policy.");
    }

	pantheios::log_DEBUG("MAIN| Leaving activateRealtimeScheduling()");
	pantheios::log_DEBUG("MAIN| ");

	return(0);
}


/**
 * Disable Swapping and pagging for this process
 *
 * @param enableMemoryLocking true or false
 *
 * @return 0
 */
int systemOptimization::activateMemoryLocking(uint32_t enableMemoryLocking)
{
    pantheios::log_DEBUG("MAIN| Entering activateMemoryLocking()");

	if( geteuid() == 0 ) // only root can to this
	{
		// Memory locking for realtime processes, swapping or unloading from memory will be denied
		// https://www.kernel.org/doc/man-pages/online/pages/man2/mlock.2.html

		if( enableMemoryLocking == 1 )
		{
			if( mlockall(MCL_CURRENT|MCL_FUTURE) != 0 )
				pantheios::log_ERROR("MAIN| -> Disabling swapping/unloading memory pages of this process: FAILED.");
			else
				pantheios::log_DEBUG("MAIN| -> Swapping/unloading memory pages of this process: SUCCESSED.");
		}
		else
			pantheios::log_DEBUG("MAIN| -> Swapping/unloading memory pages of this process: DISABLED (kernel default).");
	}
	else
	{
		if( enableMemoryLocking == 1 )
			pantheios::log_ERROR("MAIN| -> Missing root privileges to set memory locking.");
		else
			pantheios::log_DEBUG("MAIN| -> Missing root privileges. Swapping/unloading memory pages of this process: DISABLED.");
	}


	pantheios::log_DEBUG("MAIN| Leaving activateMemoryLocking()");
	pantheios::log_DEBUG("MAIN| ");

	return(0);
}

/**
 * Set the CPU affinity of the given thread
 *
 * @param enabled
 * @param threadNumber
 * @param ThreadID
 * @param ThreadName
 *
 * @return	0
 */
int systemOptimization::setThreadCpuAffinity(uint32_t enabled, uint32_t threadNumber, pthread_t ThreadID, string ThreadName)
{
    pantheios::log_DEBUG("MAIN| Entering setThreadCpuAffinity()");

	if( enabled == 1 )
	{
		cpu_set_t cpuset;
		CPU_ZERO( &cpuset );

		string setTo = "0";

		switch(this->systemCpuCount)
		{
            case 1:
                switch(threadNumber)
				{
					case 0:	CPU_SET(0, &cpuset); break; // mReceivingThread
					case 1: CPU_SET(0, &cpuset); break; // mSendingThread
					case 2: CPU_SET(0, &cpuset); break; // receiverDataDistributer
					case 3: CPU_SET(0, &cpuset); break; // senderDataDistributer
					case 4: CPU_SET(0, &cpuset); break; // sSendingThread
					case 5: CPU_SET(0, &cpuset); break; // sReceivingThread
				}
                break;
			case 2:
                switch(threadNumber)
				{
					case 0:	CPU_SET(1, &cpuset); setTo = "1"; break; // mReceivingThread
					case 1: CPU_SET(1, &cpuset); setTo = "1"; break; // mSendingThread
					case 2: CPU_SET(0, &cpuset); break; // receiverDataDistributer
					case 3: CPU_SET(0, &cpuset); break; // senderDataDistributer
					case 4: CPU_SET(0, &cpuset); break; // sSendingThread
					case 5: CPU_SET(0, &cpuset); break; // sReceivingThread
				}
                break;
			case 3:
                switch(threadNumber)
				{
					case 0:	CPU_SET(1, &cpuset); setTo = "1"; break; // mReceivingThread
					case 1: CPU_SET(2, &cpuset); setTo = "2"; break; // mSendingThread
					case 2: CPU_SET(0, &cpuset); break; // receiverDataDistributer
					case 3: CPU_SET(0, &cpuset); break; // senderDataDistributer
					case 4: CPU_SET(0, &cpuset); break; // sSendingThread
					case 5: CPU_SET(0, &cpuset); break; // sReceivingThread
				}
                break;
			case 4:
                switch(threadNumber)
				{
					case 0:	CPU_SET(1, &cpuset); setTo = "1"; break; // mReceivingThread
					case 1: CPU_SET(2, &cpuset); setTo = "2"; break; // mSendingThread
					case 2: CPU_SET(3, &cpuset); setTo = "3"; break; // receiverDataDistributer
					case 3: CPU_SET(3, &cpuset); setTo = "3"; break; // senderDataDistributer
					case 4: CPU_SET(0, &cpuset); break; // sSendingThread
					case 5: CPU_SET(0, &cpuset); break; // sReceivingThread
				}
                break;
			case 5:
                switch(threadNumber)
				{
					case 0:	CPU_SET(1, &cpuset); setTo = "1"; break; // mReceivingThread
					case 1: CPU_SET(2, &cpuset); setTo = "2"; break; // mSendingThread
					case 2: CPU_SET(3, &cpuset); setTo = "3"; break; // receiverDataDistributer
					case 3: CPU_SET(4, &cpuset); setTo = "4"; break; // senderDataDistributer
					case 4: CPU_SET(0, &cpuset); break; // sSendingThread
					case 5: CPU_SET(0, &cpuset); break; // sReceivingThread
				}
                break;
			case 6:
                switch(threadNumber)
				{
					case 0:	CPU_SET(1, &cpuset); setTo = "1"; break; // mReceivingThread
					case 1: CPU_SET(2, &cpuset); setTo = "2"; break; // mSendingThread
					case 2: CPU_SET(3, &cpuset); setTo = "3"; break; // receiverDataDistributer
					case 3: CPU_SET(4, &cpuset); setTo = "4"; break; // senderDataDistributer
					case 4: CPU_SET(5, &cpuset); setTo = "5"; break; // sSendingThread
					case 5: CPU_SET(5, &cpuset); break; // sReceivingThread
				}
                break;
                // 7 or more processors or processor cores
			default:
                switch(threadNumber)
				{
					case 0:	CPU_SET(1, &cpuset); setTo = "1"; break; // mReceivingThread
					case 1: CPU_SET(2, &cpuset); setTo = "2"; break; // mSendingThread
					case 2: CPU_SET(3, &cpuset); setTo = "3"; break; // receiverDataDistributer
					case 3: CPU_SET(4, &cpuset); setTo = "4"; break; // senderDataDistributer
					case 4: CPU_SET(5, &cpuset); setTo = "5"; break; // sSendingThread
					case 5: CPU_SET(6, &cpuset); setTo = "6"; break; // sReceivingThread
				}
		}
        if( pthread_setaffinity_np(ThreadID, sizeof(cpu_set_t), &cpuset) != 0 )
		    pantheios::log_ERROR("MAIN| -> Setting CPU Affinity to CPU_", setTo.c_str(), " for \"", ThreadName.c_str(),"\" PThread failed.");
		else
		    pantheios::log_DEBUG("MAIN| -> Setting CPU Affinity to CPU_", setTo.c_str(), " for \"", ThreadName.c_str(),"\" PThread succeeded.");
	}
	else
		pantheios::log_DEBUG("MAIN| -> Setting CPU Affinity for \"", ThreadName.c_str(),"\" PThread not enabled.");

	pantheios::log_DEBUG("MAIN| Leaving setThreadCpuAffinity()");
	pantheios::log_DEBUG("MAIN| ");

	return(0);
}

/**
 *	Set the CPU affinity of the main process
 *
 * @param enabled true or false
 *
 * @return	0
 */
int systemOptimization::setMainProcessCpuAffinty(uint32_t enabled)
{
    pantheios::log_DEBUG("MAIN| Entering setMainProcessCpuAffinty()");

	if( enabled == 1 )
	{
		cpu_set_t cpuset;
		CPU_ZERO( &cpuset );
		CPU_SET(0, &cpuset); // Main program on CPU 0

		if( sched_setaffinity(0, sizeof(cpuset), &cpuset) != 0 )
			pantheios::log_ERROR("MAIN| -> Setting CPU Affinity for Process to CPU_0 failed.");
		else
			pantheios::log_DEBUG("MAIN| -> Setting CPU Affinity for Process to CPU_0 succeeded.");
	}
	else
		pantheios::log_DEBUG("MAIN| -> Setting CPU Affinity for Process to CPU_0 not enabled.");

	pantheios::log_DEBUG("MAIN| Leaving setMainProcessCpuAffinty()");
	pantheios::log_DEBUG("MAIN| ");

	return(0);
}

/**
 * Set the priority of the given thread to default value
 *
 * @param threadNumber
 * @param schedMethod	1 SCHED_RR, 2 SCHED_FIFO
 * @param threadID
 * @param ThreadName
 *
 * @return	0
 */
int systemOptimization::activateRealtimeThreadPriority(uint32_t threadNumber, uint32_t schedMethod, pthread_t threadID, string ThreadName)
{pantheios::log_DEBUG("MAIN| Entering activateRealtimeThreadPriority()");

	// Default Scheduler uses Priorities from -20 to 20, Smaller is better
	// Realtime Scheduler uses Priorities from 1 to 99, Bigger is better. Will be displayed in Htop or nice as negative value

	struct sched_param schedParam;

	if( geteuid() == 0 ) // only root can to this
	{
		if( schedMethod == 1 )	// round robin
		{
			int maxUsedPriority = sched_get_priority_max(SCHED_RR); // Should be 1 to 99

			if( maxUsedPriority > 20 )
			{
            // The maximum priority value needs to be equal to the main process priority value.
            // Otherwise, the main thread will starve on a single processor machine
				switch(threadNumber)
				{
					case 0: schedParam.sched_priority = 4; break;	// mReceivingThread 4
					case 1: schedParam.sched_priority = 3; break;	// mSendingThread 4
					case 2: schedParam.sched_priority = 1; break;	// receiverDataDistributerThread 2
					case 3: schedParam.sched_priority = 1; break;	// senderDataDistributer 2
					case 4: schedParam.sched_priority = 1; break;	// sSendingThread 1
					case 5: schedParam.sched_priority = 1; break;	// sReceivingThread 1 
				}
			}
			else
				schedParam.sched_priority = 1;	// failsafe ...

			int rv = pthread_setschedparam(threadID, SCHED_RR, &schedParam);
			if( rv != 0 )
				if ( rv == ESRCH)
					pantheios::log_ERROR("MAIN| -> Setting \"", ThreadName.c_str(),"\" PThread CPU scheduling priority for SCHED_RR: FAILED (no thread with the ID)");
				else if ( rv == EINVAL)
					pantheios::log_ERROR("MAIN| -> Setting \"", ThreadName.c_str(),"\" PThread CPU scheduling priority for SCHED_RR: FAILED (policy not recognized, or param does not make sense)");
				else if ( rv == EPERM)
					pantheios::log_ERROR("MAIN| -> Setting \"", ThreadName.c_str(),"\" PThread CPU scheduling priority for SCHED_RR: FAILED (not enough privileges)");
				else
					pantheios::log_ERROR("MAIN| -> Setting \"", ThreadName.c_str(),"\" PThread CPU scheduling priority for SCHED_RR: FAILED");
			else
				pantheios::log_DEBUG("MAIN| -> Setting \"", ThreadName.c_str(),"\" PThread CPU scheduling priority for SCHED_RR: SUCCESSED");
		}
		else if( schedMethod == 2 )	// fifo
		{
			int maxUsedPriority = sched_get_priority_max(SCHED_FIFO);	// Should be 1 to 99

			if( maxUsedPriority > 20 )
			{
            // The maximum priority value needs to be equal to the main process priority value.
            // Otherwise, the main thread will starve on a single processor machine
				switch(threadNumber)
				{
					case 0: schedParam.sched_priority = 4; break;	// mReceivingThread 4
					case 1: schedParam.sched_priority = 3; break;	// mSendingThread 4
					case 2: schedParam.sched_priority = 1; break;	// receiverDataDistributerThread 2
					case 3: schedParam.sched_priority = 1; break;	// senderDataDistributer 2
					case 4: schedParam.sched_priority = 1; break;	// sSendingThread 1
					case 5: schedParam.sched_priority = 1; break;	// sReceivingThread 1
				}
			}
			else
				schedParam.sched_priority = 1;	// failsafe ...

			int rv = pthread_setschedparam(threadID, SCHED_FIFO, &schedParam);
			if( rv != 0 )
				if ( rv == ESRCH)
					pantheios::log_ERROR("MAIN| -> Setting \"", ThreadName.c_str(),"\" PThread scheduling priority for SCHED_FIFO: FAILED (no thread with the ID)");
				else if ( rv == EINVAL)
					pantheios::log_ERROR("MAIN| -> Setting \"", ThreadName.c_str(),"\" PThread scheduling priority for SCHED_FIFO: FAILED (policy is recognized, or param does not make sense)");
				else if ( rv == EPERM)
					pantheios::log_ERROR("MAIN| -> Setting \"", ThreadName.c_str(),"\" PThread scheduling priority for SCHED_FIFO: FAILED (not enough privileges)");
				else
					pantheios::log_ERROR("MAIN| -> Setting \"", ThreadName.c_str(),"\" PThread scheduling priority for SCHED_FIFO: FAILED");
			else
				pantheios::log_DEBUG("MAIN| -> Setting \"", ThreadName.c_str(),"\" PThread scheduling priority for SCHED_FIFO: SUCCESSED");
		}
		else if( schedMethod == 0 )	// default Scheduler
		{
			switch(threadNumber) // 20 to -20
			{
				case 0: schedParam.sched_priority = -19; break;	// mReceivingThread
				case 1: schedParam.sched_priority = -19; break;	// mSendingThread
				case 2: schedParam.sched_priority = -17; break;	// receiverDataDistributerThread
				case 3: schedParam.sched_priority = -17; break;	// senderDataDistributer
				case 4: schedParam.sched_priority = -16; break;	// sSendingThread
				case 5: schedParam.sched_priority = -16; break;	// sReceivingThread
			}


			int rv = pthread_setschedparam(threadID, SCHED_OTHER, &schedParam);
			if( rv != 0 )
				if ( rv == ESRCH)
					pantheios::log_ERROR("MAIN| -> Setting \"", ThreadName.c_str(),"\" PThread scheduling priority for SCHED_OTHER: FAILED (no thread with the ID)");
				else if ( rv == EINVAL)
					pantheios::log_ERROR("MAIN| -> Setting \"", ThreadName.c_str(),"\" PThread scheduling priority for SCHED_OTHER: FAILED (policy not recognized, or param does not make sense)");
				else if ( rv == EPERM)
					pantheios::log_ERROR("MAIN| -> Setting \"", ThreadName.c_str(),"\" PThread scheduling priority for SCHED_OTHER: FAILED (not enough privileges)");
				else
					pantheios::log_ERROR("MAIN| -> Setting \"", ThreadName.c_str(),"\" PThread scheduling priority for SCHED_OTHER: FAILED");
			else
				pantheios::log_DEBUG("MAIN| -> Setting \"", ThreadName.c_str(),"\" PThread scheduling priority for SCHED_OTHER: SUCCESSED");
		}
	}
	else
	{
		if( schedMethod > 0 )
			pantheios::log_ERROR("MAIN| -> Missing root privileges to set \"", ThreadName.c_str(),"\" PThread priority.");
	}

	pantheios::log_DEBUG("MAIN| Leaving activateRealtimeThreadPriority()");
	pantheios::log_DEBUG("MAIN| ");

	return(0);
}

/**
 * Activate the given scheduling policiy for the given thread
 *
 * @param schedMethod	1 SCHED_RR, 2 SCHED_FIFO
 * @param attribute
 * @param threadName
 *
 * @return 0
 */
int systemOptimization::activateRealtimeThreadScheduling(uint32_t schedMethod, pthread_attr_t *attribute, string threadName)
{pantheios::log_DEBUG("MAIN| Entering activateRealtimeThreadScheduling()");

	// Default Scheduler uses Priorities from -20 to 20, Smaller is better
	// Realtime Scheduler uses Priorities from 1 to 99, Bigger is better. Will be displayed in Htop or nice as negative value

	// Kernel processes scheduling which the will CPU use
	// https://www.kernel.org/doc/man-pages/online/pages/man2/sched_getscheduler.2.html

	if( geteuid() == 0 ) // only root can to this
	{
		if( schedMethod == 1 )	// round robin
		{
			if( pthread_attr_setschedpolicy(attribute, SCHED_RR) != 0 )
				pantheios::log_ERROR("MAIN| -> Setting \"", threadName.c_str(), "\" PThread CPU scheduling policy to SCHED_RR: FAILED");
			else
				pantheios::log_DEBUG("MAIN| -> Setting \"", threadName.c_str(), "\" PThread CPU scheduling policy to SCHED_RR: SUCCESSED");
		}
		else if( schedMethod == 2 )	// fifo
		{
			if( pthread_attr_setschedpolicy(attribute, SCHED_FIFO) != 0 )
				pantheios::log_ERROR("MAIN| -> Setting \"", threadName.c_str(), "\" PThread scheduling policy to SCHED_FIFO: FAILED");
			else
				pantheios::log_DEBUG("MAIN| -> Setting \"", threadName.c_str(), "\" PThread scheduling policy to SCHED_FIFO: SUCCESSED");
		}
		else if( schedMethod == 0 )	// default Scheduler
		{
			// 20 to -20, smaller is better
			//http://www.kernel.org/doc/man-pages/online/pages/man2/setpriority.2.html

			if( pthread_attr_setschedpolicy(attribute, SCHED_OTHER) != 0 )
				pantheios::log_ERROR("MAIN| -> Setting \"", threadName.c_str(), "\" PThread scheduling policy to SCHED_OTHER: FAILED");
			else
				pantheios::log_DEBUG("MAIN| -> Setting \"", threadName.c_str(), "\" PThread scheduling policy to SCHED_OTHER: SUCCESSED");
		}
	}
	else if( schedMethod > 0 )
		pantheios::log_ERROR("MAIN| -> Missing root privileges to set \"", threadName.c_str(), "\" scheduling policy.");


	pantheios::log_DEBUG("MAIN| Leaving activateRealtimeScheduling()");
	pantheios::log_DEBUG("MAIN| ");

	return(0);
}
