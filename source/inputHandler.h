/* inputHandler.h */
#ifndef _inputHandler_
#define _inputHandler_

// Namespace
using namespace std;

// External logging framework
#include "pantheios/pantheiosHeader.h"
#include "measurementMethods/measurementMethods.h"
#include <stdint.h>


/**
 * This class handles the parsing of the command line input from the user. It will check and store the input for later use.
 */
class inputHandler
{
	private:	// - Member variables -
				// --------------------
				uint32_t experimentDuration;		/**< Experiment Duration. 									Default 300s 				Range [1s to 4294967295s] */
				uint32_t ipVersion;					/**< IP protocol version.									Default IP4					Range [0 IPv4, 1 IPv6] */
				string   logfilePath;				/**< Path where the logfile will be stored.	 				Default "results/"			Range [any path] */
				string	 inputLogfilePath;			/**< Path which was entered at the commandline				Default "results/"			Range [any path] */
				string   inputFileNameFormat;		/**< Format of the filename which will be generated			Default ""					Range [any filename] */
				uint32_t logLevel;					/**< Logging Level. 										Default normal				Range [0 normal, 1 extended, 2 debug] */
//				uint32_t measurementMethod;			/**< Measurement Method. 									Default basic 				Range [0 basic, 1 assolo, 2 new] */
				measurementMethodClass * measurementMethodClassObj;
				uint32_t measuredLink;				/**< Which link should be measured. 						Default both				Range [0 up, 1 down, 2 both] */
				uint32_t nodeRole;					/**< Server or Client. 						 				Default client 				Range [0 client, 1 server] */
				uint32_t safeguardChannel;			/**< Status of the safeguard channel. 						Default off					Range [0 off, 1 on] */
				uint32_t timingMethod;				/**< Select the method how the time stamp will be acquired. Default auto				Range [0 HPET, 1 TimeOfDay, 2 Custom, >3 auto detect] */
				uint32_t uint32_t_size;				/**< Internal size of integer of the counterpart			Default System dependent 	Range [4 or greater] */

				int maxDeviceQueueSize;		/**< Maximum Size of the Device Queue Size before queueing more Packets	Default -1 (off) */
				int maxDeviceQueueSizePercentage; /**< Maximum Size of the Device Queue Size in percent of the maximum devicequeuesize before queueing more Packets	Default -1 (off) */


				uint32_t maxQueueSize;				/**< Maximal space in a queue								Default 1000 				Range [2 to 4294967295] */
				uint32_t maxWriteAttemptsOnSocket;	/**< Maximal retry count to write on a socket				Default 10 					Range [1 to 4294967295] */
				uint32_t maxWaitForSocketWrite;		/**< Max Seconds of waiting for a socket for writing		Default 10 					Range [0 to 4294967295] */
				uint32_t maxReadAttemptsOnSocket;	/**< Maximal retry count to read on a socket				Default 10 					Range [1 to 4294967295] */
				uint32_t maxWaitForSocketRead;		/**< Max Seconds of waiting for a socket for reading		Default 10 					Range [0 to 4294967295] */
				string   parameterString;			/**< Contains unparsed program parameter					Default userspecific		Range [any input] */
				uint32_t memorySwapping;			/**< Don't allow swappping memory (need root)				Default off					Range [0 off, 1 on] */
				uint32_t processScheduling;			/**< Which scheduling will be used for processes (need root)Default System default		Range [0 default, 1 Realtime FIFO, 2 Realtime Round Robin] */
				uint32_t scalingGovernor;			/**< Enable performance CPU scaling governor (need root) 	Default off					Range [0 system default, 1 performance] */
				uint32_t cpuAffinity;				/**< Enable setting cpu affinity for process/threads	 	Default on					Range [0 off, 1 on] */

				// Client
				string   client_cAddress;			/**< IP/DNS address of client control network interface.		Default 0.0.0.0				Range [IP4,IP6,DNS] */
				string   client_mAddress;			/**< IP/DNS address of client measurement network interface.	Default ""					Range [IP4,IP6,DNS] */
				string   client_sAddress;			/**< IP/DNS address of client safeguard network interface.		Default ""					Range [IP4,IP6,DNS] */
				uint32_t client_cPort;				/**< TCP client control port.									Default 8387. 				Range [1 to 65535] */
				uint32_t client_mPort;				/**< UDP client measurement port.								Default 8389. 				Range [1 to 65535] */
				uint32_t client_sPort;				/**< UDP client safeguard port.									Default 8391. 				Range [1 to 65535] */
				string   client_cInterface;			/**< Interface name for client control channel.					Default IP selected			Range [any local interface] */
				string   client_mInterface;			/**< Interface name for client measurement channel.				Default IP selected			Range [any local interface] */
				string 	 client_sInterface;			/**< Interface name for client safeguard channel.				Default IP selected			Range [any local interface] */
				string   client_sGateway;			/**< Gateway for client safeguard channel interface. */
				// Server
				string   server_cAddress;			/**< IP/DNS address of server control network interface.		Default 0.0.0.0				Range [IP4,IP6,DNS] */
				string   server_mAddress;			/**< IP/DNS address of server measurement network interface.	Default ""					Range [IP4,IP6,DNS] */
				string   server_sAddress;			/**< IP/DNS address of server safeguard network interface.		Default ""					Range [IP4,IP6,DNS] */
				uint32_t server_cPort;				/**< TCP server control port.									Default 8388. 				Range [1 to 65535] */
				uint32_t server_mPort;				/**< UDP server measurement port.								Default 8390. 				Range [1 to 65535] */
				uint32_t server_sPort;				/**< UDP server safeguard port.									Default 8392. 				Range [1 to 65535] */
				string   server_cInterface;			/**< Interface name for server control channel.					Default IP selected			Range [any local interface] */
				string   server_mInterface;			/**< Interface name for server measurement channel.				Default IP selected			Range [any local interface] */
				string   server_sInterface;			/**< Interface name for server safeguard channel.				Default IP selected			Range [any local interface] */

                uint32_t measurement_magic;
                uint32_t safeguard_magic;
				// --------------------


				// - Private functions -
				// ---------------------
				void   printHelp(void);						// Print help information and exit
				void   printVersion(void);					// Print program version and exit
				string resolveInterface(string, uint32_t);	// Resolve IPv4/IPv6 for local interfaces
				// ---------------------



	public:		// - Con-/Destructor -
				// -------------------
				 inputHandler(void);	// Set member variables to default values
				~inputHandler(void);	// ATM not needed
                
                void reset();
				// -------------------


				// - Functions -
				// --------------
				int  	activateLogging(int, char**);				// Get the Folder an Loglevel from parameter before doing anything else
				int	 	createLogfileName();		// (Re)set the filename where pantheios will log to
//				int	 	createLogfileName(uint32_t, uint32_t);		// (Re)set the filename where pantheios will log to
				int  	parseParameter(int, char**);				// Parse the command line input and check if its in correct bounds
				int  	checkSanityOfInput(void);					// Check if all MemberVariables are in correct bounds. If not stop the program.
				string 	resolveAddress(string, uint32_t, uint32_t);	// Resolve IPv4/IPv6/DNS Address for TCP or UDP
				// --------------


				// - Get Methods -
				// ---------------
				uint32_t get_experimentDuration(void)		/** Returns the experimentDuration.		*/	{	return (this->experimentDuration);		};
				uint32_t get_ipVersion(void)				/** Returns the ipVersion.				*/	{	return (this->ipVersion);				};
				string   get_logfilePath(void)				/** Returns the logfilePath.			*/	{	return (this->logfilePath);				};
				uint32_t get_logLevel(void)					/** Returns the logLevel.				*/	{	return (this->logLevel);				};
				//uint32_t get_measurementMethod(void)		/** Returns the measurementMethod.		*/	{	return (this->measurementMethod);		};
				uint32_t get_measuredLink(void)				/** Returns the measuredLink.			*/	{	return (this->measuredLink);			};
				measurementMethodClass* get_measurementMethodClass(void){
					return this->measurementMethodClassObj;
				}
				string get_inputLogfilePath(void) {
					return this->inputLogfilePath;				
				}
				uint32_t get_nodeRole(void)					/** Returns the nodeRole.				*/	{	return (this->nodeRole);				};
                bool amIServer()                            /** Returns true if this is the server  */  {   return (this->nodeRole==1);             };
				uint32_t get_safeguardChannel(void)			/** Returns the safeguardChannel.		*/	{	return (this->safeguardChannel);		};
				uint32_t get_timingMethod(void)				/** Returns the Timing Method.			*/	{	return (this->timingMethod);			};
				uint32_t get_uint32_t_size(void)			/** Returns the uint32_t size 			*/	{	return (this->uint32_t_size);			};

				int get_maxDeviceQueueSize(void)				/** Returns the maxQueueSize 			*/	{	return (this->maxDeviceQueueSize);			};
				int get_maxDeviceQueueSizePercentage(void)				/** Returns the maxQueueSize 			*/	{	return (this->maxDeviceQueueSizePercentage);			};

				uint32_t get_maxQueueSize(void)				/** Returns the maxQueueSize 			*/	{	return (this->maxQueueSize);			};
				uint32_t get_maxReadAttemptsOnSocket(void)	/** Returns the maxReadAttemptsOnSocket */	{	return (this->maxReadAttemptsOnSocket);	};
				uint32_t get_maxWaitForSocketRead(void)		/** Returns the maxWaitForSocketRead 	*/	{	return (this->maxWaitForSocketRead);	};
				uint32_t get_maxWriteAttemptsOnSocket(void)	/** Returns the maxWriteAttemptsOnSocket*/	{	return (this->maxWriteAttemptsOnSocket);};
				uint32_t get_maxWaitForSocketWrite(void)	/** Returns the maxWaitForSocketWrite 	*/	{	return (this->maxWaitForSocketWrite);	};
				string   get_parameterString(void)			/** Returns the parameterString			*/	{	return (this->parameterString);			};

				uint32_t get_memorySwapping(void)			/** Returns the memorySwapping			*/	{	return (this->memorySwapping);			};
				uint32_t get_processScheduling(void)		/** Returns the processScheduling		*/	{	return (this->processScheduling);		};
				uint32_t get_scalingGovernor(void)			/** Returns the scalingGovernor			*/	{	return (this->scalingGovernor);			};
				uint32_t get_cpuAffinity(void)				/** Returns the cpuAffinity				*/	{	return (this->cpuAffinity);				};

				string	 get_inputFileNameFormat(void)		/** Returns the cpuAffinity				*/	{	return (this->inputFileNameFormat);				};

				string   get_client_cAddress(void)			/** Returns the client_cAddress.	*/	{	return (this->client_cAddress);		};
				string   get_client_mAddress(void)			/** Returns the client_mAddress.	*/	{	return (this->client_mAddress);		};
				string   get_client_sAddress(void)			/** Returns the client_sAddress.	*/	{	return (this->client_sAddress);		};
				uint32_t get_client_cPort(void)				/** Returns the client_cPort.		*/	{	return (this->client_cPort);		};
				uint32_t get_client_mPort(void)				/** Returns the client_mPort.		*/	{	return (this->client_mPort);		};
				uint32_t get_client_sPort(void)				/** Returns the client_sPort.		*/	{	return (this->client_sPort);		};
				string   get_client_cInterface(void)		/** Returns the client_cInterface.	*/	{	return (this->client_cInterface);	};
				string   get_client_mInterface(void)		/** Returns the client_mInterface.	*/	{	return (this->client_mInterface);	};
				string   get_client_sInterface(void)		/** Returns the client_sInterface	*/	{	return (this->client_sInterface);	};
				string   get_client_sGateway(void)			/** Returns the client_sGateway		*/	{	return (this->client_sGateway);		};

				string   get_server_cAddress(void)			/** Returns the client_sInterface.	*/	{	return (this->server_cAddress);		};
				string   get_server_mAddress(void)			/** Returns the server_mAddress.	*/	{	return (this->server_mAddress);		};
				string   get_server_sAddress(void)			/** Returns the server_sAddress.	*/	{	return (this->server_sAddress);		};
				uint32_t get_server_cPort(void)				/** Returns the server_sAddress.	*/	{	return (this->server_cPort);		};
				uint32_t get_server_mPort(void)				/** Returns the server_sAddress.	*/	{	return (this->server_mPort);		};
				uint32_t get_server_sPort(void)				/** Returns the server_sPort.		*/	{	return (this->server_sPort);		};
				string   get_server_cInterface(void)		/** Returns the server_cInterface.	*/	{	return (this->server_cInterface);	};
				string   get_server_mInterface(void)		/** Returns the server_mInterface.	*/	{	return (this->server_mInterface);	};
				string   get_server_sInterface(void)		/** Returns the server_sInterface.	*/	{	return (this->server_sInterface);	};

				uint32_t get_measurement_magic(void)		/** Returns the uint32_t size 			*/	{	return (this->measurement_magic);			};
				uint32_t get_safeguard_magic(void)			/** Returns the uint32_t size 			*/	{	return (this->safeguard_magic);			};

				// ---------------


				// - Set Methods -
				// ---------------
				int set_experimentDuration(uint32_t input)			/** */ {	this->experimentDuration = input; 		return(0);	};
				int set_maxDeviceQueueSize(int input)				/** */ {	this->maxDeviceQueueSize = input; 			return(0);			};
				int set_maxDeviceQueueSizePercentage(int input)				/** */ {	this->maxDeviceQueueSizePercentage = input; 			return(0);			};
				int set_maxQueueSize(uint32_t input)				/** */ {	this->maxQueueSize = input; 			return(0);			};
//				int set_measurementMethod(uint32_t input)			/** */ {	if( input >= 0 && input < 4 )	 		{this->measurementMethod = input;} else{return(-1);}; return(0);	};
				int set_measurementMethodClass(measurementMethodClass * input){
	this->measurementMethodClassObj = input;
return (0);
}				int set_safeguardChannel(uint32_t input)			/** */ {	if( input >= 0 && input < 2 )	 		{this->safeguardChannel  = input;} else{return(-1);}; return(0);	};
				int set_measuredLink(uint32_t input)				/** */ {	if( input >= 0 && input < 3 )	 		{this->measuredLink 	 = input;} else{return(-1);}; return(0);	};
				int set_uint32_t_size(uint32_t input)				/** */ {	if( input > 3  && input < 16 )	 		{this->uint32_t_size  	 = input;} else{return(-1);}; return(0);	};
				int set_inputFileNameFormat(string input)			/** */ {	this->inputFileNameFormat = input; 		return(0);	};

				int set_maxWriteAttemptsOnSocket(uint32_t input)	/** */ {	this->maxWriteAttemptsOnSocket = input; return(0);	};
				int set_maxReadAttemptsOnSocket(uint32_t input)		/** */ {	this->maxReadAttemptsOnSocket = input; 	return(0);	};

				int set_client_cAddress(string input)				/** */ {	this->client_cAddress = input; 			return(0);	};
				int set_client_mAddress(string input)				/** */ {	this->client_mAddress = input; 			return(0);	};
				int set_client_sAddress(string input)				/** */ {	this->client_sAddress = input; 			return(0);	};
				int set_client_mPort(uint32_t input)				/** */ {	if( input > 0  && input < 65536 )		{this->client_mPort	= input;} else{return(-1);}; return(0);	};
				int set_client_sPort(uint32_t input)				/** */ {	if( input > 0  && input < 65536 )		{this->client_sPort = input;} else{return(-1);}; return(0);	};

				int set_server_cAddress(string input)				/** */ {	this->server_cAddress = input; 			return(0);	};
				int set_server_mAddress(string input)				/** */ {	this->server_mAddress = input; 			return(0);	};
				int set_server_sAddress(string input)				/** */ {	this->server_sAddress = input; 			return(0);	};
				int set_server_mPort(uint32_t input)				/** */ {	if( input > 0  && input < 65536 )		{this->server_mPort	= input;} else{return(-1);}; return(0);	};
				int set_server_sPort(uint32_t input)				/** */ {	if( input > 0  && input < 65536 )		{this->server_sPort	= input;} else{return(-1);}; return(0);	};

                int set_measurement_magic(uint32_t input)			/** */ {	this->measurement_magic  	= input;    return(0);	};
                int set_safeguard_magic(uint32_t input)				/** */ {	this->safeguard_magic       = input;    return(0);	};

				// ---------------
				measurementMethodClass* create_measurementMethodClassById(int id);
				vector<measurementMethodClass*> get_measurementMethods();
};

#endif /* _inputHandler_ */
