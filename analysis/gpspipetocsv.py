#!/usr/bin/env python
# -*- coding: utf-8 -*-
# 
import csv

GPSD_LOG_FILE="gpsd.log"
POSITION_LOG_FILE="position.csv"

def convertGpsdlogToCsv(directory):
	filename = directory+"/"+GPSD_LOG_FILE
	filenameout = directory+"/"+POSITION_LOG_FILE
	split = []
	positiondata = []
	with open(filename,'r') as f:
		positiondata.append(('#time','lat','lon','speed','track'))
		for line in f:
			split = line.split(',')
			if "GGA" in split[1]:
				time = split[0].split(':')[0]
				
				for token in split:
					if "lat" in token:
						lat=token.split(':')[1]
					if "lon" in token:
						lon=token.split(':')[1]
					if "speed" in token:
						speed=token.split(':')[1]
					if "track" in token:
						track=token.split(':')[1]
				positiondata.append((time,lat,lon,speed,track))
	
				#print 'time=',time ,' lat=',lat,' lon=',lon,' speed=',speed,' track=',track,' ***',split
		#print positiondata
	
	with open(filenameout, "w") as outFile:
	    #csv.register_dialect("custom", delimiter=",", skipinitialspace=True)
	    #writer = csv.writer(outFile, dialect="custom")
	    writer = csv.writer(outFile)
	    for row in positiondata:
	        writer.writerow(row)
	    outFile.flush()

	print 'GPS data written to ',filenameout
	return

