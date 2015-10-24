/**
 * @authors Tobias Krauthoff <tobias.krauthoff@hhu.de, krauthoff@cs.uni-duesseldorf.de>
 * @date 23. June 2014
 * global position system date
 */

#ifndef gpsDate_H_
#define gpsDate_H_

using namespace std;


#include <string>
#include <stdint.h>

#include <fstream>
#include <sstream>


class GpsDate {
	private:
		bool isValid_;
		double nLatitude_;
		double nLongitude_;
		double nAltitude_;
		double nSpeed_;
		double nTrack_;
		double nTimeError_;
		double nLongitudeError_;
		double nLatitudeError_;
		double nAltitudeError_;
		double nSpeedError_;
		int nMode_;
		string time_;
		
	public:
		GpsDate();
		~GpsDate();
		
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
		
		bool   is_valid() 			{ return isValid_; }
		double get_latitude() 		{ return nLatitude_; }
		double get_longitude() 		{ return nLongitude_; }
		double get_altitude()		{ return nAltitude_; }
		double get_speed()			{ return nSpeed_; }
		double get_track()			{ return nTrack_; }
		double get_timeError()		{ return nTimeError_; }
		double get_longitudeError()	{ return nLongitudeError_; }
		double get_latitudeError()	{ return nLatitudeError_; }
		double get_altitudeError()	{ return nAltitudeError_; }
		double get_speedError()		{ return nSpeedError_; }
		string get_time()			{ return time_; }
		int32_t	   get_mode()			{ return nMode_; }

		void printPantheiosDebug();
		void printStdCout();
		void printPrettyPantheiosDebug();
		int32_t writeToFile( fstream& );
};

#endif /* gpsDate_H_ */
