#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import numpy as np
import csv

T_GUARD = 0.050   #50ms guard time
GPSD_LOG_FILE="gpsd.log"
POSITION_LOG_FILE="position.csv"

class NodeLogs:
	"""Class loading and holding all rmf log files of a node"""
	def __init__(self, role, directory, maxReceiveRate, DEBUG=False):
		self.DEBUG=DEBUG
		self.role = role
		self.directory = directory
		self.mSenderRawData = np.array([])
		self.mReceiverRawData = np.array([])
		self.mReceiverDelay = np.array([])
		self.mReceiverDatarate = np.array([])
		self.mReceiverGaps = np.array([])
		self.mReceiverNewTtx = np.array([])
		self.sReceiverData = np.array([])
		self.sSenderData = np.array([])
		self.streamData = np.array([])
		self.statistics = np.array([])
		self.streamDataFilename=''
		self.statisticDataFilename=''
		self.direction=''
		self.maxReceiveRate = maxReceiveRate
		if role=='server':
			self.streamDataFilename='upstream-data.csv'
			self.statisticDataFilename='upstream-stats.csv'
			self.direction='upstream'
		else:
			self.streamDataFilename='downstream-data.csv'
			self.statisticDataFilename='downstream-stats.csv'
			self.direction='downstream'

	def storeTupleListToCsv(self,tupleList,fileoutname) :
		'''Store the Tuplelist in the given filename as a csv'''
		print 'Storing ',len(tupleList),' lines in ',fileoutname
		with open(fileoutname, "wb") as outFile:
		    #csv.register_dialect("custom", delimiter=",", skipinitialspace=True)
		    #writer = csv.writer(outFile, dialect="custom")
		    writer = csv.writer(outFile)
		    for row in tupleList:
		        writer.writerow(row)
		    outFile.flush()
		return

	def readFromLogFile(self,suffix,skipHeader):
		'''Read the numpy Array from directory+self.role+suffix'''
		print('Reading logfile '+self.directory+'/'+self.role+suffix)
		npArray = np.genfromtxt(self.directory+'/'+self.role+suffix, dtype=None, delimiter=',', names=True, skip_header=skipHeader) 
		if self.DEBUG: print npArray.dtype.names
		return npArray

	def getmSenderRawData(self):
		if self.mSenderRawData.size == 0:
			self.mSenderRawData = self.readFromLogFile('.mSenderRawData',0)
		return self.mSenderRawData
		
	def getmReceiverRawData(self):
		if self.mReceiverRawData.size == 0:
			self.mReceiverRawData = self.readFromLogFile('.mReceiverRawData',0)
		return self.mReceiverRawData
		
	def getmReceiverDelay(self):
		if self.mReceiverDelay.size == 0:
			self.mReceiverDelay = self.readFromLogFile('.mReceiverDelay',1)
		return self.mReceiverDelay
		
	def getmReceiverDatarate(self):
		if self.mReceiverDatarate.size == 0:
			self.mReceiverDatarate = self.readFromLogFile('.mReceiverDatarate',1)
		return self.mReceiverDatarate
		
	def getmReceiverGaps(self):
		if self.mReceiverGaps.size == 0:
			self.mReceiverGaps = self.readFromLogFile('.mReceiverGaps',2)
		return self.mReceiverGaps
		
	def getmReceiverNewTtx(self):
		if self.mReceiverNewTtx.size == 0:
			self.mReceiverNewTtx = self.readFromLogFile('.mReceiverNewTtx',1)
		return self.mReceiverNewTtx
		
	def getsReceiverData(self):
		if self.sReceiverData.size == 0:
			self.sReceiverData = self.readFromLogFile('.sReceiverData',0)
		return self.sReceiverData

	def getsSenderData(self):
		if self.sSenderData.size == 0:
			self.sSenderData = self.readFromLogFile('.sSenderData',0)
		return self.sSenderData

	def getStreamData(self):
		if self.streamData.size == 0:
			self.streamData = self.readFromLogFile(self.streamDataFilename,0)
		return self.streamData	

	def getChirpSendData(self,chirp):
		rev_raw = self.getmSenderRawData()[::-1]
		idx_last = np.argmax(rev_raw['Chirp'] <= chirp)
		last = rev_raw[idx_last]
		if last['Chirp'] == chirp:
			chirpdata = { 'ttx_firstns' : last['ttx_firstns'] ,
						  'ttx_lastns' : last['ttxns'] ,
						  'packets' :last['Pack_1'], 
						  'BytePacket' :last['BytePacket'] }
		else:
			chirpdata = { 'ttx_firstns' : -1 ,
						  'ttx_lastns' : -1 ,
						  'packets' : -1, 
						  'BytePacket' : -1 }
		return chirpdata

	def checkOutOfOrderInner(self,strippedSortedSubArray):
		'''Returns true if the chirp/packet sorted list has received packets out of their normal ordering'''
		size = strippedSortedSubArray.size
		lasttrx_ns = strippedSortedSubArray[0]['trxns']
		for x in xrange(1,size -1):
			if lasttrx_ns > strippedSortedSubArray[x]['trxns']:
				return True
		return False

	def getStreamData(self,sender,overwrite=False):
		'''returns the chirp properties of all chirps send by sender and received by self'''
		streamFilename = self.directory+'/'+self.streamDataFilename
		statisticsFilename = self.directory+'/'+self.statisticDataFilename
		if self.streamData.size == 0 or overwrite:
			if os.path.isfile(streamFilename) and not overwrite:
				print('Reading logfile '+streamFilename)
				self.streamData = np.genfromtxt(streamFilename, dtype=None, delimiter=',', names=True, skip_header=0) 
				if self.DEBUG: 
					print self.streamData.dtype.names
					#print self.streamData.dtype.type
			else:
				tempStreamdata, tempStatistics = self.calculateStreamData(sender)
				self.storeTupleListToCsv(tempStreamdata, streamFilename)
				self.storeTupleListToCsv(tempStatistics, statisticsFilename)
				print('Reading logfile '+streamFilename)
				self.streamData = np.genfromtxt(streamFilename, dtype=None, delimiter=',', names=True, skip_header=0) 
				if self.DEBUG: 
					print self.streamData.dtype.names
					#print self.streamData.dtype.type
		return self.streamData

	def getStatistics(self,sender,overwrite=False):
		'''returns the statistics for all packets send by sender and received by self'''
		streamFilename = self.directory+'/'+self.streamDataFilename
		statisticsFilename = self.directory+'/'+self.statisticDataFilename
		if self.statistics.size == 0 or overwrite:
			if os.path.isfile(statisticsFilename) and not overwrite:
				print('Reading logfile '+statisticsFilename)
				self.statistics = np.genfromtxt(statisticsFilename, dtype=None, delimiter=',', names=True, skip_header=0) 
				if self.DEBUG: 
					print self.statistics.dtype.names
					#print self.streamData.dtype.type
			else:
				tempStreamdata, tempStatistics = self.calculateStreamData(sender)
				self.storeTupleListToCsv(tempStreamdata, streamFilename)
				self.storeTupleListToCsv(tempStatistics, statisticsFilename)
				print('Reading logfile '+streamFilename)
				self.statistics = np.genfromtxt(statisticsFilename, dtype=None, delimiter=',', names=True, skip_header=0) 
				if self.DEBUG: 
					print self.statistics.dtype.names
					#print self.streamData.dtype.type
		return self.statistics

	def calculateStreamData(self,sender):
		'''calculates the chirp properties of all chirps going from sender to receiver'''
		mReceiverRawData = self.getmReceiverRawData()
		mSenderRawData = sender.getmSenderRawData()
		minChirp = min(mReceiverRawData['Chirp'])
		maxChirp = max(mReceiverRawData['Chirp'])
		chirpCountsReceived = np.bincount(mReceiverRawData['Chirp'])
		lastChirp = 0
		lastPacket = 0;
		chirpdata = []
		chirpdata.append( ('#chirp','#send','#arrived','#arrivedOnce','#duplicates','packetsize [Bytes]','loss rate','data rate [Byte/s]','delay [s]',
			'first ttx [ns]','first trx [ns]','last ttx [ns]','last trx[ns]','delta trx [s]','t_gap_i [s]','unfiltered data rate [Byte/s]'))
		statistics = []
		statistics.append(('run','direction','duration[s]',
			'#packets','#packetloss','P(packetloss)',
			'#chirps','#chirploss','P(chirploss)',
			'#congested chirps','P(con)','#congestion phases',
			'mindely[ms]','maxdelay[ms]','avgdelay[ms]','r95delay[ms]',
			'longest congestion[ms]','longest congestion start chirp'
			,'#datarate valid','#datarate lost','#datarate invalid'))
		firstofachirpindex = np.argmax(mReceiverRawData['Pack'] > 0)
		sizeOfFirstPacket = mReceiverRawData['BytePacket'][0]
		reverseOrder = mReceiverRawData[::-1]
		length = len(reverseOrder)
		lastlastpacket = mReceiverRawData[0] #take the first packet as a dummy

		

		#packet statistics
		packets_send = mSenderRawData.size
		packets_received = mReceiverRawData.size
		packets_lost = 0 #used for counting lost packets

		duration = mSenderRawData['ttxns'][packets_send-1] - mSenderRawData['ttxns'][0]

		#congestion statistics
		lastNotCongestedChirp = mReceiverRawData['Chirp'][0]
		lastNotCongestedTtx = mReceiverRawData['ttxns'][0] #dummy for longest congestion phase calculation
		longestCongestionPhaseLengthNs = 0 #dummy for longest congestion phase timespan
		longestCongestionPhaseStartChirp = 0
		currentlyInCongestion = False
		congestionPhases = 0
		chirps_congested = 0

		#number of chirps
		chirps_send = maxChirp #this is not exactly correct if complete chirps are lost at the end of the measurement
		chirps_received = 1 #as we do not count the first chirp
		chirps_lost = 0

		#calc delay(min,max,avg,r95)
		packetdelays = mReceiverRawData['trxns'] - mReceiverRawData['ttxns']
		mindelay = np.min(packetdelays)
		maxdelay = np.max(packetdelays)
		avgdelay = np.average(packetdelays)
		r95delay = np.sort(packetdelays)[int(packetdelays.size*0.95)]

		#datarate statistics
		datarate_invalid = 0
		datarate_valid = 0 
		datarate_lost = 0
		
		#iterate over all the arrived chirps
		for x in xrange(minChirp,maxChirp):
			idx_firstPacketOfChirpX = np.argmax(mReceiverRawData['Chirp'] >= x )			#index of first received packet of chirp x
			idx_lastPacketOfChirpX = length - np.argmax(reverseOrder['Chirp'] <= x )     	#index of last  received packet of chirp x
			firstPacket = mReceiverRawData[idx_firstPacketOfChirpX] 						#first received packet of chirp x
			lastPacket = mReceiverRawData[(idx_lastPacketOfChirpX-1)] 						#last  received packet of chirp x
			subArray = mReceiverRawData[idx_firstPacketOfChirpX:idx_lastPacketOfChirpX:1]   #array containing all packets of chirp x plus probably out of order packets of other chirps

			if len(subArray)==0:
				chirps_lost+=1
				datarate_lost+=1
				#print 'chirp ',x,'was completely lost, filling some of the missing data with data from the sender'
				send=sender.getChirpSendData(x)
				packets_lost+=send['packets']
				#chirpdata.append( ('chirp','#send','#arrived','#arrivedOnce','#duplicates','packetsize [Bytes]','loss rate %','data rate [Byte/s]','delay [ns]','first ttx [ns]','first trx [ns]','last ttx [ns]','last trx [ns], delta trx [s]','t_gap_i [s]'),'unfiltered data rate [Byte/s]')
				chirpdata.append(  (x      ,send['packets'], 0 ,   0         ,       0     , send['BytePacket'] , 100.0       ,float('nan')        , float('nan'),send['ttx_firstns'],     -1     , send['ttx_lastns'], -1       , float('nan')  , float('nan') , float('nan')))
			else:
				if firstPacket['Chirp']!=lastPacket['Chirp'] and self.DEBUG:
					print idx_firstPacketOfChirpX,'/',idx_lastPacketOfChirpX,' ',firstPacket['Chirp'],':',firstPacket['Pack'],' / ',lastPacket['Chirp'],':',lastPacket['Pack']
					print subArray

				sortedSubArray = subArray[subArray[['Chirp','Pack']].argsort()]                     #sorted array of the subarray by chirp,packet
				idx_start = np.argmax(sortedSubArray['Chirp'] >= x )								#indexes of packet 1 and packet n of chirp x in the sorted array
				idx_end = np.argmax(sortedSubArray['Chirp'] >= (x+1))
				if idx_end==0:
					idx_end = len(sortedSubArray)
				strippedSortedSubArray = sortedSubArray[idx_start : idx_end :1]						#just the packets of chirp x sorted by chirp,packet
				arrived = len(strippedSortedSubArray)												#arrived packets of chirp x (with duplicates)
				if arrived==0:
					chirps_lost+=1
					datarate_lost+=1
					send=sender.getChirpSendData(x)
					#chirpdata.append( ('chirp','#send','#arrived','#arrivedOnce','#duplicates','packetsize [Bytes]','loss rate %','data rate [Byte/s]','delay [ns]','first ttx [ns]','first trx [ns]','last ttx [ns]','last trx [ns], delta trx [s]','t_gap_i [s]'),'unfiltered data rate [Byte/s]')
					chirpdata.append(  (x      ,send['packets'], 0 ,   0         ,       0     , send['BytePacket'] , 100.0       ,float('nan')        , float('nan'),send['ttx_firstns'],     -1     , send['ttx_lastns'], -1       , float('nan')  , float('nan') ,float('nan')))  
				else:
					chirps_received+=1
					packetbins = np.bincount(strippedSortedSubArray['Pack'])
					lost_ArrivedOnce_Duplicates = np.bincount(packetbins[1:len(packetbins):1])  		
					lost = lost_ArrivedOnce_Duplicates[0]												#[0] stores the number of lost packets
					arrivedOnce = lost_ArrivedOnce_Duplicates[1]										#[1] stores the number of packets which arrived once
					duplicates = arrived - arrivedOnce 													

					outOfOrderOuter =  not (arrived == len(subArray))									#did packets of other chirps arrive during reception of chirp x
					outOfOrderInner = self.checkOutOfOrderInner(strippedSortedSubArray)		                #did the packets of this chirp arrive in correct ordering

					#if lost > 0 or duplicates >0:
					if self.DEBUG and duplicates >0: # for debug purposes print the subarrays for chipr x
						print 'x, idx_firstPacketOfChirpX, idx_lastPacketOfChirpX ',x, idx_firstPacketOfChirpX, idx_lastPacketOfChirpX
						print subArray
						print '-----------------------------'
						print sortedSubArray
						print 'idx_start,idx_end,outOfOrderOuter ',idx_start,idx_end, outOfOrderOuter
						print '*****************************'
						print strippedSortedSubArray
						print '#############################'
						print packetbins
						print lost_ArrivedOnce_Duplicates
						print 'send,arrivedOnce,duplicates,lost ', firstPacket['Pack_1'],  arrivedOnce, duplicates, lost
						print '======================================'
					
					firstttx_ns = int(firstPacket['ttx_firstns'])   											#transmit time of first arrived packet of chirp x
					firsttrx_ns = int(firstPacket['trxns'])														#receive  time of first arrived packet of chirp x
					lastttx_ns = int(lastPacket['ttxns'])														#transmit time of last  arrived packet of chirp x
					lasttrx_ns = int(lastPacket['trxns'])														#receive  time of last  arrived packet of chirp x

					total = int(firstPacket['Pack_1'])															#number of send packets of chirp x
					lossrate = float( (100.0* (total - arrivedOnce))/total )    								#lossrate for the chirp (cleaned from duplicates and out of order)
					packsize = int(lastPacket['BytePacket'])													#the size of the packets of chirp x (the first might be smaller)

					t_gap_i=float(firsttrx_ns-lastlastpacket['trxns'])/1000000000.0	    						#the gap between chirp x and chirp x-1 in seconds
					if t_gap_i > 1: #larger than 1 s gap is supsicous
						print 'high t_gap_i due to out of order packets t_gap_i, (c:p), (c:p) ',t_gap_i,' (',firstPacket['Chirp'],':',firstPacket['Pack'],') (',lastlastpacket['Chirp'],':',lastlastpacket['Pack'],')'

					#check t_gap_i before using delay!
					if t_gap_i >= T_GUARD:
						if firstPacket['Pack'] <=5: 
							delay_ns = float(firstPacket['trxns'] - firstPacket['ttxns'])/1000000000.0				#the delay of the first received packet of the chirp is usable
							if currentlyInCongestion:
								#this is the end of the congestion
								conlengthNs = firstPacket['trxns'] - lastNotCongestedTtx
								if conlengthNs > longestCongestionPhaseLengthNs: 
									longestCongestionPhaseLengthNs = conlengthNs
									longestCongestionPhaseStartChirp = lastNotCongestedChirp
							lastNotCongestedTtx = firstPacket['ttxns']
							lastNotCongestedChirp = firstPacket['Chirp']
							currentlyInCongestion=False
						else:
							#special case where we can not really decide if a congestion starts or ends here so we ignore it for congestion calculations
							delay_ns = float('nan')																	#the delay of the first received packet of the chirp is unusable	
					else:
						if t_gap_i < 0:
							print 'wrong t_gap_i due to out of order packets t_gap_i, (c:p), (c:p) ',t_gap_i,' (',firstPacket['Chirp'],':',firstPacket['Pack'],') (',lastlastpacket['Chirp'],':',lastlastpacket['Pack'],')'
							t_gap_i = float('nan')

						chirps_congested+=1
						delay_ns = float('nan')																	#the delay of the first received packet of the chirp is unusable
						if self.DEBUG: print 'chirp=',x,' lastchirp=',lastlastpacket['Chirp'],' t_gap_i=',t_gap_i
								
						if not currentlyInCongestion:
							currentlyInCongestion = True
							congestionPhases+=1
						else: 
							#count it - maybe we do not leave the congestion phase before the measurement ends
							conlengthNs = firstPacket['trxns'] - lastNotCongestedTtx
							if conlengthNs > longestCongestionPhaseLengthNs: 
								longestCongestionPhaseLengthNs = conlengthNs
								longestCongestionPhaseStartChirp = lastNotCongestedChirp

					if arrivedOnce >=2: 
						deltatrx_s = (int(lastPacket['trxns']) - int(firstPacket['trxns']))/1000000000.0
						datarate = int( (arrivedOnce - 1.0) * packsize / deltatrx_s)
						unfiltered_datarate = datarate
						if datarate > self.maxReceiveRate: #filter unbelievable values
							print 'rate/max exceeded for chirp (',x,'): ',datarate,' / ',self.maxReceiveRate
							datarate = float('nan')
							datarate_invalid+=1
						else :
							datarate_valid+=1
					else: #intentionally we do not handle chirps with a single arrived packet as the data rate is not really useable
						datarate = float('nan')
						unfiltered_datarate = datarate
						deltatrx_s = float('nan')
						datarate_invalid+=1

					#chirpdata.append( ('chirp','#send','#arrived','#arrivedOnce','#duplicates','packetsize [Bytes]','loss rate %','data rate [Byte/s]','delay [ns]','first ttx [ns]','first trx [ns]','last ttx [ns]','last trx [ns], delta trx [s]','t_gap_i [s]'))
					chirpdata.append( (     x  ,total  ,arrived   ,arrivedOnce   ,duplicates   ,packsize            ,lossrate     ,datarate            ,delay_ns    ,firstttx_ns     ,firsttrx_ns     ,lastttx_ns     ,lasttrx_ns    ,deltatrx_s     ,t_gap_i,unfiltered_datarate))

					lastlastpacket = lastPacket
		##FIXME: add data for chirp before and after minchirp that were send!
		#
		packet_loss_probability = packets_lost*100.0/packets_send
		congestion_probability = chirps_congested * 100.0 / chirps_send
		chirp_loss_probability = chirps_lost * 100.0 / chirps_send

		# statistics.append(('run','direction','duration[s]',
		# 	'#packets','#packetloss','P(packetloss)',
		# 	'#chirps','#chirploss','P(chirploss)',
		# 	'#congested chirps','P(con)','#congestion phases',
		# 	'mindely[ms]','maxdelay[ms]','avgdelay[ms]','r95delay[ms]',
		# 	'longest congestion[ms]','longest congestion start chirp'
		# 	,'#datarate valid','#datarate lost','#datarate invalid'))
		statistics.append((self.directory, self.direction, duration/1E9, 
			packets_send, packets_lost, packet_loss_probability, 
			chirps_send, chirps_lost, chirp_loss_probability, 
			chirps_congested, congestion_probability, congestionPhases, 
			mindelay/1E6, maxdelay/1E6, avgdelay/1E6, r95delay/1E6, 
			longestCongestionPhaseLengthNs/1E6,longestCongestionPhaseStartChirp,
			datarate_valid, datarate_lost, datarate_invalid) )                                                                                                                      

		print statistics
		# print 'duration[s] ',duration/1E9
		# print 'packets(send,ploss,P(loss) (',packets_send,',',packets_lost,',',packet_loss_probability,')'
		# print 'chirps(send,received,dropped,loss_probability) (',chirps_send,',',chirps_received,',',chirps_lost,',',chirp_loss_probability,')'
		# print 'congestion(phases,congested_chirps,congestion_probablilty,longest_congestion_phase[ms],longest_phase_start_chirp) (',congestionPhases,',',chirps_congested,',',congestion_probability,',',longestCongestionPhaseLengthNs/1E6,',',longestCongestionPhaseStartChirp,')'
		# print 'delay[ms](min,max,avg,r95) (',mindelay/1E6,',',maxdelay/1E6,',',avgdelay/1E6,',',r95delay/1E6,')'
		# print 'datarate(valid,lost,too_high) (',datarate_valid,',',datarate_lost,',',datarate_invalid,')'
		return (chirpdata,statistics)

	def getGpsData(self,sender):
		'''returns the chirp properties of all chirps send by sender and received by self'''
		outfilename = self.directory+'/'+self.streamDataFilename
		if self.streamData.size == 0:
			if os.path.isfile(self.directory+'/'+POSITION_LOG_FILE):
				print('Reading logfile '+outfilename)
				self.streamData = np.genfromtxt(outfilename, dtype=None, delimiter=',', names=True, skip_header=0) 
			else:
				temp = self.calculateStreamData(sender)
				print('Writing logfile '+outfilename)
				self.storeTupleListToCsv(temp,outfilename)
				print('Reading logfile '+outfilename)
				self.streamData = np.genfromtxt(outfilename, dtype=None, delimiter=',', names=True, skip_header=0) 
		return self.streamData

	def convertGpsdlogToCsv(self):
		filename = self.directory+"/"+GPSD_LOG_FILE
		filenameout = self.directory+"/"+POSITION_LOG_FILE
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

	def getNormalization(self):
		rawSend = self.getmSenderRawData()
		norm = int( rawSend['ttxns'][0] )
		if self.DEBUG: print 'Normalization = ',norm
		return norm

#some basic testing of the class
#logs = NodeLogs('server','/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150619/182357',True)
# logs.getmReceiverDelay()
# logs.getmReceiverGaps()
# logs.getsReceiverData()
# logs.getsSenderData()
# logs.getmReceiverNewTtx()
# logs.getmReceiverRawData()
# logs.getmSenderRawData()
# logs.getmReceiverDatarate()
# logs.getmReceiverDatarate()
# logs.getmSenderRawData()
#print logs.getChirpSendData(-1)
#print logs.getChirpSendData(200000000)
#print logs.getChirpSendData(2000)