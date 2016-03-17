import serial
import time

# ser = serial.Serial(port='/dev/ttyACM0', baudrate=115200, btesize=8, parity='N', stopbits=1, timeout=1, xonoff=False, rtscts=False, dsrdtr=False)
ser = serial.Serial('/dev/ttyACM0', 115200, timeout=0.1)
cmd = 'AT*E2EMM=9\r\n'
cmd_encoded = cmd.encode()

SERVING="WCDMA - Serving Cell ( Idle )\r\n"
SERVING_HEADER="MCC, MNC,  LAC,   Ch,  SC, RSCP, EcNo, RSSI, ServL, ServQ, Hs, Rs"
SERVING_NAME='serving.csv'
INTRA="Monitored Intra-Frequency Cells\r\n"
INTRA_HEADER="Ch,  SC, RSCP, EcNo, PTxPwr, ULTxPwr, ServL, ServQ, Hn, Rn"
INTRA_NAME='intra.csv'
INTER="Monitored Inter-Frequency Cells\r\n"
INTER_HEADER="Ch,  SC, RSCP, EcNo, PTxPwr, ULTxPwr, ServL, ServQ, Hn, Rn"
INTER_NAME='inter.csv'
RAT="Monitored Inter-RAT Cells\r\n"
RAT_HEADER="Ch, BSIC,  CIO,  RSSI, RxLevMin"
RAT_NAME='rat.csv'
DETECTED="Detected Cells\r\n"
DETECTED_HEADER="Ch,  SC, PTxPwr, ULTxPwr, ServL, ServQ,  Hn, Rn"
DETECTED_NAME='detected.csv'

def openFiles():
	global serving_file, intra_file, inter_file, rat_file, detected_file

	serving_file = open(SERVING_NAME,'w')
	serving_file.write('time [ms]    , '+SERVING_HEADER+'\n')
	intra_file = open(INTRA_NAME,'w')
	intra_file.write('time [ms]    , '+INTRA_HEADER+'\n')
	inter_file = open(INTER_NAME,'w')
	inter_file.write('time [ms]    , '+INTER_HEADER+'\n')
	rat_file = open(RAT_NAME,'w')
	rat_file.write('time [ms]    , '+RAT_HEADER+'\n')
	detected_file = open(DETECTED_NAME,'w')
	detected_file.write('time [ms]    , '+DETECTED_HEADER+'\n')

def closeFiles():
	global serving_file, intra_file, inter_file, rat_file, detected_file

	serving_file.close()
	intra_file.close()
	inter_file.close()
	rat_file.close()
	detected_file.close()

def parseAndStore(timestamp,file,header,printit=False):
	global ser

	resp = ser.readline()

	while True: #len(resp)!=2:
		if len(resp) == 0 or len(resp) == 2:
			break
		elif header not in resp:
			file.write(str(timestamp))
			file.write (','+resp.rstrip('\r\n')+'\n')
			if printit: print resp.rstrip('\r\n')
		resp = ser.readline()
	file.flush()
	return

openFiles()

for y in xrange(1,1200):
	ser.write(cmd_encoded)
	ms = int(round(time.time()*1000.0)) # get milliseconds
	for x in xrange(1,1000):
		resp = ser.readline()
		if len(resp) == 0:
			break
		elif resp == SERVING:
			parseAndStore(ms,serving_file,SERVING_HEADER,True),
		elif resp == INTER:
			parseAndStore(ms,inter_file,INTER_HEADER)
		elif resp== INTRA:
			parseAndStore(ms,intra_file,INTRA_HEADER)
		elif RAT == resp:
			parseAndStore(ms,rat_file,RAT_HEADER)
		elif DETECTED == resp:
			parseAndStore(ms,detected_file,DETECTED_HEADER)
		elif "OK\r\n" == resp:
			pass
			#print "OK Detected"
		elif len(resp) ==2 or "AT*E2EMM=9" in resp or resp == "*E2EMM\r\n":
			pass
		else:
			print resp
	time.sleep(0.9)

closeFiles()

			#"OK\r\n" : break
		#}[resp]
	# if resp =="OK\r\n":
	# 	print "OK detected"
	# elif resp == ""
	# elif  len(resp) >0 :
	# 	print len(resp),resp


# ser.write(cmd_encoded)
# resp = ser.read(5000)
# print ms,resp
# if "WCDMA - Serving0 in resp:
# millile resp!="":
# 		resp = ser.readline()
# 		print resp
# print "New Line detected"
# while resp !="OK": 
# 	resp = ser.readline()
ser.close()
