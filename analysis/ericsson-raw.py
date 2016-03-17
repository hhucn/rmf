import serial
import time
import os.path

SERIAL_DEVICE='/dev/ttyACM0'
LOGFILE='/tmp/cellinfo.txt'

cmd = 'AT*E2EMM=9\r\n' 
cmd_encoded = cmd.encode()



def log(message):
	global raw
	ms = int(round(time.time()*1000.0)) # get milliseconds
	raw.write('################################################\r\n')
	raw.write(str(ms)+'\r\n')
	raw.write(message)
	raw.flush()


tstamp = int(round(time.time()))
raw = open(LOGFILE,'w')

while True:
	if (not os.path.exists(SERIAL_DEVICE)):
		log("Serial device "+SERIAL_DEVICE+" no found.\r\n")
		time.sleep(1)
	else:
		try:
			ser = serial.Serial(SERIAL_DEVICE, 115200, timeout=0.15)
			for y in xrange(1,1200):
				ser.write(cmd_encoded)
				ms = int(round(time.time()*1000.0)) # get milliseconds
				resp = ser.read(5000)
				log(resp)
				# raw.write('################################################\r\n')
				# raw.write(str(ms)+'\r\n')
				# raw.write(resp)
				# raw.flush()
		except Exception as e:
			log("I/O error({0}): {1}\r\n".format(e.errno, e.strerror))
			# ms = int(round(time.time()*1000.0)) # get milliseconds
			# raw.write('################################################\r\n')
			# raw.write(str(ms)+'\r\n')
			# raw.write("I/O error({0}): {1}".format(e.errno, e.strerror))
			# print "I/O error({0}): {1}".format(e.errno, e.strerror)
			try:
				ser.close()
			except Exception as e:
				pass
			time.sleep(1)
	    	#raw.write('Serial connection error'+error))

raw.close()
ser.close()
