/**
 * @authors Tobias Krauthoff <tobias.krauthoff@hhu.de, krauthoff@cs.uni-duesseldorf.de>
 * @date 23. June 2014
 * abstract wrapper for posix threads
 */

#ifndef PTHREADWRAPPER_H
#define PTHREADWRAPPER_H
#include <pthread.h>
#include <stdint.h>

using namespace std;

// Abstract class for wrapping pthread
class PThreadWrapper {
	public:
		PThreadWrapper();						// default constructor
		virtual ~PThreadWrapper();				// default virtual destructor
		virtual void run() = 0;					// thread functionality is pure virtual function  and will be reimplemented in derived classes
		int32_t start();							// start thread
		int32_t join();								// join thread
		bool isRunning();						// check for running thread

	private:
		static void* threadDispatcher(void*);	// callback function passing to pthread create API
		pthread_t *threadId;					// internal pthread ID..
};
#endif
