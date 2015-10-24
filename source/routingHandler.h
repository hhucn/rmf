/*
 * routingHandler.h
 *
 *  Created on: Jun 12, 2014
 *      Author: shadow-pc
 */

#ifndef ROUTINGHANDLER_H_
#define ROUTINGHANDLER_H_

#include <string>

namespace routingHandler {

class routingHandler {
public:
	routingHandler(std::string interfaceParam, std::string interfaceGateway, std::string destinationAddressParam, int destinationPortParam, std::string protocolParam);
	~routingHandler();

private:
	std::string fileRTTables;
	std::string saveFilePrefixRoutingTable;
	std::string saveFilePrefixIPTables;
	std::string fileDHCPLeases;
	std::string saveFilePrefixRPFilter;

	std::string sourceAddress;
	std::string destinationAddress;
	int destinationPort;
	bool ipv6;
	bool init;

	bool error;
	std::string errorMessage;

	std::string interface;
	std::string protocol;
	std::string gateway;

	std::string tableName;
	int tableNumber;
	int markerId;
	bool wasRpFilterActive;

	std::string ipTablesSaveFile;

	int initValues();

	/**
	 * Error return codes from backupValues();
	 */
	#define RH_BACKUPVALUES_RT_NOREAD 1
	#define RH_BACKUPVALUES_RT_NOWRITE 2
	#define RH_BACKUPVALUES_IP_NOSAVE 3
	#define RH_BACKUPVALUES_RPF_NOSAVE 4
	int backupValues();
	std::string bVErrorMessage(int code);

	void restoreValues();


	int initRoutingTableInfos(); // looks for an unused routingtable Id and Name
	int createRTTablesEntry(); // creates an entry in the routing table file with tableNumber and tableName
	int saveRTTTables(); // backups the rt_table file
	int restoreRTTTable(); //restores the rt_table file from backup
	std::string getRTTTableString(); // returns the rt_table line for the new routing table


	int initMarkerId();

	int initDefaultGateway(); // sets the default gateway of the current interface

	int createRouteTable(); // creates the default route in the new routing table
	int removeRouteTable(); // creates the default route in the new routing table

	void addIpRule();
	void removeIpRole();

	// IP Tables save & restore
	int saveIPTables();
	int restoreIPTables();
	int createIpTablesEntries();

	int initSourceAddress();
	std::string parseSourceAddress(std::string device, bool ipv6);

	int backupRpFilter();
	int getRpFilterValue(std::string interface); // returns the current rp filter value for an interface
	int setRpFilter(); // sets the rp filter to 0 for the current interface
	int setRpFilterTo(int value); // sets the rp filter to value for the current interface
	int setRpFilterTo(int value, std::string interface); // sets the rp filter to value for an interface
	int getRpFilterValueFromBackup(); // returns the current rp filter value for the current interface from the current backup file
	int getRpFilterValueFromBackup(std::string interface); // returns the  rp filter value for an interface from the current backup file
	int restoreRpFilter();
	int restoreRpFilter(std::string deviceInterface);

	//helper
	std::string createBackupFilename(std::string prefix, std::string interfaceName);
	int copyFile(std::string source, std::string dest); // just a helper function to copy 2 files
	std::string intToString(int i);
	int StringToInt(std::string str);
	bool isRootUser();

	int debugMessage(std::string message);
	int systemCall(std::string call);


};

} /* namespace routingHandler */
 /* ROUTINGHANDLER_H_ */

#endif
