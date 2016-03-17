import serial
import time
import os.path

SERIAL_DEVICE='/dev/ttyACM1'
LOGFILE='/tmp/cellinfo.txt'

gsm_cmds =[]
gsm_cmds.append('AT+CREG?\r\n'.encode())
gsm_cmds.append('AT*EGSCI\r\n'.encode())
gsm_cmds.append('AT*EGNCI\r\n'.encode())

wcdma_cmds = []
wcdma_cmds.append('AT*EWSCI\r\n'.encode())
wcdma_cmds.append('AT*EWNCI\r\n'.encode())
wcdma_cmds.append('AT+CSQ\r\n'.encode())

init_cmds = []
init_cmds.append('AT*EEVINFO\r\n'.encode())
init_cmds.append('AT+GMM\r\n'.encode())
init_cmds.append('AT+GMR\r\n'.encode())
init_cmds.append('AT+CREG=2\r\n'.encode())
init_cmds.append('AT+CREG?\r\n'.encode())
init_cmds.append('AT+GMI\r\n'.encode())
init_cmds.append('AT+CFUN=1\r\n'.encode()) #1=foce automode CFUN=5 GSM CFUN=6 wcdma

def log(message):
	global raw
	ms = int(round(time.time()*1000.0)) # get milliseconds
	raw.write('################################################\r\n')
	raw.write(str(ms)+'\r\n')
	raw.write(message)
	raw.flush()
	# print message


tstamp = int(round(time.time()))
raw = open(LOGFILE,'w')
gsm = True

while True:
	if (not os.path.exists(SERIAL_DEVICE)):
		log("Serial device "+SERIAL_DEVICE+" no found.\r\n")
		time.sleep(1)
	else:
		try:
			ser = serial.Serial(SERIAL_DEVICE, 115200, timeout=0.5)
			for cmd in init_cmds:
				ser.write(cmd)
				resp = ser.read(5000)
				log(resp)

			ser.timeout=0.02
			#ser.interCharTimeout=0.00001 does not have any influence at all
			while True:
				if gsm:
					for cmd in gsm_cmds:
						ser.write(cmd)
						resp = ser.read(5000)
						log(resp)
						if resp.endswith("ERROR\r\n"):
							gsm=False
							break
				if not gsm:
					for cmd in wcdma_cmds:
						ser.write(cmd)
						resp = ser.read(5000)
						log(resp)
						if resp.endswith("ERROR\r\n"):
							gsm=True
							break
		except Exception as e:
			log("I/O error({0}): {1}\r\n".format(e.errno, e.strerror))
			try:
				ser.close()
			except Exception as e:
				pass
			time.sleep(1)

raw.close()
ser.close()
