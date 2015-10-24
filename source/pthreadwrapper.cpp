/**
 * @authors Tobias Krauthoff <tobias.krauthoff@hhu.de, krauthoff@cs.uni-duesseldorf.de>
 * @date 23. June 2014
 * abstract wrapper for posix threads
 */

#include "pthreadwrapper.h"
#include "pantheios/pantheiosHeader.h"

/**
 * default constructor
 */
PThreadWrapper::PThreadWrapper() {
	threadId = NULL;
}

/**
 * default destructor
 */
PThreadWrapper::~PThreadWrapper() {

}

/**
 * Will create a pthread, when recent thread is null
 * @return 0 when pthread could be created, -1 otherwise
 */
int32_t PThreadWrapper::start() {
	if (threadId == NULL) {
		threadId = new pthread_t;
		long unsigned int address = reinterpret_cast<long unsigned int>(threadId);
		pantheios::log_DEBUG("PTHW| Thread started with id: ", pantheios::integer(address));
		int32_t rv = pthread_create(threadId, NULL, threadDispatcher, this);
		if (rv != 0) {
			pantheios::log_ERROR("PTHW| pthread_create failed!");
			return -1;
		}
	} else {
		pantheios::log_ERROR("PTHW| Multiple thread start!");
		return -1;
	}
	return 0;
}

/**
 * Will join a pthread, when recent thread is not null
 * @return 0 when pthread could be joined, -1 otherwise
 */
int32_t PThreadWrapper::join() {
	if (threadId) {
		int32_t rv = pthread_join(*threadId, NULL);
		if (rv != 0) {
			pantheios::log_ERROR("PTHW| pthread_join failed");
			return -1;
		}
        delete threadId;
        threadId = NULL;
	} else {
		pantheios::log_ERROR("PTHW| Multiple thread join!");
		return -1;
	}
	return 0;
}

/**
 * Boolean for running thread
 * @return true, when current pthread is not null
 */
bool PThreadWrapper::isRunning(){
	return (threadId != NULL);
};

/**
 * The wrapper itself :)
 * @param pTr abstract class
 */
void* PThreadWrapper::threadDispatcher(void* pTr) {
	PThreadWrapper* pThis = static_cast<PThreadWrapper*>(pTr);
	pThis->run();
	pthread_exit(0);
}
