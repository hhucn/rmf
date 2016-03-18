#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import numpy as np
import nodelogs
import time

class BothLogs:
	"""Class holding instances to client and server log file classes"""
	def __init__(self, directory, DEBUG=False, maxUpstreamBytes=5760000/8, maxDownStreamBytes=21000000/8):
	#def __init__(self, directory, DEBUG=False, maxUpstreamBytes=5760000/8, maxDownStreamBytes=7200000/8):
	#def __init__(self, directory, DEBUG=False, maxUpstreamBytes=2000000/8, maxDownStreamBytes=7200000/8):
		print 'cwd in init = ',os.getcwd()
		self.directory = directory
		self.DEBUG=DEBUG
		os.chdir(directory)
		maxUpStreamBytes,maxDownStreamBytes = self.getMaxUpMaxDown()
		print "Using the following maxup/maxdown data rates [Bytes/s]: "+maxUpStreamBytes+"/"+maxDownStreamBytes
		self.client = nodelogs.NodeLogs('client',directory,maxDownStreamBytes, DEBUG)
		self.server = nodelogs.NodeLogs('server',directory,maxUpstreamBytes, DEBUG)
		self.statistics = np.array([]) # initialize empty statistics for this measurement
		self.statisticsFilename = 'stats.csv'

	def getMaxUpMaxDown(self):
		#filename = self.directory+"/client.log"
		filename = "client.log"
		# [rmf.1, Sep 07 07:51:25.735; Notice]: MEME| -> Sent message to server: "MEASURE_TBUS_SYNC|0|1|1|42|50|3|25|20000|20000|20000|2625000|720000|0|0|/dev/ttyS0|2947|"
		# #[rmf.1, Sep 07 07:51:25.735; Informational]: MEME| -> Found maximal datarate for the download = 2625000[Byte/s]
		#[rmf.1, Sep 07 07:51:25.735; Informational]: MEME| -> Found maximal datarate for the upload = 720000[Byte/s]
		TBUS_SYNC="MEASURE_TBUS_SYNC"
		MAX_DOWNLOAD="MEME| -> Found maximal datarate for the download = "
		MAX_UPLOAD="MEME| -> Found maximal datarate for the upload = "

		maxdown=0
		maxdownfound=False
		maxup=0
		maxupfound=False

		print 'cwd in getMaxUpMaxDown = ',os.getcwd()
		with open(filename) as f:
		    line = f.readline()
		    while line and not (maxdownfound and maxupfound):
		    	if not maxdownfound and MAX_DOWNLOAD in line:
		    		maxdown = line.split('= ')[1].split('[')[0]
		    		maxdownfound=True
		    	elif not maxupfound and MAX_UPLOAD in line:
		    		maxup = line.split('= ')[1].split('[')[0]
		    		maxdupfound=True
		    	elif not maxdownfound and TBUS_SYNC in line:
		    		#this part is sensible about changed tbus sync parameters!
		    		splits = line.split('|')
		    		maxdown = splits[12]
		    		maxup = splits[13]
		    		maxdownound=True
		    		maxupfound=True
		    		print "Only second best data rate descriptions in TBUS_SYNC found"
		    	line = f.readline()

		print "Found the following maxup/maxdown data rates [Bytes/s]: "+maxup+"/"+maxdown
		return (maxup,maxdown)

	def getNormalization(self):
		return max(self.client.getNormalization(),self.server.getNormalization())

	def recalcAllStreamData(self):
		print 'Starting recalculation for streamdata in directory ',self.directory
		self.client.getStreamData(self.server,True)
		self.server.getStreamData(self.client,True)
		print 'Recalculation for Streamdata in directory ',self.directory,' done'
		return

	def getStatistics(self,overwrite=False):
		#outfilename = self.directory+'/'+self.statisticsFilename
		outfilename = self.statisticsFilename
		if self.statistics.size == 0 or overwrite:
			if os.path.isfile(outfilename) and not overwrite:
				print('Reading logfile '+outfilename)
				self.statistics = np.genfromtxt(outfilename, dtype=None, delimiter=',', names=True, skip_header=0) 
				if DEBUG:
					print self.statistics.dtype.names
					#print self.statistics.dtype.type
			else:
				temp = self.calculateStatistics(self)
				self.client.storeTupleListToCsv(temp,outfilename) ##use the nodelogs function to write the tuple to disk
				print('Reading logfile '+outfilename)
				self.statistics = np.genfromtxt(outfilename, dtype=None, delimiter=',', names=True, skip_header=0) 
				if self.DEBUG: 
					print self.statistics.dtype.names
					#print self.statistics.dtype.type
		return self.statistics

	def getStatsForArray(self,nparray,prefix):
		pref = prefix+'_'
		non_nan_nparray = nparray[~np.isnan(nparray)]
		usable = non_nan_nparray.size
		usable_percent = 100.0*usable/nparray.size 
		mini = np.min(non_nan_nparray)
		maxi = np.max(non_nan_nparray)
		mean = np.mean(nparray)
		nanmean = np.nanmean(nparray)
		med = np.median(nparray)
		avg = np.average(non_nan_nparray)
		return { pref+'min' : mini,
				 pref+'max'	: maxi,
				 pref+'avg'	: avg,
				 pref+'mean' : mean,
				 pref+'nanmean' : nanmean,
				 pref+'med'  : med,
				 pref+'usable' : usable,
				 pref+'usable_percent' : usable_percent }
		
	def calculateStatistics(self):
		downstream = self.client.getStreamData(self.server,False)
		upstream = self.server.getStreamData(self.client,False)
		upstreamstats = {'duration_s' : (upstream['last_trxns'][upstream.size -1]-upstream['first_ttx_ns'][1])/1E9 }

		up_chirps = upstream.size -1 
		up_send = np.sum(upstream['send'])
		up_arrived = np.sum(upstream['arrived'])
		up_arrivedonce = np.sum(upstream['arrivedOnce'])
		up_duplicates = np.sum(upstream['duplicates'])
		up_lost = up_send - up_arrivedonce
		up_bytes_send = np.sum(upstream['packetsize_Bytes']*(upstream['send']-1)) + 126*up_chirps
		up_min_delay_s = np.nanmin(upstream['delay_s'])
		non_nan_delays = upstream['delay_s'][~np.isnan(upstream['delay_s'])]  # remove the nans from the nparray
		up_average_delay_s = np.average(non_nan_delays)
		up_mean_delay_s = np.nanmean(upstream['delay_s'])
		up_max_delay_s = np.nanmax(upstream['delay_s'])
		#up_outOfOrderInner = np.sum(upstream[''])
		up_usable_delays = non_nan_delays.size
		up_duration_s = (upstream['last_trxns'][upstream.size -1]-upstream['first_ttx_ns'][1])/1E9
		print 'up_duration [s] = ',up_duration_s
		print self.getStatsForArray(upstream['delay_s'],'delay_s')
		print
		print self.getStatsForArray(upstream['t_gap_i_s'],'t_gap_i_s') #TODO tgap_usable is wrong as zero is the bad thing here
		print
		print self.getStatsForArray(upstream['delta_trx_s'],'delta_trx_s')
		print
		print self.getStatsForArray(upstream['data_rate_Bytes'],'data_rate_Bytes')
		print 

		upstreamstats=dict(self.getStatsForArray(upstream['delay_s'],'delay_s').items() 
						  +self.getStatsForArray(upstream['t_gap_i_s'],'t_gap_i_s').items()
						  +self.getStatsForArray(upstream['data_rate_Bytes'],'data_rate_Bytes').items()
						  +self.getStatsForArray(upstream['t_gap_i_s'],'t_gap_i_s').items())
		print 'Upstreamstts: '
		print upstreamstats
		# upstreamstats.append(self.getStatsForArray(upstream['delay_s'],'delay_s'))
		# upstreamstats.merge(self.getStatsForArray(upstream['t_gap_i_s'],'t_gap_i_s'))

		import csv
		f = open('dict.csv','wb')
		w = csv.DictWriter(f,upstreamstats.keys())
		w.writeheader()
		w.writerow(upstreamstats)
		f.close()
		
		upstreamStats = { 'duration'		: up_duration_s,
						  'chirps'          : up_chirps ,
		 				  'send'            : up_send,
		 				  'arrived'         : up_arrived,
		 				  'arrivedOnce'     : up_arrivedonce,
		 				  'duplicates'      : up_duplicates,
		 				  'lost'		    : up_lost,
		 				  'bytes_send'      : up_bytes_send,
		 				  'min_delay_s'	    : up_min_delay_s,
		 				  'max_delay_s'     : up_max_delay_s,
		 				  'mean_delay_s'    : up_mean_delay_s,
		 				  'avg_delay_s'     : up_average_delay_s,
		 				  'usable_delays'   : up_usable_delays,
		 				  'usable_delays_%' : 100.0*up_usable_delays/up_chirps
		 				  }
		print 'Upstreamstts: '
		print upstreamStats

		# rev_raw = self.getmSenderRawData()[::-1]
		# idx_last = np.argmax(rev_raw['Chirp'] <= chirp)
		# last = rev_raw[idx_last]
		# if last['Chirp'] == chirp:
		# 	chirpdata = { 'ttx_firstns' : last['ttx_firstns'] ,
		# 				  'ttx_lastns' : last['ttxns'] ,
		# 				  'packets' :last['Pack_1'], 
		# 				  'BytePacket' :last['BytePacket'] }
		# else:
		# 	chirpdata = { 'ttx_firstns' : -1 ,
		# 				  'ttx_lastns' : -1 ,
		# 				  'packets' : -1, 
		# 				  'BytePacket' : -1 }
		# return chirpdata

	def calculateSimpleStatisticsForPaper(self):
		downstream = self.client.getStreamData(self.server,False)
		print 
		print self.calculateSimpleStatisticsForPaperOneDir(downstream,'downstream')

		upstream = self.server.getStreamData(self.client,False)
		print 
		print self.calculateSimpleStatisticsForPaperOneDir(upstream,'upstream')
		return

	def calculateSimpleStatisticsForPaperOneDir(self,streamdata,direction):
		duration_s = (streamdata['last_trxns'][streamdata.size -1]-streamdata['first_ttx_ns'][1])/1E9
		packets_send = np.sum(streamdata['send'])
		packets_arrived = np.sum(streamdata['arrived'])
		packets_arrivedonce = np.sum(streamdata['arrivedOnce'])
		packets_duplicates = np.sum(streamdata['duplicates'])
		packets_lost = packets_send - packets_arrivedonce
		packets_ploss = packets_lost *100.0/ packets_send
		
		chirps_send = streamdata['chirp'][streamdata.size-1]  #the last chirp send carries this number and we start at 1
		chirps_lost = chirps_send - np.count_nonzero(streamdata['arrived'])  #TODO: this is wrong, we have to count chirps with no arrivals
		chirps_plost = chirps_lost * 100.0 / chirps_send

		#TODO: we need to get the congestion stats
		chirps_congested = 0
		chirps_pcongested = 0
		congestion_phases = 0

		min_delay_s = np.nanmin(streamdata['delay_s'])
		non_nan_delays = streamdata['delay_s'][~np.isnan(streamdata['delay_s'])]  # remove the nans from the nparray
		average_delay_s = np.average(non_nan_delays)
		mean_delay_s = np.nanmean(streamdata['delay_s'])
		max_delay_s = np.nanmax(streamdata['delay_s'])
		delays = streamdata['delay_s']
		sorted_nonnan_delays = delays[~np.isnan(delays)]
		print 'delays/nonnandelay ',streamdata.size,'/',sorted_nonnan_delays.size
		print delays
		#outOfOrderInner = np.sum(streamdata[''])
		usable_delays = non_nan_delays.size
		bytes_send = np.sum(streamdata['packetsize_Bytes']*(streamdata['send']-1)) + 126*chirps_send
		return 			{ 'run'				: self.directory,
						  'trace direction'	: direction,
					      'duration'		: duration_s,
					      'packets'			: packets_send,
					      'ploss'			: packets_lost,
					      'P(ploss)'		: packets_ploss,
						  'trains'          : chirps_send,
		 				  'tloss'           : chirps_lost,
		 				  'P(tloss)'        : chirps_plost,
		 				  'con'				: chirps_congested,
		 				  'P(con)'			: chirps_pcongested,
		 				  'phases'			: congestion_phases,
		 				  'rmin'			: min_delay_s*1000,
		 				  'rmax'			: max_delay_s*1000,
		 				  'ravg'			: average_delay_s*1000,
		 				  'r95'				: 'nan'
		 				  }

#some simple tests for the class
# directories = []
# directories.append(r'/home/goebel/rmf-logs-to-keep/testlogs')
# directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury-h5321gw/20150903/090013') # Wehrhahn -> Uni (beide Seiten mit Drossel für Emergency SC Nachrichten)
# directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury-h5321gw/20150903/080121') # Münchener Straße -> Wehrhahn. Leider auf dem Laptop noch ohne emergency message Drossel
# for directory in directories: 
# 	print '\n\n'
# 	print 'Generating stats for dir = '+directory
# 	bothlogs = BothLogs(directory,False)
# 	bothlogs.recalcAllStreamData()
#	bothlogs.calculateSimpleStatisticsForPaper()
# print bothlogs.server.getStreamData(bothlogs.client)
# print('**************************************')
# print('**************************************')
# print bothlogs.client.getStreamData(bothlogs.server)
