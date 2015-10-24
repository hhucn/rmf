/**
 * @authors Tobias Krauthoff <tobias.krauthoff@hhu.de, krauthoff@cs.uni-duesseldorf.de>
 * @date 23. June 2014
 * global position system date
 */

#include "gpsDate.h"


#include "pantheios/pantheiosHeader.h"
#include <iostream>
#include <iomanip>

/**
 * CONSTRUCTOR
 */
GpsDate::GpsDate(){
	this->isValid_			= false;
	this->nLatitude_		= 0.0;
	this->nLongitude_		= 0.0;
	this->nAltitude_		= 0.0;
	this->nSpeed_			= 0.0;
	this->nTrack_			= 0.0;
	this->nTimeError_		= 0.0;
	this->nLongitudeError_	= 0.0;
	this->nLatitudeError_	= 0.0;
	this->nAltitudeError_	= 0.0;
	this->nSpeedError_		= 0.0;
	this->time_				= "";
	this->nMode_			= 0;
}

/**
 * DESTRUCTOR
 */
GpsDate::~GpsDate(){
}

/* SETTER */
void GpsDate::set_valid( bool v )				 { isValid_ 			= v; }
void GpsDate::set_latitude( double lat )		 { nLatitude_ 		= lat; }
void GpsDate::set_longitude( double lng )		 { nLongitude_ 		= lng; }
void GpsDate::set_altitude( double alt )		 { nAltitude_ 		= alt; }
void GpsDate::set_speed( double s )				 { nSpeed_ 			= s; }
void GpsDate::set_track( double t )				 { nTrack_ 			= t; }
void GpsDate::set_timeError( double te )		 { nTimeError_ 		= te; }
void GpsDate::set_longitudeError( double lngerr ){ nLongitudeError_	= lngerr; }
void GpsDate::set_latitudeError( double laterr ) { nLatitudeError_	= laterr; }
void GpsDate::set_altitudeError( double alterr ) { nAltitudeError_	= alterr; }
void GpsDate::set_speedError( double spderr )	 { nSpeedError_		= spderr; }
void GpsDate::set_time( string t )			 	 { time_				= t; }
void GpsDate::set_mode( int m )	 			 	 { nMode_			= m; }



/**
 * Pretty print out on pantheios::log_DEBUG
 */
void GpsDate::printPrettyPantheiosDebug() {
	pantheios::log_DEBUG("GPSDate| ");
	pantheios::log_DEBUG("GPSDate| valid: ",		pantheios::boolean(isValid_));
	pantheios::log_DEBUG("GPSDate| time: ",			time_);
	pantheios::log_DEBUG("GPSDate| mode: ",			pantheios::integer(nMode_));
	pantheios::log_DEBUG("GPSDate| lat: ",			pantheios::real(nLatitude_));
	pantheios::log_DEBUG("GPSDate| lng: ",			pantheios::real(nLongitude_));
	pantheios::log_DEBUG("GPSDate| alt: ",			pantheios::real(nAltitude_));
	pantheios::log_DEBUG("GPSDate| track: ",		pantheios::real(nSpeed_));
	pantheios::log_DEBUG("GPSDate| speed: ", 		pantheios::real(nTimeError_));
	pantheios::log_DEBUG("GPSDate| error lat: ", 	pantheios::real(nLatitudeError_));
	pantheios::log_DEBUG("GPSDate| error lng: ", 	pantheios::real(nLongitudeError_));
	pantheios::log_DEBUG("GPSDate| error alt: ", 	pantheios::real(nAltitudeError_));
	pantheios::log_DEBUG("GPSDate| error speed: ", 	pantheios::real(nSpeedError_));
	pantheios::log_DEBUG("GPSDate|");
}

/**
 * Prints out on pantheios::log_DEBUG
 */
void GpsDate::printPantheiosDebug() {
	pantheios::log_DEBUG("GPSDate| ");
	pantheios::log_DEBUG("GPSDate| valid: ", pantheios::boolean(isValid_)
		, ",  time: ", time_
		, ",  mode: ", pantheios::integer(nMode_)
		, ",  lat: ", pantheios::real(nLatitude_)
		, ",  lng: ", pantheios::real(nLongitude_)
		, ",  alt: ", pantheios::real(nAltitude_)
		, ",  track: ", pantheios::real(nSpeed_)
		, ",  speed: ", pantheios::real(nTimeError_)
		, ",  error lat: ", pantheios::real(nLatitudeError_)
		, ",  error lng: ", pantheios::real(nLongitudeError_)
		, ",  error alt: ", pantheios::real(nAltitudeError_)
		, ",  error speed: ", pantheios::real(nSpeedError_));
	pantheios::log_DEBUG("GPSDate|");
}

/**
 * Prints on standard out
 */
void GpsDate::printStdCout() {
	cout<< std::setprecision(10)
		<< "valid: "		<< isValid_
		<< ",  time: "		<< time_
		<< ",  mode: "		<< nMode_
		<< ",  lat: "		<< nLatitude_
		<< ",  lng: "		<< nLongitude_
		<< ",  alt: "		<< nAltitude_
		<< ",  track: "		<< nTrack_
		<< ",  speed: "		<< nSpeed_
		<< ",  error time: "	<< nTimeError_
		<< ",  error lat: "	<< nLatitudeError_
		<< ",  error lng: "	<< nLongitudeError_
		<< ",  error alt: "	<< nAltitudeError_
		<< ",  error speed:"	<< nSpeedError_
		<< "\n";
}

/**
 * Writes current date into file
 * @param outputFile for writing
 * @return 0
 */
int GpsDate::writeToFile( fstream& outputFile ) {
	// check for empty file
	outputFile.seekp(0,ios::end);
    size_t size = outputFile.tellg();
    if( size == 0 ){
		outputFile
			<< "valid\t"
			<< "time\t"
			<< "mode\t"
			<< "lat\t"
			<< "lng\t"
			<< "alt\t"
			<< "track\t"
			<< "speed\t"
			<< "error time,"
			<< "error lat\t"
			<< "error lng\t"
			<< "error alt\t"
			<< "error speed"
			<< "\n";
	}
	
	outputFile << std::setprecision(10)
		<< isValid_ 		<< "\t"
		<< time_ 			<< "\t"
		<< nMode_ 			<< "\t"
		<< nLatitude_ 		<< "\t"
		<< nLongitude_ 		<< "\t"
		<< nAltitude_ 		<< "\t"
		<< nTrack_ 			<< "\t"
		<< nSpeed_ 			<< "\t"
		<< nTimeError_ 		<< "\t"
		<< nLatitudeError_	<< "\t"
		<< nLongitudeError_ << "\t"
		<< nAltitudeError_	<< "\t"
		<< nSpeedError_ 	<< "\n";
	
	return 0;
}

