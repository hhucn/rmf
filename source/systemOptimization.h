/* systemOptimization.h */
#ifndef SYSTEMOPTIMIZATION_H_
#define SYSTEMOPTIMIZATION_H_

// Namespace
using namespace std;

#include <stdint.h>
#include <string>


/**
 * This class handles the access and modification of the oss
 */
class systemOptimization
{
	private:	// - Member variables -
				// --------------------
				uint32_t systemCpuCount;		/**< Physical CPU Count*/
				long int systemMemory;			/**< RAM of this Machine*/
				string   systemScalingGovernor; /**< Reset Scaling Govenor to this value, if it was changed*/
				// --------------------

	public:		// - Con-/Destructor -
				// -------------------
				systemOptimization();
				~systemOptimization();
				// -------------------

				// - Functions -
				// --------------
				int reset_ScalingGovenor();
				int setCPU_FrequencyAndGovernor(uint32_t);

				int activateMemoryLocking(uint32_t);

				int setMainProcessCpuAffinty(uint32_t);
				int setThreadCpuAffinity(uint32_t, uint32_t, pthread_t, string);
				int activateRealtimeThreadPriority(uint32_t, uint32_t, pthread_t, string);

				int activateRealtimeScheduling(uint32_t);
				int activateRealtimeThreadScheduling(uint32_t, pthread_attr_t*, string);
				// --------------
};

#endif /* SYSTEMOPTIMIZATION_H_ */
