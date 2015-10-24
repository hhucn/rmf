using namespace std;

#include <iostream>
#include <stdlib.h>
#include <string>
#include <signal.h>

#include "gpsdThreadbase.h"

void handle_Sigint( int );
gpsdThreadbase mGpsd; //gpsdData mGpsd( true, true );

int main (int argc, char *argv[]){
	std::cout << "MAIN| REMEMBER TO RUN WITH ROOT" << endl;
	signal(SIGINT, handle_Sigint);	
	
	bool DEBUG_CQ = false;
	bool write = false;
	string parameter;
	
	for (int i=1; i<argc; i++){	parameter += (string) argv[i] + "|"; }
	
	if ( parameter.find("-w|") != string::npos ){
		write = parameter.substr( 3,1 ).compare("1") == 0 ? true : false;
		parameter.erase( 0,5 );
	}
	if ( parameter.find("-d|") != string::npos ){
		DEBUG_CQ = parameter.substr( 3,1 ).compare("1") == 0 ? true : false ;
		parameter.erase( 0,5 );
	}
	if ( parameter.find("-h") != string::npos ){
		std::cout << "HELP GPSD-WRAPPER\n"
			<< "-w  0|1  write data to file (default 0)\n"
			<< "-d  0|1  debug trace (default 0)\n" << endl;
		return 0;
	}
	
	std::cout << "MAIN| calling gpsd with debug:"
		<< ( DEBUG_CQ ? "true" : "false" )
		<< " and writing:"
		<< ( write ? "true" : "false" ) << endl;
	mGpsd.set_debugFlag( DEBUG_CQ );
	mGpsd.set_writeFileFlag( write );
	
	mGpsd.start();
	mGpsd.join();
	
	return 0;
}

void handle_Sigint(int sig){
	std::cout << "MAIN| SIGINT Signal. Cleaning up and exit (signal: " << sig << ")" << endl;
	
	//mGpsd.set_breakFlag( true );
	//
	
	exit(-1);
}
