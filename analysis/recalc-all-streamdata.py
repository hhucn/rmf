#!/usr/bin/env python
# -*- coding: utf-8 -*-
# 
import bothlogs

directories = [	r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150619/221143',
				r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150619/182357',
				r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150619/064105',
				r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150618/082259',
				r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150617/080513',
				r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150616/103403',
				r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150616/093334',
				r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150611/080304',
				r'/home/goebel/rmf-logs-to-keep/alix1-strongrom/20150610/081751' ]
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
	temp = bothlogs.BothLogs(directory,True)
	temp.recalcAllStreamData()

print 'Finished calculation'