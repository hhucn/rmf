#!/usr/bin/env python
# -*- coding: utf-8 -*-
# 
import bothlogs

import matplotlib.pyplot as plt
import time
import getopt,sys
import os

from pylab import rcParams
rcParams['figure.figsize'] = 10, 10


NORMALIZE = 0
BILLION = 1000000000.0
MILLION = 1000000.0
DEBUG = True
#TYPE = 'trx','ttx','chirp'
TYPE_TRX = 'trx'
TYPE_TTX = 'ttx'
TYPE_CHIRP = 'chirp'
HOW = TYPE_CHIRP

def normalize(what):
	return (what - NORMALIZE)

def plotDatarateAndLossrate(subplot,streamdata, title, how=HOW):
	if how==TYPE_TRX:     subplot.plot( normalize(streamdata['first_trx_ns'])/BILLION ,streamdata['data_rate_Bytes']/1000,'b', label='Data rate vs. trx'  ,drawstyle='steps-post',linewidth=1)
	elif how==TYPE_TTX:   subplot.plot( normalize(streamdata['first_ttx_ns'])/BILLION ,streamdata['data_rate_Bytes']/1000,'b', label='Data rate vs. ttx'  ,drawstyle='steps-post',linewidth=1)
	elif how==TYPE_CHIRP: subplot.plot(          (streamdata['chirp']       )         ,streamdata['data_rate_Bytes']/1000,'b', label='Data rate vs. chirp',drawstyle='steps-post',linewidth=1)
	#subplot.set_title(title)
	subplot.set_ylabel('Data rate [KByte/s]', color='b')
	subplot.legend(loc=2)
	subplot.grid(True)

	righty = subplot.twinx()
	righty.set_ylabel('Loss rate [%]', color='r')
	if how==TYPE_TRX:     righty.plot( normalize(streamdata['first_trx_ns'])/BILLION ,streamdata['loss_rate'],'r', label='Loss rate vs. trx'  ,drawstyle='steps-post',linewidth=1)
	elif how==TYPE_TTX:   righty.plot( normalize(streamdata['first_ttx_ns'])/BILLION ,streamdata['loss_rate'],'r', label='Loss rate vs. ttx'  ,drawstyle='steps-post',linewidth=1)
	elif how==TYPE_CHIRP: righty.plot(          (streamdata['chirp']       )         ,streamdata['loss_rate'],'r', label='Loss rate vs. chirp',drawstyle='steps-post',linewidth=1)
	righty.set_ylim([0,100])
	righty.legend(loc=1)
	return

def plotDelay(subplot,streamdata,title,how=HOW):
	if how==TYPE_TRX:     subplot.plot( normalize(streamdata['first_trx_ns'])/BILLION ,streamdata['delay_s']*1000,'r', label='Delay vs. trx'  ,drawstyle='steps-post',linewidth=1)
	elif how==TYPE_TTX:   subplot.plot( normalize(streamdata['first_ttx_ns'])/BILLION ,streamdata['delay_s']*1000,'r', label='Delay vs. ttx'  ,drawstyle='steps-post',linewidth=1)
	elif how==TYPE_CHIRP: subplot.plot(          (streamdata['chirp']       )         ,streamdata['delay_s']*1000,'r', label='Delay vs. chirp',drawstyle='steps-post',linewidth=1)
	#subplot.set_title(title)
	subplot.set_ylabel('Delay [ms]', color='r')
	subplot.legend(loc=2)
	subplot.grid(True)
	return

def plotTtxFirst(subplot,streamdata,title,how=HOW):
	if how==TYPE_TRX:     subplot.plot( normalize(streamdata['first_trx_ns'])/BILLION ,streamdata['first_ttx_ns'],'y', label='Delay vs. trx'  )
	elif how==TYPE_TTX:   subplot.plot( normalize(streamdata['first_ttx_ns'])/BILLION ,streamdata['first_ttx_ns'],'y', label='Delay vs. ttx'  )
	elif how==TYPE_CHIRP: subplot.plot(          (streamdata['chirp']       )         ,streamdata['first_ttx_ns'],'y', label='Delay vs. chirp')
	#subplot.set_title(title)
	subplot.set_ylabel('ttx first [ns]')
	subplot.legend(loc=2)
	subplot.grid(True)
	return

def plotTGap(subplot,streamdata,title,how=HOW):
	if how==TYPE_TRX:     subplot.plot( normalize(streamdata['first_trx_ns'])/BILLION ,streamdata['t_gap_i_s']*1000,'g.', label='t_gap_i vs. trx'  ,markersize=2)
	elif how==TYPE_TTX:   subplot.plot( normalize(streamdata['first_ttx_ns'])/BILLION ,streamdata['t_gap_i_s']*1000,'g.', label='t_gap_i vs. ttx'  ,markersize=2)
	elif how==TYPE_CHIRP: subplot.plot(          (streamdata['chirp']       )         ,streamdata['t_gap_i_s']*1000,'g.', label='t_gap_i vs. chirp',markersize=2)
	#subplot.set_title(title)
	subplot.axhline(y=50 ,linewidth=1,c='black',zorder=0)
	subplot.axhline(y=100,linewidth=1,c='black',zorder=0)
	subplot.axhline(y=200,linewidth=1,c='black',zorder=0)
	subplot.set_ylabel('t_gap_i [ms]')#, color='g',drawstyle='dotted')
	subplot.legend(loc=2)
	subplot.grid(True)
	return

def plotTGapZoom(subplot,streamdata,title,how=HOW):
	if how==TYPE_TRX:     subplot.plot( normalize(streamdata['first_trx_ns'])/BILLION ,streamdata['t_gap_i_s']*1000,'g.', label='t_gap_i vs. trx'  ,markersize=2)
	elif how==TYPE_TTX:   subplot.plot( normalize(streamdata['first_ttx_ns'])/BILLION ,streamdata['t_gap_i_s']*1000,'g.', label='t_gap_i vs. ttx'  ,markersize=2)
	elif how==TYPE_CHIRP: subplot.plot(          (streamdata['chirp']       )         ,streamdata['t_gap_i_s']*1000,'g.', label='t_gap_i vs. chirp',markersize=2)
	#subplot.set_title(title)
	subplot.axhline(y=50 ,linewidth=1,c='black',zorder=0)
	subplot.axhline(y=100,linewidth=1,c='black',zorder=0)
	subplot.axhline(y=200,linewidth=1,c='black',zorder=0)
	subplot.axis(ymin=0,ymax=500)
	subplot.set_ylabel('t_gap_i [ms]')#, color='g',drawstyle='dotted')
	subplot.legend(loc=2)
	subplot.grid(True)
	return	

def plotIndividualPacketDelays(subplot,mReceiverRawData,title,how=HOW):
	delay = (mReceiverRawData['trxns'] - mReceiverRawData['ttxns'])/BILLION
	if how==TYPE_TRX:     subplot.plot( normalize(mReceiverRawData['trxns'])/BILLION ,delay,'r.', label='Packetdelays vs. trx'  ,markersize=2)
	elif how==TYPE_TTX:   subplot.plot( normalize(mReceiverRawData['ttxns'])/BILLION ,delay,'r.', label='Packetdelays vs. ttx'  ,markersize=2)
	elif how==TYPE_CHIRP: subplot.plot(          (mReceiverRawData['Chirp']       )  ,delay,'r.', label='Packetdelays vs. chirp',markersize=2)
	
	subplot.set_ylabel('Packetdelays [ms]', color='r')
	subplot.set_xlabel('Number of the chirp')
	# subplot.annotate('what happened here?', xy=(194.24, 2.1), xytext=(194, 5),
	#             arrowprops=dict(facecolor='black', shrink=0.05))
	subplot.legend(loc=2)
	subplot.grid(True)
	return

def plotSendingRate(subplot,streamdata,title,how=HOW,measurementIntervalMs=200.0, utilization=0.5):
	sendrate = (streamdata['packetsize_Bytes']* ( streamdata['send'] - 1 ) +  126)*1000.0/measurementIntervalMs/utilization
	print sendrate[1019:1023:1]
	if how==TYPE_TRX:     subplot.plot( normalize(streamdata['first_trx_ns'])/BILLION ,sendrate/1000.0,'g', label='Sending rate vs. trx'  ,drawstyle='steps-post')
	elif how==TYPE_TTX:   subplot.plot( normalize(streamdata['first_ttx_ns'])/BILLION ,sendrate/1000.0,'g', label='Sending rate vs. ttx'  ,drawstyle='steps-post')
	elif how==TYPE_CHIRP: subplot.plot(          (streamdata['chirp']       )         ,sendrate/1000.0,'g', label='Sending rate vs. chirp',drawstyle='steps-post')
	#subplot.set_title(title)
	subplot.set_ylabel('Data rate [KByte/s]', color='g')
	subplot.legend(loc=2)
	subplot.grid(True)
	return


def plotNumberOfSendPackets(subplot,streamdata,title, how=HOW):
	if how==TYPE_TRX:     subplot.plot( normalize(streamdata['first_trx_ns'])/BILLION ,streamdata['send'],'g', label='Packets sent vs. trx'  ,drawstyle='steps-post',linewidth=1)
	elif how==TYPE_TTX:   subplot.plot( normalize(streamdata['first_ttx_ns'])/BILLION ,streamdata['send'],'g', label='Packets sent vs. ttx'  ,drawstyle='steps-post',linewidth=1)
	elif how==TYPE_CHIRP: subplot.plot(          (streamdata['chirp']       )         ,streamdata['send'],'g', label='Packets sent vs. chirp',drawstyle='steps-post',linewidth=1)
	#subplot.set_title(title)
	subplot.set_ylabel('Number of packets sent', color='g')
	subplot.set_xlabel('Number of the chirp')
	subplot.legend(loc=1)
	subplot.grid(True)
	return

def plotReceivedDatarateViaBackChannel(subplot,mReceiverRawData,title,how=HOW):
	if how==TYPE_TRX:     subplot.plot( normalize(mReceiverRawData['trxns'])/BILLION ,mReceiverRawData['RateBytes']/1000.0,'b', label='mReceiverRawDatarate in payload vs. trx'  ,drawstyle='steps-post')
	elif how==TYPE_TTX:   subplot.plot( normalize(mReceiverRawData['ttxns'])/BILLION ,mReceiverRawData['RateBytes']/1000.0,'b', label='mReceiverRawDatarate in payload vs. ttx'  ,drawstyle='steps-post')
	elif how==TYPE_CHIRP: subplot.plot(          (mReceiverRawData['bChirp']    )    ,mReceiverRawData['RateBytes']/1000.0,'b', label='Datarate in payload vs. chirp'            ,drawstyle='steps-post')
	#subplot.set_title(title)
	subplot.set_ylabel('Data rate received via backchannel')
	subplot.legend(loc=1)
	subplot.grid(True)
	return

def setXlim(subplots,how=HOW):
	if how==TYPE_TRX:     subplot.plot( normalize(mReceiverRawData['trxns'])/BILLION ,mReceiverRawData['RateBytes']/1000.0,'b', label='mReceiverRawDatarate in payload vs. trx'  ,drawstyle='steps-post')
	elif how==TYPE_TTX:   subplot.plot( normalize(mReceiverRawData['ttxns'])/BILLION ,mReceiverRawData['RateBytes']/1000.0,'b', label='mReceiverRawDatarate in payload vs. ttx'  ,drawstyle='steps-post')
	elif how==TYPE_CHIRP:
		minx = min(streamdata['chirp'])
		maxx = max(streamdata['chirp'])
	for s in sub:
		s.set_xlim( [minx,maxx])

def multiplot(directory,DEBUG=False,RECALC=False,DPI=300):
	logs = bothlogs.BothLogs(directory,DEBUG)
	if RECALC: 
		logs.recalcAllStreamData()

	NORMALIZE = logs.getNormalization() 

	#downstream
	fig, sub = plt.subplots(4, sharex=True)
	fig.tight_layout()   # get smaller borders
	fig.suptitle('Downstream measurement '+logs.directory, fontsize=14, fontweight='bold')
	streamdata = logs.client.getStreamData(logs.server)
	plt.subplots_adjust(left=0.08, right=0.93, top=0.95, bottom=0.05,wspace=0.05,hspace=0.08) #for 10x10 inch
	#plt.subplots_adjust(left=0.06, right=0.95, top=0.96, bottom=0.04,wspace=0.05,hspace=0.07) #fuer 12x12 inch
	#plotReceivedDatarateViaBackChannel(sub[0],logs.server.getmReceiverRawData(),'Downstream data rate via backchannel')
	plotDatarateAndLossrate(sub[0],streamdata , 'Downstream data rate and loss rate')
	#plotSendingRate(sub[0],streamdata,'Downstream sending rate')
	plotDelay(sub[1],streamdata , 'Downstream delay')
	plotTGap(sub[2],streamdata , 'Downstream t_gap_i')
	#plotTtxFirst(sub[2].twinx(),streamdata , 'TTX First')
	plotIndividualPacketDelays(sub[3], logs.client.getmReceiverRawData(),' Downstream individual packet delays')
	plotNumberOfSendPackets(sub[3].twinx(), streamdata,'Downstream send packets')
	fig.savefig("downstream-"+str(DPI)+"DPI.png",dpi=DPI)
	fig.savefig("downstream-"+str(DPI)+"DPI.jpeg",dpi=DPI)
	#fig.savefig("downstream-"+str(int(DPI))+"-"+str(int(size[0]*DPI))+"x"+str(int(size[1]*DPI))+".png",dpi=80,orientation='portrait',papertype='a4',bbox_inches=None,format='png')

	zoom, sub = plt.subplots(1)
	zoom.tight_layout()   # get smaller borders
	plt.subplots_adjust(left=0.08, right=0.93, top=0.92, bottom=0.05,wspace=0.05,hspace=0.08) #for 10x10 inch
	zoom.suptitle('Downstram t_gap_i zoom '+logs.directory, fontsize=14, fontweight='bold')
	zoom.set_size_inches(10, 5)
	plotTGapZoom(sub,streamdata , 'Downstream t_gap_i')
	zoom.savefig("tgap-zoom-downstream-"+str(DPI)+"DPI.png",dpi=DPI)
	zoom.savefig("tgap-zoom-downstream-"+str(DPI)+"DPI.jpeg",dpi=DPI)

	#upstream
	fig2, sub = plt.subplots(4, sharex=True)
	fig2.tight_layout(pad=0.4, w_pad=0.5, h_pad=1.0)   # get smaller borders
	fig2.suptitle('Upstream measurement '+logs.directory, fontsize=14, fontweight='bold')
	streamdata = logs.server.getStreamData(logs.client)
	plt.subplots_adjust(left=0.08, right=0.93, top=0.95, bottom=0.05,wspace=0.05,hspace=0.08) #for 10x10 inch
	#plt.subplots_adjust(left=0.06, right=0.95, top=0.96, bottom=0.04,wspace=0.05,hspace=0.07) #fuer 12x12 inch
	#plotReceivedDatarateViaBackChannel(sub[0],logs.client.getmReceiverRawData(),'Upstream data rate via backchannel')
	plotDatarateAndLossrate(sub[0],streamdata , 'Upstream data rate and loss rate')
	#plotSendingRate(sub[0],streamdata,'Downstream sending rate')
	plotDelay(sub[1],streamdata , 'Upstream delay')
	plotTGap(sub[2],streamdata , 'Upstream t_gap_i')
	#plotTtxFirst(sub[2].twinx(),streamdata , 'TTX First')
	plotIndividualPacketDelays(sub[3], logs.server.getmReceiverRawData(),'Upstream individual packet delays')
	plotNumberOfSendPackets(sub[3].twinx(), streamdata,'Upstream send packets')
	fig2.savefig("upstream-"+str(DPI)+"DPI.png",dpi=DPI)
	fig2.savefig("upstream-"+str(DPI)+"DPI.jpeg",dpi=DPI)
	
	zoom2, sub = plt.subplots(1)
	zoom2.tight_layout()   # get smaller borders
	plt.subplots_adjust(left=0.08, right=0.93, top=0.92, bottom=0.05,wspace=0.05,hspace=0.08) #for 10x10 inch
	zoom2.suptitle('Upstream t_gap_i zoom '+logs.directory, fontsize=14, fontweight='bold')
	zoom2.set_size_inches(10, 5)
	plotTGapZoom(sub,streamdata , 'Upstream t_gap_i')
	zoom2.savefig("tgap-zoom-upstream-"+str(DPI)+"DPI.png",dpi=DPI)
	zoom2.savefig("tgap-zoom-upstream-"+str(DPI)+"DPI.jpeg",dpi=DPI)

	plt.grid()





#define directories for plotting
#
directories = []

#take for paper
# directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury-h5321gw/20150904/215435') # D1 21mbit - great performance but one enourmous sending rate spike (with no impact) in the downstream plot
# directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury-h5321gw/20150907/072020') # D1 21mbit - long and really great (but again a negative 800ms t_gap - I guess my script is wrong somewhere)
# directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury-h5321gw/20150907/075124') # D1 21mbit - short but perfect




# directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury-h5321gw/20150907/072020') # Allerheiligen -> Frdierichstadt - D1 21mbit - long and really great (but again a negative 800ms t_gap - I guess my script is wrong somewhere)
# directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury-h5321gw/20150907/075124') # Fridrichstadt -> Wehrhahn - D1 21mbit - short but perfect
# directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury-h5321gw/20150904/183536') # Allerheiligen -> Uedesheim - D1 21mbit - what the hack -  minus 7s t_gap in downstream? out of order?
# directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury-h5321gw/20150904/184349') # Uedeseim -> Wendersplatz - D1 21mbit - short but perfec
# directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury-h5321gw/20150904/214302') # Wendersplatz -> Europadamm (sehr kurz) - D1 21mbit - seems like perfect rubbish to me - invalid timesync and duplicate packets in dthe downstream?
# directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury-h5321gw/20150904/215435') # Koelner Strasse Neuss ueber Schlicherum und Rosellen nach Allerheiligen - D1 21mbit - great performance but one enourmous sending rate spike (with no impact) in the downstream plot


#both quite good
#directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury-h5321gw/20150903/090013') # Wehrhahn -> Uni (beide Seiten mit Drossel für Emergency SC Nachrichten)
#directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury-h5321gw/20150903/080121') # Münchener Straße -> Wehrhahn. Leider auf dem Laptop noch ohne emergency message Drossel

#mit x200t gegen mercury (21mbit/5.76mbit) leider in der Drossel
#directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury-h5321gw/20150831/085728') # D1 statisch in der Uni - Telekom Drossel
#directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury-h5321gw/20150828/163644') # D1 Taubental nach Allerheiligen (Jetzt sollte der Emergency Drossel Bug gelöst sein, aber das Wiederverbinden klappt nach wie vor nicht.)  - Telekom Drossel
#directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury-h5321gw/20150828/162130') # D1 Uni bis Taubental (mit vermeintlicher Drosselung der Emergency Nachrichten - diese hatte aber einen Bug)  - Telekom Drossel nach einigen Minuten

#mit x200t gegen mercury (21mbit/5.76mbit)
#directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury-h5321gw/20150828/073826') # D1 Uni über Taubental nach Allerheiligen (Negativer gap bei chirp 3805? out of order?)
#directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury-h5321gw/20150825/172937') # D1 Uni über Taubental nach Allerheiligen (Der Abbruch am Ende war mal kein Modemproblem wie es scheint, ich habe weiter Logausgaben zum RMF hinzugefügt um zu sehen von wem ein emergency-stop initiiert wurde)
#directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury-h5321gw/20150825/172937-spielwiese') # D1 Uni über Taubental nach Allerheiligen (Der Abbruch am Ende war mal kein Modemproblem wie es scheint, ich habe weiter Logausgaben zum RMF hinzugefügt um zu sehen von wem ein emergency-stop initiiert wurde)
#directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury-h5321gw/20150825/172155') # D1 Uni stationär auf dem Parkplatz


#mit x200t gegen mercury (7.2mbit/2mbit):
# directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury/20150821/215721') # D1 Taubental -> Allerheiligen
# directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury/20150821/213512') # D1 Taubental (schneller Abbruch durch das Modem)
# directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury/20150821/211837') # D1 Neuss Hafen -> Taubental 
# directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury/20150821/184439') # D1 Allerheiligen -> Neuss Hafen (zweiter Teil nach Verbindungsabbruch dank f3507g - leider auch mit der nachgekauften Karte)
# directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury/20150821/183402') # D1 Allerheiligen -> Neuss Hafen
# directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury/20150821/162222') # D1 Uni -> Allerheiligen 

#directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury/20150807/073810') #
#directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury/20150807/070929') #
#directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury/20150807/070102') #

#vor patch, der auch gescheit die wirklichen IPs der Verbindungen mitloggt und alarm schlägt wenn safeguardchannel und measurement channel identisch sind
# directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury/20150805/072718') #D1 von Allerheiligen zur uni - nur 25KB/s upstream, einige Lossspikes, negative gap?
# directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury/20150803/093139') #
# directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury/20150803/092532') #
# directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury/20150803/091227') #
# directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury/20150803/090007') #
# directories.append(r'/home/goebel/rmf-logs-to-keep/x200t-mercury/20150803/085352') #



#normal

# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150723/170140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150723/140140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150723/110140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150723/080140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150723/050140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150723/020140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel

# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150722/230140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150722/200141') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150722/170140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150722/140140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150722/110140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150722/080140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150722/050139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150722/020139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel

# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150721/230140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150721/200140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150721/170141') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150721/140140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150721/110147') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150721/080139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150721/050140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150721/020140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel

# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150720/230141') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150720/200139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150720/170140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150720/140140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150720/110140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150720/080139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150720/050147') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150720/020140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel

# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150719/230140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150719/200140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150719/170140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150719/142557') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150719/123729') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel - warum so kurz und so spät?
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150719/094537') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel - warum so kurz und so spät?
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150719/050139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150719/020139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel

# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150718/230141') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150718/200139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150718/170139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150718/140139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150718/110140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150718/080140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150718/050140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150718/020140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel

# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150717/230139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150717/200140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150717/170139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150717/140140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150717/110140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150717/080140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150717/050140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150717/020141') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel

# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150716/230140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150716/200140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150716/170141') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150716/140139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150716/110139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150716/080140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150716/050140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150716/020140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150715/230145') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150715/200140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150715/170145') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150715/140139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel - was ist hier passiert? kein upstream und gravierender Downstreamloss?
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150715/110141') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150715/080140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150715/050139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150715/020140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150714/230140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150714/200140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel

# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150714/170140') # D1 Messungen Lokal Uni zu mercury105 - ab Sekunde 470 in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150714/140140') # D1 Messungen Lokal Uni zu mercury105 - gut
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150714/110139') # D1 Messungen Lokal Uni zu mercury105 - gut
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150714/080140') # D1 Messungen Lokal Uni zu mercury105 - gut
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150714/050139') # D1 Messungen Lokal Uni zu mercury105 - gut
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150714/020140') # D1 Messungen Lokal Uni zu mercury105 - gut
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150713/230140') # D1 Messungen Lokal Uni zu mercury105 - was ist hier passiert? massiver packetloss
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150713/200140') # D1 Messungen Lokal Uni zu mercury105 - gut
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150713/170148') # D1 Messungen Lokal Uni zu mercury105 - gut
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150713/140140') # D1 Messungen Lokal Uni zu mercury105 - gut
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150713/110140') # D1 Messungen Lokal Uni zu mercury105 - gut
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150713/080140') # D1 Messungen Lokal Uni zu mercury105 - gut - im Doenstream bei ca. 230 Sekunden detektierter Delayspike, im Upstream bei ca. 430 Sekunden das gleiche
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150713/050140') # D1 Messungen Lokal Uni zu mercury105 - sehr schöne Punktewolken um
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150713/020140') # D1 Messungen Lokal Uni zu mercury105 - gut

# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150712/230140') # D1 Messungen Lokal Uni zu mercury105 - gut
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150712/200146') # D1 Messungen Lokal Uni zu mercury105 - gut
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150712/170140') # D1 Messungen Lokal Uni zu mercury105 - gut
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150712/140140') # D1 Messungen Lokal Uni zu mercury105 - gut, einige wenige 0ms tgaps
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150712/110141') # D1 Messungen Lokal Uni zu mercury105 - gut

# #ne Menge in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150712/080140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150712/050139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150712/020147') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel

# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150711/230145') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150711/200148') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150711/170141') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150711/140140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150711/110140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150711/080140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150711/050140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150711/020139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel

# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150710/230139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150710/200140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150710/170140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150710/140140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150710/110140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150710/080139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150710/050140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150710/020140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel

# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150709/230140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150709/200140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150709/170140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150709/140140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150709/110139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150709/080139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150709/050145') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150709/020140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel

# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150708/230140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150708/200139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150708/170139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150708/140139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150708/110139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150708/080140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150708/050140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150708/020140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel

# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150707/230140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150707/200140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150707/170140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150707/140140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150707/110139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150707/080139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150707/050140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150707/020142') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel


# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150706/230140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150706/200139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150706/170139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150706/140140') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150706/110139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150706/080139') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150706/050138') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150706/020145') # D1 Messungen Lokal Uni zu mercury105 - in der Drossel

# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150705/230139') # D1 Messungen Lokal Uni zu mercury105 - ab Sekunde 360 in der Datenratendrossel
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150705/200140') # D1 Messungen Lokal Uni zu mercury105 - klasse, sehr konstant
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150705/170140') # D1 Messungen Lokal Uni zu mercury105 - klasse, sehr konstant
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150705/140140') # D1 Messungen Lokal Uni zu mercury105 - ok, minor spikes in upstream
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150705/110140') # D1 Messungen Lokal Uni zu mercury105 - nur 180 sekunden, warum? ansonsten gut
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150705/080140') # D1 Messungen Lokal Uni zu mercury105 - gut, im Downstream aber immer wieder Einbrüche von 700 auf 400 kByte/s
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150705/050140') # D1 Messungen Lokal Uni zu mercury105 - super 
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150705/020140') # D1 Messungen Lokal Uni zu mercury105 - super 

# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150704/230140') # D1 Messungen Lokal Uni zu mercury105 - mehr Schwankungen in der DR - SP Saal Party?
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150704/200140') # D1 Messungen Lokal Uni zu mercury105 - gut, ziemlich konstante DR, Spike bei ca. 460s - woher?
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150704/170140') # D1 Messungen Lokal Uni zu mercury105 - gut, konstantere Downstream DR als um 14 Uhr
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150704/140139') # D1 Messungen Lokal Uni zu mercury105 - gut, deutlich mehr Downstream DR Einbrüche als um 2 Uhr Nachts
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150704/020111') # D1 Messungen Lokal Uni zu mercury105 - klasse, kaum Schwankungen im Dowstream, mehr Schwknungen im Upstream - woher der geringe Delay?


# #unbrauchbar:
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury-statisch-uni/20150704/000511') # D1 Messungen Lokal Uni zu mercury105 - only 16s and with high packet loss downstream and upstream


# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury/20150703/082534') # D1 Benrather Schloss und Umgebung - massiver Loss in beide Richtungen nach ca. 2400s - was zum Henker ist da passiert?
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury/20150703/075337') # D1 zur Uni 
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-mercury/20150702/171924') # D1 in der Uni - First measurement agains mercury105
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150702/081855') # D1 zur Uni - quite good
# directories.append(r'/home/goebel/rmf-logs-to-keep/t530lan-kamps/20150701/171854') # UNI-LAN-KAMP mit settings für Unitymedia und ohne gescheite zeitsync auf dem t530
# directories.append(r'/home/goebel/rmf-logs-to-keep/t530lan-kamps/20150701/172209') # UNI-LAN-KAMP mit settings für Unitymedia und ohne gescheite zeitsync auf dem t530
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150701/091608') # VF working flawlessly
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150629/165644') # VF without any problems
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150629/172948') # VF massive loss in both directions
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150630/081109') # VF loss on upstream
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150629/074717') # D1 - all good but traffic limit at the end of the measurement
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150626/174020') # D1 - massive packet loss in upstream
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150626/085518') # D1 - working flawlessly - no sic through to high sending rate by ourselve anymore
# 
# all measurements earlier than 20150626 suffer from possible excessive sending rates in the upstream caused by a pogramming error
# also the #packets/chirp start with 20 packets and only rise if 20 MTU packets do not suffice
# 
# zeitweise zu Hohe Senderaten im Upstream (Programmierfehler) 
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150619/221143') # D1 - excessive sensing rates causeing icsic of 12 and 14s in the upstream - sic detection working well everywhere else
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150619/182357') # D1 - all well except excessive sensing rates causeing icsic in the upstream - sic detection working really good
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150619/064105') # D1 - all weell except excessive sensing rates causeing icsic in the upstream 
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150618/082259') # D1 - all weell except excessive sensing rates causeing some short loss spikes in the upstream
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150617/080513') # D1 - all well except excessive sending rates causing loss spikes and icsic in the upstream
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150616/103403') # D1 - massive losses in up- and donwsream starting in the mddle of the measurement run and negative delay at the end!
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150616/093334') # D1 - mostly well except excessive sending rates causing loss spikes and icsic in the upstream - also some loss spikes in the downstream
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150611/080304') # D1 - all well except excessive sending rates causing loss spikes and icsic in the upstream
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150610/081751') # D1 - Jede Menge out of order Pakete im Upstream, keine Ahnung was t-mobile oder das ZIM hier angestellt haben.
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150609/075436') # D1 - all weell except excessive sensing rates causeing some short loss spikes in the upstream
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150608/080325') # D1 - all weell except excessive sensing rates causeing some loss spikes and icsic of up to 16s in the upstream
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150608/072115') # D1 - all weell except excessive sensing rates causeing many loss spikes and small icsic in the upstream
# 
# measurements conducted on 20150605 evaluate different measurement intervalls showing that 200ms is much better than 1s
# 
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150605/163409') # D1 - 200ms intervall and no loss in upstream

# directories.append(r'/home/goebel/rmf-logs-to-keep/t530lan/20150610/001736' )
# directories.append(r'/home/goebel/rmf/results/mTBUS/192.168.10.50_to_134.99.147.228/20150609-233421')
# directories.append(r'/home/goebel/rmf/results/mTBUS/192.168.2.184_to_134.99.147.228/20150609-175154')
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150527/083805')

# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150602/072522')
# directories.append(r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150602/093359')

#plt.ion() #enable interactive mode
#plt.xkcd()

def usage():
	print "test.py -i <inputdir> [-d -r]"
	print "        -i inputdirectory with rmf logs (can be used multiple times)"
	print "        -r recalc everything"
	print "        -d use default directory as input"
	print "        -x do not show plots"

def main(argv):
	directories=[]
	recalcall = False
	usedefault = False
	showplots = True
	extension = "jpeg"

	try:
		opts, args = getopt.getopt(argv,"hi:drx",["inputdir="])
	except getopt.GetoptError:
		usage()
		sys.exit(2)
	for opt, arg in opts:
		if opt == "-h":
			usage()
			sys.exit()
		elif opt in ("-i", "--inputdir"):
			directories.append(arg)
		elif opt == "-r":
			recalcall = True
		elif opt == "-d":
			usedefault = True
		elif opt == "-x":
			showplots = False
	if usedefault:
		directories=[]
		os.chdir('/home/goebel/rmf-logs-to-keep')
		directories.append(r'x200t-mercury-h5321gw/20150907/075124');
		directories.append(r'x200t-mercury-h5321gw/20150904/183536');
		directories.append(r'x200t-mercury-h5321gw/20150904/215435');
		directories.append(r'x200t-mercury-h5321gw/20150904/184349');
		directories.append(r'x200t-mercury-h5321gw/20150903/090013');
		directories.append(r'x200t-mercury-h5321gw/20150903/080121');
		directories.append(r'x200t-mercury-h5321gw/20150828/073826');
		directories.append(r'x200t-mercury-h5321gw/20150825/172937');
		directories.append(r'x200t-mercury-h5321gw/20150907/072020');

	print "Inputdirectories = ", directories
	print "Recalcall = ",recalcall
	print "usedfault = ", usedefault
	print "\n"

	for directory in directories: 
		print 'Generating plot for dir = '+directory
		pwd = os.getcwd()
		multiplot(directory,False,recalcall,1000)
		os.chdir(pwd)
		plt.cla

	#plt.ioff()
	if showplots:
		plt.show()
	print "Finished"
if __name__ == "__main__":
	main(sys.argv[1:])





