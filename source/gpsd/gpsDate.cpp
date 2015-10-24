//=================================================
// Name        : gpsDate.cpp
// Author      : Tobias Krauthoff
// Version     : 1.0
// Description : Global Positioning System Dates
//=================================================

#include "gpsDate.h"

/* CONSTRUCTOR */
gpsDate::gpsDate(){
	this->valid				= false;
	this->latitude			= 0.0;
	this->longitude			= 0.0;
	this->altitude			= 0.0;
	this->speed				= 0.0;
	this->track				= 0.0;
	this->timeError			= 0.0;
	this->longitudeError	= 0.0;
	this->latitudeError		= 0.0;
	this->altitudeError		= 0.0;
	this->speedError		= 0.0;
	this->time				= "";
	this->mode				= 0;
}

/* DESTRUCTOR */
gpsDate::~gpsDate(){
}

/* SETTER */
void gpsDate::set_valid( bool v )				 { valid 			= v; }
void gpsDate::set_latitude( double lat )		 { latitude 		= lat; }
void gpsDate::set_longitude( double lng )		 { longitude 		= lng; }
void gpsDate::set_altitude( double alt )		 { altitude 		= alt; }
void gpsDate::set_speed( double s )				 { speed 			= s; }
void gpsDate::set_track( double t )				 { track 			= t; }
void gpsDate::set_timeError( double te )		 { timeError 		= te; }
void gpsDate::set_longitudeError( double lngerr ){ longitudeError	= lngerr; }
void gpsDate::set_latitudeError( double laterr ) { latitudeError	= laterr; }
void gpsDate::set_altitudeError( double alterr ) { altitudeError	= alterr; }
void gpsDate::set_speedError( double spderr )	 { speedError		= spderr; }
void gpsDate::set_time( string t )			 	 { time				= t; }
void gpsDate::set_mode( int m )	 			 	 { mode				= m; }


void gpsDate::prettyPrint() {
	pantheios::log_DEBUG("GPSDate| ");
	pantheios::log_DEBUG("GPSDate| valid: ",		pantheios::boolean(valid));
	pantheios::log_DEBUG("GPSDate| time: ",			time);
	pantheios::log_DEBUG("GPSDate| mode: ",			pantheios::integer(mode));
	pantheios::log_DEBUG("GPSDate| lat: ",			pantheios::real(latitude));
	pantheios::log_DEBUG("GPSDate| lng: ",			pantheios::real(longitude));
	pantheios::log_DEBUG("GPSDate| alt: ",			pantheios::real(altitude));
	pantheios::log_DEBUG("GPSDate| track: ",		pantheios::real(speed));
	pantheios::log_DEBUG("GPSDate| speed: ", 		pantheios::real(timeError));
	pantheios::log_DEBUG("GPSDate| error lat: ", 	pantheios::real(latitudeError));
	pantheios::log_DEBUG("GPSDate| error lng: ", 	pantheios::real(longitudeError));
	pantheios::log_DEBUG("GPSDate| error alt: ", 	pantheios::real(altitudeError));
	pantheios::log_DEBUG("GPSDate| error speed: ", 	pantheios::real(speedError));
	pantheios::log_DEBUG("GPSDate|");
	//	cout<< std::setprecision(10)
	//		<< "time:\t\t"		<< time
	//		<< "mode:\t\t"		<< mode
	//		<< "lat:\t\t"		<< latitude
	//		<< "lng:\t\t"		<< longitude
	//		<< "alt:\t\t"		<< altitude
	//		<< "track:\t\t"		<< track
	//		<< "speed:\t\t"		<< speed
	//		<< "error time:\t"	<< timeError
	//		<< "error lat:\t"	<< latitudeError
	//		<< "error lng:\t"	<< longitudeError
	//		<< "error alt:\t"	<< altitudeError
	//		<< "error speed:\t"	<< speedError
}

void gpsDate::print() {
	pantheios::log_DEBUG("GPSDate| ");
	pantheios::log_DEBUG("GPSDate| valid: ", pantheios::boolean(valid)
		," time: ", time
		," mode: ", pantheios::integer(mode)
		," lat: ", pantheios::real(latitude)
		," lng: ", pantheios::real(longitude)
		," alt: ", pantheios::real(altitude)
		," track: ", pantheios::real(speed)
		," speed: ", pantheios::real(timeError)
		," error lat: ", pantheios::real(latitudeError)
		," error lng: ", pantheios::real(longitudeError)
		," error alt: ", pantheios::real(altitudeError)
		," error speed: ", pantheios::real(speedError));
	pantheios::log_DEBUG("GPSDate|");
//	cout<< std::setprecision(10)
//		<< "valid: "		<< valid
//		<< ", time: "		<< time
//		<< ", mode: "		<< mode
//		<< ", lat: "		<< latitude
//		<< ", lng: "		<< longitude
//		<< ", alt: "		<< altitude
//		<< ", track: "		<< track
//		<< ", speed: "		<< speed
//		<< ", error time: "	<< timeError
//		<< ", error lat: "	<< latitudeError
//		<< ", error lng: "	<< longitudeError
//		<< ", error alt: "	<< altitudeError
//		<< ", error speed:"	<< speedError << endl;
}

int gpsDate::writeToFile( fstream& outputFile ) {
	// check for empty file
	outputFile.seekp(0,ios::end);
    size_t size = outputFile.tellg();
    if( size == 0 ){
		outputFile << "#"
			<< "valid,"			<< "time,"
			<< "mode,"			<< "lat,"
			<< "lng,"			<< "alt,"
			<< "track,"			<< "speed,"
			<< "error time,"	<< "error lat,"
			<< "error lng,"		<< "error alt,"
			<< "error speed"	<< endl;
	}
	
	outputFile << std::setprecision(10)
		<< valid 			<< ","	<< time 			<< ","
		<< mode 			<< ","	<< latitude 		<< ","
		<< longitude 		<< ","	<< altitude 		<< ","
		<< track 			<< ","	<< speed 			<< ","
		<< timeError 		<< ","	<< latitudeError	<< ","
		<< longitudeError 	<< ","	<< altitudeError	<< ","
		<< speedError 		<< "\n";
	
	return 0;
}

