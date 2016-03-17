#!/usr/bin/env python
# -*- coding: utf-8 -*-

import bothlogs

import matplotlib.pyplot as plt

NORMALIZE = 0
BILLION = 1000000000.0
MILLION = 1000000.0
DEBUG = False
TRX = True

def normalize(what):
	return (what - NORMALIZE)

def plottop(subplot,direction,title):
	#create the top most plot
	if TRX:
		subplot.plot( (direction.mReceiverRawData['trxns'] - NORMALIZE)/BILLION, (direction.mReceiverRawData['trxns'] - direction.mReceiverRawData['ttxns']) / BILLION,'r+-', label='Delay vs. trx')
	else: 
		subplot.plot( (direction.mReceiverRawData['ttxns'] - NORMALIZE)/BILLION, (direction.mReceiverRawData['trxns'] - direction.mReceiverRawData['ttxns']) / BILLION,'r+-', label='Delay vs. ttx')
	subplot.set_title(title)
	subplot.set_ylabel('Packetdelays [s]')
	subplot.annotate('what happened here?', xy=(194.24, 2.1), xytext=(194, 5),
	            arrowprops=dict(facecolor='black', shrink=0.05))
	subplot.legend(loc=2)
	return

def plotmiddle(subplot,direction):
	if TRX:
		subplot.plot( normalize(direction.mReceiverDatarate['trxns'])/BILLION, direction.mReceiverDatarate['RateBytes']/1000, label='Rate vs. trx')
	else:
		subplot.plot( normalize(direction.mReceiverDatarate['ttxns'])/BILLION, direction.mReceiverDatarate['RateBytes']/1000, label='Rate vs. ttx')
	subplot.step( normalize(direction.sSenderData['ttx_dynPayns'])/BILLION , direction.sSenderData['RateBytes']/1000 , label='Rate send via sc vs. sc_ttx')
	subplot.set_ylabel('DataRate [KByte/s]')
	subplot.legend(loc=2)

	righty = subplot.twinx()
	righty.set_ylabel('Lossrate [%]', color='r')
	if TRX:
		righty.step( normalize(direction.mReceiverDatarate['trxns'])/BILLION, direction.mReceiverDatarate['loss'],'yx', label='Loss vs. trx')
	else: 
		righty.step( normalize(direction.mReceiverDatarate['ttxns'])/BILLION, direction.mReceiverDatarate['loss'],'r+', label='Loss vs. ttx')
	righty.step( normalize(direction.sSenderData['ttx_dynPayns'])/BILLION , direction.sSenderData['Loss']/1000 , label='Loss send via sc vs. sc_ttx')
	righty.set_ylim([0,100])
	righty.legend(loc=1)
	return

def plotbottom(subplot,direction,opposite_direction):
	if TRX:
		subplot.plot( normalize(direction.mReceiverGaps['trxns'])/BILLION , direction.mReceiverGaps['t_gap_ins']/MILLION,'g+ ', label='t_gap(i) vs trx')
		subplot.plot( normalize(direction.mReceiverGaps['trxns'])/BILLION , direction.mReceiverGaps['deltatrxns']/MILLION,'b+ ', label='Delat_trx(i) vs trx')
	else:
		subplot.plot( normalize(direction.mReceiverGaps['ttxns'])/BILLION , direction.mReceiverGaps['t_gap_ins']/MILLION,'y+ ', label='t_gap(i) vs ttx')
		subplot.plot( normalize(direction.mReceiverGaps['ttxns'])/BILLION , direction.mReceiverGaps['deltatrxns']/MILLION,'r+ ', label='Delat_trx(i) vs ttx')
	subplot.set_ylabel('Gap or Delta_trx [ms]')
	subplot.legend(loc=2)
	subplot.set_xlabel('Time [s]')


	righty = subplot.twinx()
	righty.set_ylabel('SC Delay [ms]', color='r')
	print opposite_direction.sSenderData.dtype.names
	righty.step( normalize(opposite_direction.sSenderData['ttx_dynPayns'])/BILLION, opposite_direction.sSenderData['SC_OWDns']/MILLION,'k+ ', label='safeguard delay')
	righty.legend(loc=1)
	return

def plotdirection(direction,logs):
	if direction=="upstream":
		# create a plotset of 3 plots sharing the same x-axis
		print "Creating an upstream plot for "+logs.directory
		figup, upplots = plt.subplots(3, sharex=True)
		figup.tight_layout()   # get smaller borders

		plottop(upplots[0],logs.server,'upstream  '+logs.directory)
		plotmiddle(upplots[1],logs.server)
		plotbottom(upplots[2],logs.server,logs.client)

		plt.subplots_adjust(left=0.05, right=0.95, top=0.97, bottom=0.04,wspace=0.05,hspace=0.04)
	else:
		print "Creating an downstream plot for "+logs.directory
		figdown, downplots = plt.subplots(3, sharex=True)
		figdown.tight_layout()   # get smaller borders

		plottop(downplots[0],logs.client,' downstream  '+logs.directory)
		plotmiddle(downplots[1],logs.client)
		plotbottom(downplots[2],logs.client,logs.server)

		plt.subplots_adjust(left=0.05, right=0.95, top=0.97, bottom=0.04,wspace=0.05,hspace=0.04)
	return


def plotall(directory):
	logs = bothlogs.BothLogs(directory)
	NORMALIZE = logs.client.mSenderRawData['ttxns'][0]
	#NORMALIZE = min(logs.client.mSenderRawData['ttxns'][0],logs.server.mSenderRawData['ttxns'][0])
	print "Normalized by "+str(NORMALIZE)
	plotdirection('upstream',logs)
	plotdirection('downstream',logs)
	return

directories = [ r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150619/064105' ]
#directories = [ r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150615/172315', 
# directories = [			  	r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150616/093334',
#			  	r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150616/103403' ]
#directories = [ r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150611/080304' ]
#directories = [ r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150610/081751' ]
# directories = [ r'/home/goebel/rmf-logs-to-keep/t530lan/20150610/001736' ,
# 				r'/home/goebel/rmf-logs-to-keep/t530lan/20150610/003441',
# 				r'/home/goebel/rmf-logs-to-keep/t530lan/20150610/004518']
#directories = [ r'/home/goebel/rmf/results/mTBUS/192.168.10.50_to_134.99.147.228/20150609-233421' ]
#directories = [ r'/home/goebel/rmf/results/mTBUS/192.168.2.184_to_134.99.147.228/20150609-175154' ]
#directories = [ r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150609/075436' ]
				# r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150608/072115',
				# r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150608/080325' ]

# directories = [ r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150527/083805' , 
# 				r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150602/072522',
# 				r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150602/093359' ]	

for directory in directories: 
	print 'Generating plot for dir = '+directory
	plotall(directory)

plt.show()
print "Finished"


# logs = bothlogs.BothLogs(r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150527/083805')
# #logs = bothlogs.BothLogs(r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150527/172219')


# #store the smallest timestamp in all the logs to normalize the output
# NORMALIZE= 0
# #NORMALIZE = min(logs.client.mSenderRawData['ttxns'][0],logs.server.mSenderRawData['ttxns'][0])
# NORMALIZE = logs.client.mSenderRawData['ttxns'][0]
# #if server_mSenderRawData['ttxns'][0] < NORMALIZE:
# #	NORMALIZE = server_mSenderRawData['ttxns'][0]
# print "Normalized by "+str(NORMALIZE)

# plotdirection('upstream',logs)
# plotdirection('downstream',logs)
# plt.show()
# print "Finished"






# #create the top most plot
# if TRX:
# 	sub[0].plot( (logs.client.mReceiverRawData['trxns'] - NORMALIZE)/BILLION, (logs.client.mReceiverRawData['trxns'] - logs.client.mReceiverRawData['ttxns']) / BILLION,'r+-', label='Delay vs. trx')
# else: 
# 	sub[0].plot( (logs.client.mReceiverRawData['ttxns'] - NORMALIZE)/BILLION, (logs.client.mReceiverRawData['trxns'] - logs.client.mReceiverRawData['ttxns']) / BILLION,'r+-', label='Delay vs. ttx')
# sub[0].set_title('Downstream')
# sub[0].set_ylabel('Packetdelays [s]')
# sub[0].annotate('what happened here?', xy=(194.24, 2.1), xytext=(194, 5),
#             arrowprops=dict(facecolor='black', shrink=0.05))
# sub[0].legend(loc=2)

# # sub0_y2 = sub[0].twinx()
# # sub0_y2.set_ylabel('Packetdelays[s] vs. trx', color='r')
# # sub0_y2.plot(client_mReceiverRawData_NormalizedTrx , client_mReceiverRawData_Packetdelays,'r+-')
# # sub0_y2.legend(loc=1)

#create the second plot
#sub[1].set_title(r'$\sum_{i=0}^\infty x_i$')
# if TRX:
# 	sub[1].plot( (logs.client.mReceiverDatarate['trxns'] - NORMALIZE)/BILLION, logs.client.mReceiverDatarate['RateBytes']/1000, label='Rate vs. trx')
# else:
# 	sub[1].plot( (logs.client.mReceiverDatarate['ttrx'] - NORMALIZE)/BILLION, logs.client.mReceiverDatarate['RateBytes']/1000, label='Rate vs. trx')
# 	sub[1].plot(client_mReceiverDatarate_normalizedTtx, client_mReceiverDatarate['RateBytes']/1000, label='Rate vs. ttx')
# sub[1].step( (client_sSenderData['ttx_dynPayns']-NORMALIZE)/BILLION , client_sSenderData['RateBytes']/1000 , label='Rate send via sc vs. sc_ttx')
# sub[1].set_ylabel('DataRate [KByte/s]')
# sub[1].legend(loc=2)

# sub1_y2 = sub[1].twinx()
# sub1_y2.set_ylabel('Lossrate [%]', color='r')
# if TRX:
# 	sub1_y2.step(client_mReceiverDatarate_normalizedTrx, client_mReceiverDatarate['loss'],'yx', label='Loss vs. trx')
# else: 
# 	sub1_y2.step(client_mReceiverDatarate_normalizedTtx, client_mReceiverDatarate['loss'],'r+', label='Loss vs. ttx')
# sub1_y2.step((client_sSenderData['ttx_dynPayns']-NORMALIZE)/BILLION , client_sSenderData['Loss']/1000 , label='Loss send via sc vs. sc_ttx')
# sub1_y2.set_ylim([0,100])
# sub1_y2.legend(loc=1)


# #create the third plot
# if TRX:
# 	sub[2].plot((client_mReceiverGaps['trxns']-NORMALIZE)/BILLION , client_mReceiverGaps['t_gap_ins'],'g+ ', label='t_gap(i) vs trx')
# 	sub[2].plot((client_mReceiverGaps['trxns']-NORMALIZE)/BILLION , client_mReceiverGaps['deltatrxns'],'b+ ', label='Delat_trx(i) vs trx')
# else:
# 	sub[2].plot((client_mReceiverGaps['ttxns']-NORMALIZE)/BILLION , client_mReceiverGaps['t_gap_ins'],'y+ ', label='t_gap(i) vs ttx')
# 	sub[2].plot((client_mReceiverGaps['ttxns']-NORMALIZE)/BILLION , client_mReceiverGaps['deltatrxns'],'r+ ', label='Delat_trx(i) vs ttx')
# sub[2].set_ylabel('Gap or Delta_trx [s]')
# sub[2].legend(loc=2)
# sub[2].set_xlabel('Time [s]')


# sub2_y2 = sub[2].twinx()
# sub2_y2.set_ylabel('SC Delay', color='r')
# print server_sSenderData.dtype.names
# sub2_y2.step((server_sSenderData['ttx_dynPayns']-NORMALIZE)/BILLION, server_sSenderData['SC_OWDns'],'k+ ', label='safeguard delay')
# sub2_y2.legend(loc=1)


