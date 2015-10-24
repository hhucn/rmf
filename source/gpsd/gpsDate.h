//=================================================
// Name        : gpsDate.h
// Author      : Tobias Krauthoff
// Version     : 1.0
// Description : Global Positioning System Dates
//=================================================

#ifndef gpsDate_H_
#define gpsDate_H_

using namespace std;

#include "../pantheios/pantheiosHeader.h"

#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>


class gpsDate {
	private:
		bool valid;
		double latitude;
		double longitude;
		double altitude;
		double speed;
		double track;
		double timeError;
		double longitudeError;
		double latitudeError;
		double altitudeError;
		double speedError;
		int mode;
		string time;
		
	public:
		gpsDate();
		~gpsDate();
		
		void set_valid(bool);
		void set_latitude(double);
		void set_longitude(double);
		void set_altitude(double);
		void set_speed(double);
		void set_track(double);
		void set_timeError(double);
		void set_longitudeError(double);
		void set_latitudeError(double);
		void set_altitudeError(double);
		void set_speedError(double);
		void set_time(string);
		void set_mode(int);
		
		bool   get_valid() 			{ return valid; }
		double get_latitude() 		{ return latitude; }
		double get_longitude() 		{ return longitude; }
		double get_altitude()		{ return altitude; }
		double get_speed()			{ return speed; }
		double get_track()			{ return track; }
		double get_timeError()		{ return timeError; }
		double get_longitudeError()	{ return longitudeError; }
		double get_latitudeError()	{ return latitudeError; }
		double get_altitudeError()	{ return altitudeError; }
		double get_speedError()		{ return speedError; }
		string get_time()			{ return time; }
		int	   get_mode()			{ return mode; }

		void print();
		void prettyPrint();
		int writeToFile( fstream& );
};

#endif /* gpsDate_H_ */
