/*
 * routingHandler.cpp
 *
 *  Created on: Jun 12, 2014
 *      Author: shadow-pc
 */

#include "routingHandler.h"
#include <sstream>
#include <fstream>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <unistd.h> //getuid

// External logging framework
#include "pantheios/pantheiosHeader.h"

namespace routingHandler {

routingHandler::routingHandler(std::string interfaceParam, std::string interfaceGateway, std::string destinationAddressParam, int destinationPortParam, std::string protocolParam){
	init = false;
	gateway = interfaceGateway;

	fileRTTables = "/etc/iproute2/rt_tables";
	fileDHCPLeases = "/var/lib/dhcp/dhclient.leases";

	saveFilePrefixRoutingTable = "rmfBackup.RoutingTables";
	saveFilePrefixIPTables = "rmfBackup.IPTables";
	saveFilePrefixRPFilter = "rmfBackup.RPFilter";
	if(!isRootUser()){
		errorMessage = "User is not root";
		error = true;
	}
	struct addrinfo hint, *res = NULL;
	if(!error){
		int ret;

		memset(&hint, '\0', sizeof hint);

		hint.ai_family = PF_UNSPEC;
		hint.ai_flags = AI_NUMERICHOST;

		ret = getaddrinfo(destinationAddressParam.c_str(), NULL, &hint, &res);
		switch(ret){
			case 0:
				break;
			case EAI_AGAIN:
			case EAI_BADFLAGS:
			case EAI_FAIL:
			case EAI_FAMILY:
			case EAI_MEMORY:
			case EAI_NONAME:
			case EAI_SERVICE:
			case EAI_SOCKTYPE:
			case EAI_SYSTEM:
				errorMessage = gai_strerror(ret);
				error = true;
				break;
			default:
				errorMessage = "Unknown getaddrinfo Error";
				error = true;
				break;
		}
	}

	if(!error){
		if(res == NULL){
			errorMessage = "addr info not initialized";
			error = true;
		}
		else if(res->ai_family == AF_INET) {
			//IPv4
			ipv6 = false;
		} else if (res->ai_family == AF_INET6) {
			//IPv6
			ipv6 = true;
		} else {
			errorMessage = "ai_family is not IPv4 or IPv6";
			error = true;
		}
	}else{
		debugMessage(errorMessage);
	}

	if(!error){
		init = true;
		interface = interfaceParam;
		destinationAddress = destinationAddressParam;
		destinationPort = destinationPortParam;
		protocol = protocolParam;
	}else{
		debugMessage(errorMessage);
	}

	int bVRet = backupValues();
	if(bVRet != 0){
		switch(bVRet) {
		case RH_BACKUPVALUES_RT_NOREAD:
		case RH_BACKUPVALUES_RT_NOWRITE:
		case RH_BACKUPVALUES_IP_NOSAVE:
		case RH_BACKUPVALUES_RPF_NOSAVE:
			error = true;
			errorMessage = bVErrorMessage(bVRet);
			break;
		default:
			error = false || error;
			break;
		}
	}

	if(!error){
		initValues();
	}else{
		debugMessage(errorMessage);
	}
	freeaddrinfo(res);
}

routingHandler::~routingHandler(){
	if(isRootUser() && init)
	{
		restoreValues();
	}
}
int routingHandler::initValues() {

	if(strlen(gateway.c_str())>0){
		debugMessage("Initialize the Default Gateway");
		int initDefaultGatewayReturnValue = initDefaultGateway();
		if(initDefaultGatewayReturnValue == 0){
			debugMessage("OK: "+ gateway);
		}else{
			debugMessage("ERROR");
		}
	}



	debugMessage("Initialize a Marker Id");
	int initMarkerIdReturnValue = initMarkerId();
	if(initMarkerIdReturnValue == 0){
		debugMessage("OK: "+ intToString(markerId));
	}else{
		debugMessage("ERROR");
	}

	debugMessage("Initialize RoutingTable Informations");
	int initRoutingTableInfosReturnValue = initRoutingTableInfos();
	if(initRoutingTableInfosReturnValue == 0){
		debugMessage("OK: table-name: "+tableName + " table-number: "+ intToString(tableNumber));
	}else{
		debugMessage("ERROR");
	}

	debugMessage("Initialize the Interface Source Address");
	int initSourceAddressReturnValue = initSourceAddress();
	if(initSourceAddressReturnValue == 0){
		debugMessage("OK: "+ sourceAddress);
	}else{
		debugMessage("ERROR");
	}

	debugMessage("Create Routing Table Entry");
	int createRoutingTableReturnValue = createRTTablesEntry();
	if(createRoutingTableReturnValue == 0){
		debugMessage("OK");
	}else{
		debugMessage("ERROR");
	}

	debugMessage("Create Route Table");
	int createRouteTableReturnValue = createRouteTable();
	if(createRouteTableReturnValue == 0){
		debugMessage("OK");
	}else{
		debugMessage("ERROR");
	}

	debugMessage("Create Ip Table Entries");
	int createIpTableReturnValue = createIpTablesEntries();
	if(createIpTableReturnValue == 0){
		debugMessage("OK");
	}else{
		debugMessage("ERROR");
	}

	debugMessage("Deactivate the RP Filter");
	int setRpFilterReturnValue = setRpFilter();
	if(setRpFilterReturnValue == 0){
		debugMessage("OK");
	}else{
		debugMessage("ERROR");
	}
	return 0;
}

int routingHandler::backupValues(){
	debugMessage("Saving RoutingTables");
	int saveRTTReturnValue = saveRTTTables();
	switch(saveRTTReturnValue){
	case -1: debugMessage("Could not read "+fileRTTables); return RH_BACKUPVALUES_RT_NOREAD;
	case -2: debugMessage("Could not write BackupFile"); return RH_BACKUPVALUES_RT_NOWRITE;
	default: debugMessage("OK"); break;
	}

	debugMessage("Saving IP Tables");
	int saveIPTablesReturnValue = saveIPTables();
	if(saveIPTablesReturnValue != 0){
		return RH_BACKUPVALUES_IP_NOSAVE;
	}

	debugMessage("Saving RP Filter");
	int saveRPFilterReturnValue  = backupRpFilter();
	if(saveRPFilterReturnValue != 0){
		return RH_BACKUPVALUES_RPF_NOSAVE;
	}

	return 0;
}

std::string routingHandler::bVErrorMessage(int code){

	switch(code){
	case RH_BACKUPVALUES_RT_NOREAD:
	return std::string("Could not read the rt_tables file.");
	case RH_BACKUPVALUES_RT_NOWRITE:
	return std::string("Could not write the rt_tables file");
	case RH_BACKUPVALUES_IP_NOSAVE:
	return std::string("Could not backup ip-tables");
	case RH_BACKUPVALUES_RPF_NOSAVE:
	return std::string("Could not save the rp filter flag.");
	default:
		return std::string("Unknown error.");
	}
}

void routingHandler::restoreValues(){



	debugMessage("Removing the created RoutingTable");


	int removeRouteReturnValue = removeRouteTable();


	if(removeRouteReturnValue == 0){
		debugMessage("OK");
	}else{
		debugMessage("ERROR");
	}



	debugMessage("Restoring Routing Table");


	int restoreRTTReturnValue = restoreRTTTable();



	switch(restoreRTTReturnValue){
	case -1: debugMessage("Could not read BackupFile"); break;
	case -2: debugMessage("Could not write "+fileRTTables); break;
	default: debugMessage("OK"); break;
	}



	debugMessage("Restoring IP Tables");


	int restoreIPTablesReturnValue = restoreIPTables();



	if(restoreIPTablesReturnValue == 0){
		debugMessage("OK");
	}else{
		debugMessage("ERROR");
	}



	debugMessage("Restoring RP Filter");

	int restoreRPFilterReturnValue = restoreRpFilter();



	if(restoreRPFilterReturnValue == 0){
		debugMessage("OK");
	}else{
		debugMessage("ERROR");
	}


}

int routingHandler::createRTTablesEntry() {
	std::string addTablecommand("echo ");
	addTablecommand.append(getRTTTableString());
	addTablecommand.append(" >> \"");
	addTablecommand.append(fileRTTables);
	addTablecommand.append("\"");
	int returnValue = systemCall(addTablecommand.c_str());


	if(returnValue != 0){
		debugMessage("Firing "+addTablecommand);
	}

	return returnValue;
}

std::string routingHandler::getRTTTableString() {
	std::stringstream ss;
	ss << tableNumber << "	" << tableName;
	return ss.str();
}
int routingHandler::saveRTTTables() {
	return copyFile(fileRTTables, createBackupFilename(saveFilePrefixRoutingTable, interface));
}
int routingHandler::restoreRTTTable() {
	std::string filename =createBackupFilename(saveFilePrefixRoutingTable, interface);
	int retValue = copyFile(filename, fileRTTables);
	return retValue;
}

/**
 * Parses the Routing Table File and finds a free TableId and TableName, that we can use
 * Return on success: 0
 * Return on failure:
 * -1: Couldn't open the Routing table File
 */
int routingHandler::initRoutingTableInfos() {
	std::ifstream fin;
	fin.open(fileRTTables.c_str());
	if (!fin.good()) {
		return -1;
	}
	int lineBufferSize = 512;
	int ids[256];
	std::string names[256];
	int i = 0;
	while (!fin.eof()) {
		char buf[lineBufferSize];
		fin.getline(buf, lineBufferSize);

		char* idChars;
		idChars = strtok(buf, " \t=");
		if (idChars == NULL || idChars[0] == '#') {
			// no id found or
			continue;
		}
		int id = atoi(idChars);
		if (id < 0 || id > 255) {
			// not in the right range
			continue;
		}

		char* name;
		name = strtok(NULL, " \t=");
		if (name == NULL) {
			// no name found
			continue;
		}
		ids[i] = id;
		names[i] = std::string(name);
		i++;
	}
	int j;
	int newId;
	for (newId = 0; newId < 256; newId++) {
		bool found = false;
		for (j = 0; j < i; j++) {
			if (ids[j] == newId) {
				found = true;
			}
		}
		if (!found) {
			break;
		}
	}
	tableNumber = newId;

	char name[] = "rmfRoutingTable";
	int counter;
	std::string newName;
	for (counter = 0; counter < 256; counter++) {
		std::stringstream ss;
		ss << name << counter;
		newName = ss.str();

		bool found = false;
		for (j = 0; j < i; j++) {
			if (names[j].compare(newName) == 0) {
				found = true;
			}
		}
		if (!found) {
			break;
		}
	}
	tableName = newName;
	fin.close();
	return 0;
}

/**
 * Flushes the created Routing table, so that there are no traces left
 * Return on success: 0
 * Return on failure: >0
 */
int routingHandler::removeRouteTable() {
	std::string removeDefaultRouting("ip route flush table ");
	removeDefaultRouting.append(tableName);
	return systemCall(removeDefaultRouting.c_str());
}

/**
 * Adds a Routing table with the default route to the default Gateway of the default interface
 * And routes all marked Packets through this Routing Table
 * Return on success: 0
 * Return on failure: >0
 */
int routingHandler::createRouteTable() {

	std::string addDefaultRouting("ip route add default via ");
	addDefaultRouting.append(gateway);
	addDefaultRouting.append(" dev ");
	//std::string addDefaultRouting("ip route add default dev ");
	addDefaultRouting.append(interface);
	addDefaultRouting.append(" table ");
	addDefaultRouting.append(tableName);
	int addDefaultRoutingReturnValue = systemCall(addDefaultRouting.c_str());


	if(addDefaultRoutingReturnValue != 0){

		debugMessage("Firing: "+addDefaultRouting);
		debugMessage("ReturnValue: "+intToString(addDefaultRoutingReturnValue));

		return addDefaultRoutingReturnValue;
	}

	std::string addMarkedRoute("ip rule add fwmark 0x");
	addMarkedRoute.append(intToString(markerId));
	addMarkedRoute.append(" table ");
	addMarkedRoute.append(tableName);
	int addMarkedRouteReturnValue = systemCall(addMarkedRoute.c_str());


	if(addMarkedRouteReturnValue != 0){
		debugMessage("Firing: "+addMarkedRoute);
		debugMessage("ReturnValue: "+intToString(addMarkedRouteReturnValue));
	}


	return addMarkedRouteReturnValue;
}

/**
 * Saves the Ip Tables to a backup file
 * Return on success: 0
 * Return on failure: >0
 */
int routingHandler::saveIPTables() {
	std::string command = "iptables-save > \"";
	command.append(createBackupFilename(saveFilePrefixIPTables, interface));
	command.append("\"");
	return systemCall(command.c_str());
}
/**
 * Restores the IP Tables from the backup file
 * Return on success: 0
 * Return on failure: >0
 */
int routingHandler::restoreIPTables() {
	std::string command = "iptables-restore < \"";
	command.append(createBackupFilename(saveFilePrefixIPTables, interface));
	command.append("\"");
	return systemCall(command.c_str());
}

/**
 * Creates two IP Table Entries.
 * First to mark all Packets, which are going out to the destination address (We'll need this to specify the right routing table with ip route)
 * Second to change the Source Address to the right outgoing interface, so that the return messages will arrive on this interface.
 * Return on success: 0
 * Return on failure: >0
 */
int routingHandler::createIpTablesEntries() {
	std::string markIpTables("");
	if(ipv6){
		markIpTables.append("ip6tables");
	}else{
		markIpTables.append("iptables");
	}
	markIpTables.append(" -A OUTPUT -t mangle -p ");
	markIpTables.append(protocol);
	markIpTables.append(" --dport ");
	markIpTables.append(intToString(destinationPort));
	markIpTables.append(" --destination ");
	markIpTables.append(destinationAddress);
	markIpTables.append(" -j MARK --set-mark ");
	markIpTables.append(intToString(markerId));
	int markIpTablesReturnValue = systemCall(markIpTables.c_str());

	if(markIpTablesReturnValue != 0){

		debugMessage("Firing: "+markIpTables);
		debugMessage("ReturnValue: "+intToString(markIpTablesReturnValue));

		return markIpTablesReturnValue;
	}

	std::string changeSourceIpTables("");

	if(ipv6){
		changeSourceIpTables.append("ip6tables");
		changeSourceIpTables.append(" -A POSTROUTING -t mangle -p ");
		changeSourceIpTables.append(protocol);
		changeSourceIpTables.append(" --dport ");
		changeSourceIpTables.append(intToString(destinationPort));
		changeSourceIpTables.append(" --destination ");
		changeSourceIpTables.append(destinationAddress);
		changeSourceIpTables.append(" --source ");
		changeSourceIpTables.append(sourceAddress);
	}else{
		changeSourceIpTables.append("iptables");
		changeSourceIpTables.append(" -A POSTROUTING -t nat -p ");
		changeSourceIpTables.append(protocol);
		changeSourceIpTables.append(" --dport ");
		changeSourceIpTables.append(intToString(destinationPort));
		changeSourceIpTables.append(" --destination ");
		changeSourceIpTables.append(destinationAddress);
		changeSourceIpTables.append(" -j SNAT --to ");
		changeSourceIpTables.append(sourceAddress);
	}

	int changeSourceIpTablesReturnValue = systemCall(changeSourceIpTables.c_str());


	if(changeSourceIpTablesReturnValue != 0){
		debugMessage("Firing: "+changeSourceIpTables);
		debugMessage("ReturnValue: "+intToString(changeSourceIpTablesReturnValue));
	}


	return changeSourceIpTablesReturnValue;


}

/**
 *	Create a backup file with the rp filter value of the default interface
 *	Return on success: 0
 *	Return on failure: >0
 */
int routingHandler::backupRpFilter() {
	int value = getRpFilterValue(interface);
	std::string command("sudo echo ");
	command.append(intToString(value));
	command.append(" > ");
	command.append(createBackupFilename(saveFilePrefixRPFilter, interface));
	return systemCall(command.c_str());
}

/**
 *	Returns the RP Filter Value for an interface
 *	Return on success: RP Filter Value
 *	Return on failure:
 *	-1 : Could not open the Rp Filter Option
 */
int routingHandler::getRpFilterValue(std::string device){

	std::string configName("net.ipv4.conf.");
	configName.append(device);
	configName.append(".rp_filter");

	std::string saveRpFilter("sysctl ");
	saveRpFilter.append(configName);


	FILE* pipe = popen(saveRpFilter.c_str(), "r");
	if(pipe == NULL){
		return -1;
	}
	int bufSize = 512;
	char buffer[bufSize];
	std::string valueString;
	while(!feof(pipe)){
		if(fgets(buffer, bufSize, pipe) != NULL)
			valueString.append(buffer);
	}

	int value = StringToInt(valueString.substr(configName.length()+3));

	pclose(pipe);
	return value;
}


/**
 * Sets the Rp-Filter-Value for the default interface to 0; Deactivates it
 * Return on success: 0
 * Return on failure: >0
 */
int routingHandler::setRpFilter() {
	return setRpFilterTo(0);
}

/**
 * Sets the Rp-Filter-Value for the default interface to a specific value
 * Return on success: 0
 * Return on failure: >0
 */
int routingHandler::setRpFilterTo( int value) {
	return setRpFilterTo(value, interface);
}

/**
 * Sets the Rp-Filter-Value for an interface to a specific value
 * Return on success: 0
 * Return on failure: >0
 */
int routingHandler::setRpFilterTo( int value, std::string deviceInterface) {
	std::string deactivateRpFilter("sysctl net.ipv4.conf.");
	deactivateRpFilter.append(deviceInterface);
	deactivateRpFilter.append(".rp_filter=");
	deactivateRpFilter.append(intToString(value));
	return systemCall(deactivateRpFilter.c_str());
}


/**
 *	Restores the Rp-Filter-Value for the default interface out of the the backup file
 *	Return on success: 0
 *	Return on failure:
 *	-1 : Could not open the backup file
 *	>0 : Error
 */
int routingHandler::getRpFilterValueFromBackup(){
	return getRpFilterValueFromBackup(interface);
}

/**
 *	Restores the Rp-Filter-Value for an interface out of the the backup file
 *	Return on success: 0
 *	Return on failure:
 *	-1 : Could not open the backup file
 *	>0 : Error
 */
int routingHandler::getRpFilterValueFromBackup(std::string interfacename){
	std::ifstream fin;
	fin.open(createBackupFilename(saveFilePrefixRPFilter, interfacename).c_str());
	if (!fin.good()) {
		return -1;
	}
	int lineBufferSize = 512;
	std::string restoreRPFilter("");
	if (!fin.eof()) {
		char buf[lineBufferSize];
		fin.getline(buf, lineBufferSize);
		restoreRPFilter.append(buf);
	}
	fin.close();
	return StringToInt(restoreRPFilter);
}

/**
 *	Restores the Rp-Filter-Value out of the the backup file
 *	Return on success: 0
 *	Return on failure:
 *	-1 : Could not open the backup file
 *	>0 : Error
 */
int routingHandler::restoreRpFilter() {
	return restoreRpFilter(interface);
}


/**
 *	Restores the Rp-Filter-Value for an interface out of the the backup file
 *	Return on success: 0
 *	Return on failure: > 0
 */
int routingHandler::restoreRpFilter(std::string deviceInterface) {
	int restoreValue = getRpFilterValueFromBackup(deviceInterface);
	return setRpFilterTo(restoreValue, deviceInterface);
}



/**
 * Parses the the source adress of the default interface
 * Return on success: 0
 * Return on failure: 1
 */
int routingHandler::initSourceAddress() {
	std::string returnValue = parseSourceAddress(interface, ipv6);
	if(returnValue.empty()){
		return 1;
	}
	sourceAddress = returnValue;
	return 0;
}

/**
 * Parses the ipv4 or ipv6 source adress of a interface
 * Return on success: source address
 * Return on failure: empty string
 */
std::string routingHandler::parseSourceAddress(std::string device, bool isipv6) {
	struct ifaddrs *ifap, *ifa;
	struct sockaddr_in *sa;

	getifaddrs (&ifap);
	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family==AF_INET && isipv6 == false) {

			if(memcmp(ifa->ifa_name, device.c_str(), strlen(ifa->ifa_name)) == 0){
				sa = (struct sockaddr_in *) ifa->ifa_addr;
				freeifaddrs(ifap);
				return std::string(inet_ntoa(sa->sin_addr));
			}
		}else if(ifa->ifa_addr->sa_family==AF_INET6 && isipv6 == true){

			if(memcmp(ifa->ifa_name, device.c_str(), strlen(ifa->ifa_name)) == 0){
				sa = (struct sockaddr_in *) ifa->ifa_addr;
				char str[INET6_ADDRSTRLEN];
				inet_ntop(AF_INET6, &(sa->sin_addr), str, INET6_ADDRSTRLEN);
				freeifaddrs(ifap);
				return std::string(str);
			}

		}
	}
	freeifaddrs(ifap);
	return std::string("");
}

/**
 * Sets the Default Gateway, for the interface
 * Return Value
 * 0: ok, found an ip and the gateway is set
 * 1: couldn't find an ip
 * 2: couldn't open the file
 */
int routingHandler::initDefaultGateway() {
	std::ifstream fin;
	fin.open(fileDHCPLeases.c_str());
	if (!fin.good()) {
		return 2;
	}
	int lineBufferSize = 512;
	char optionString[] = "option";
	char routerString[] = "routers";
	char interfaceString[] = "interface";
	char startBlockString[] = "lease {";
	char endBlockString[] = "}";
	bool currentlyInBlock = false;
	bool isRightInterfaceBlock = false;
	while (!fin.eof()) {
		char buf[lineBufferSize];
		fin.getline(buf, lineBufferSize);

		if(!currentlyInBlock){
			if(memcmp(buf, startBlockString, strlen(startBlockString)) == 0){
				currentlyInBlock = true;
				isRightInterfaceBlock = false;
			}
		}else{
			if(memcmp(buf, endBlockString, strlen(endBlockString)) == 0){
				currentlyInBlock = false;
				isRightInterfaceBlock = false;
			}else{
				if(!isRightInterfaceBlock){
					char* firstString;
					firstString = strtok(buf, " ");
					if (firstString != NULL && memcmp(firstString, interfaceString, strlen(interfaceString)) == 0){
						// it is an interface line
						char* interfaceNameString;
						interfaceNameString = strtok(NULL, " \"");

						if(memcmp(interfaceNameString, interface.c_str(), strlen(interfaceNameString)) == 0){
							isRightInterfaceBlock = true;
						}
					}
				}else{


					char* option;
					option = strtok(buf, " ");
					if (option == NULL || memcmp(option, optionString, strlen(optionString)) != 0){
						//std::cout << "not option line" << std::endl;
						continue;
					}

					char* router;
					router = strtok(NULL, " ");
					if (router == NULL || memcmp(router, routerString, strlen(routerString)) != 0){
						//std::cout << "not router line" << std::endl;
						continue;
					}

					char* ip;
					ip = strtok(NULL, " ;");
					if (ip == NULL){
						//std::cout << "not delimiter found" << std::endl;
						continue;
					}else{
						//found valid gateway ip
						gateway = std::string(ip);
						//std::cout << gateway << std::endl;
						return 0;
					};
				}

			}

		}
	}
	return 1;
}
int routingHandler::initMarkerId() {
	//TODO: perhaps dynamic?
	markerId = 1;
	return 0;
}

std::string routingHandler::intToString(int i) {
	std::stringstream ss;
	ss << i;
	return ss.str();
}

int routingHandler::StringToInt(std::string str) {
	std::stringstream ss(str);
	int i;
	ss >> i;
	return i;
}

int routingHandler::copyFile(std::string sourceString, std::string destString) {

	FILE* source = fopen(sourceString.c_str(), "rb");
	if(source == NULL){
		return -1; // could not open source file
	}
	FILE* dest = fopen(destString.c_str(), "wb");
	if(dest == NULL){
		fclose(source);
		return -2; // could not open destination file
	}
	int bufferSize = 512;
	char buf[bufferSize];
	size_t size;
	while ((size = fread(buf, 1, bufferSize, source))) {
		fwrite(buf, 1, size, dest);
	}
	fclose(source);
	fclose(dest);
	return 0;
}

std::string routingHandler::createBackupFilename(std::string prefix, std::string interfaceName){
	std::string newName(prefix);
	newName.append(".");
	newName.append(interfaceName);
	newName.append(".bak");
	return newName;
}

int routingHandler::debugMessage(std::string message){
	pantheios::log_DEBUG("ROHA| ", message);
	return 0;
}

int routingHandler::systemCall(std::string call){
	debugMessage(call);
	return system(call.c_str());
}

bool routingHandler::isRootUser(){
	return (geteuid() == 0);
}
} /* namespace routingHandler */



