# Makefile for RateMeasurementFramework
# ---------
#CXXFLAGS =	-O0 -g -Wall -pthread
#CXXFLAGS =	

ARCH := $(shell getconf LONG_BIT)
	
#CXXERROR := -Werror
CXXERROR := 
CXXFLAGS_32 = -m32 -O0 -g -std=c++0x -Wall -Wno-unused-local-typedefs -Wshadow ${CXXERROR} -fmessage-length=0 -pthread  #for 32 Bit cross compiling
CXXFLAGS_64 = -O0 -g -std=c++0x -Wall -Wno-unused-local-typedefs -Wshadow ${CXXERROR} -fmessage-length=0 -pthread # -lprofiler -ltcmalloc -pg		  #for 64 Bit compiling on a 64 Bit System

CXXFLAGS = $(CXXFLAGS_$(ARCH))  

#enable GPERF to allow google profiling
GPERF= -ltcmalloc
#GPERF= -lprofiler

# ---------
SRC =		source
BIN = 		binary
OBJ =		objects
LOG_P =		/opt/pantheios
LOG_S =		/opt/stlsoft
#PGCC_64 =   gcc46.file64bit #fill your gcc version used to compile Phanteios whith (maximum supported is gcc 4.6!)
#PGCC_32 =   gcc46 #fill your gcc version used to compile Phanteios whith (maximum supported is gcc 4.6!)
PGCC_64 =   gcc48.file64bit #fill your gcc version used to compile Phanteios whith (maximum supported is gcc 4.6!)
PGCC_32 =   gcc48 #fill your gcc version used to compile Phanteios whith (maximum supported is gcc 4.6!)
PGCC =		$(PGCC_$(ARCH))
		
#GPP =		g++-4.6  ##fill your gcc version into this
GPP =		g++-4.8  ##fill your gcc version into this

# ---------
OBJS =	$(OBJ)/rmf.o\
	$(OBJ)/kernelQueueControl/sendWithEmptyQueue.o\
	$(OBJ)/inputHandler.o\
	$(OBJ)/networkHandler.o\
	$(OBJ)/networkThreadStructs.o\
	$(OBJ)/routingHandler.o\
	$(OBJ)/timeHandler.o\
	$(OBJ)/systemOptimization.o\
	$(OBJ)/measurementMethods/measureTBUS.o\
	$(OBJ)/measurementMethods/measurementMethodClass.o\
	$(OBJ)/measurementMethods/TBUS/TBUS_FastDatarateCalculator.o\
	$(OBJ)/packetPayloads/TBUSFixedPayload.o\
	$(OBJ)/packetPayloads/TBUSDynamicPayload.o\
	$(OBJ)/pthreadwrapper.o\
	$(OBJ)/packets.o
	
	
# ---------
INCLUDES =	-I $(LOG_P)/include -I $(LOG_S)/include
# ---------
LIBS =		-L$(LOG_P)/lib  -lpantheios.1.core.$(PGCC)\
							-lpantheios.1.be.lrsplit.$(PGCC)\
							-lpantheios.1.bel.fprintf.$(PGCC)\
							-lpantheios.1.ber.file.$(PGCC)\
							-lpantheios.1.bec.file.$(PGCC)\
							-lpantheios.1.bec.fprintf.$(PGCC)\
							-lpantheios.1.fe.all.$(PGCC)\
							-lpantheios.1.util.$(PGCC)
# ---------
TARGET =	$(BIN)/rmf
TESTING =	$(BIN)/polytest
# ---------

#don't move PGCC= and CXXFLAGS= to the all line as the fucked up shell makes "64     " out of the shell call at line 4
all:	$(OBJ) $(TARGET)


$(OBJ)/inputHandler.o: $(SRC)/inputHandler.cpp	
	$(GPP) $(CXXFLAGS) -c $(SRC)/inputHandler.cpp -o $(OBJ)/inputHandler.o $(INCLUDES) -lrt
	
$(OBJ)/timeHandler.o: $(SRC)/timeHandler.cpp	
	$(GPP) $(CXXFLAGS) -c $(SRC)/timeHandler.cpp -o $(OBJ)/timeHandler.o $(INCLUDES) -lrt
	
$(OBJ)/pthreadwrapper.o: $(SRC)/pthreadwrapper.cpp	
	$(GPP) $(CXXFLAGS) -c $(SRC)/pthreadwrapper.cpp -o $(OBJ)/pthreadwrapper.o $(INCLUDES) -lrt


$(OBJ)/systemOptimization.o: $(SRC)/systemOptimization.cpp		
	$(GPP) $(CXXFLAGS) -c $(SRC)/systemOptimization.cpp -o $(OBJ)/systemOptimization.o $(INCLUDES) -lrt

$(OBJ)/measurementMethods/measurementMethodClass.o: $(SRC)/measurementMethods/measurementMethodClass.cpp 
	$(GPP) $(CXXFLAGS) -c $(SRC)/measurementMethods/measurementMethodClass.cpp -o $(OBJ)/measurementMethods/measurementMethodClass.o $(INCLUDES) -lrt
	
$(OBJ)/measurementMethods/measureTBUS.o: $(SRC)/measurementMethods/measureTBUS.cpp $(OBJ)/measurementMethods/TBUS/TBUS_FastDatarateCalculator.o $(OBJ)/packetPayloads/TBUSFixedPayload.o	$(OBJ)/packetPayloads/TBUSDynamicPayload.o $(OBJ)/measurementMethods/measurementMethodClass.o  	
	$(GPP) $(CXXFLAGS) -c $(SRC)/measurementMethods/measureTBUS.cpp -o $(OBJ)/measurementMethods/measureTBUS.o $(INCLUDES) -lrt
	
$(OBJ)/measurementMethods/TBUS/TBUS_FastDatarateCalculator.o: $(SRC)/measurementMethods/TBUS/TBUS_FastDatarateCalculator.cpp		
	$(GPP) $(CXXFLAGS) -c $(SRC)/measurementMethods/TBUS/TBUS_FastDatarateCalculator.cpp -o $(OBJ)/measurementMethods/TBUS/TBUS_FastDatarateCalculator.o $(INCLUDES) -lrt
	
$(OBJ)/packetPayloads/TBUSFixedPayload.o: $(SRC)/packetPayloads/TBUSFixedPayload.cpp		
	$(GPP) $(CXXFLAGS) -c $(SRC)/packetPayloads/TBUSFixedPayload.cpp -o $(OBJ)/packetPayloads/TBUSFixedPayload.o $(INCLUDES) -lrt	
	
$(OBJ)/packetPayloads/TBUSDynamicPayload.o: $(SRC)/packetPayloads/TBUSDynamicPayload.cpp		
	$(GPP) $(CXXFLAGS) -c $(SRC)/packetPayloads/TBUSDynamicPayload.cpp -o $(OBJ)/packetPayloads/TBUSDynamicPayload.o $(INCLUDES) -lrt	
	
$(OBJ)/networkHandler.o: $(SRC)/networkHandler.cpp		
	$(GPP) $(CXXFLAGS) -c $(SRC)/networkHandler.cpp -o $(OBJ)/networkHandler.o $(INCLUDES) -lrt
	
$(OBJ)/networkThreadStructs.o: $(SRC)/networkThreadStructs.cpp		
	$(GPP) $(CXXFLAGS) -c $(SRC)/networkThreadStructs.cpp -o $(OBJ)/networkThreadStructs.o $(INCLUDES) -lrt

$(OBJ)/routingHandler.o: $(SRC)/routingHandler.cpp		
	$(GPP) $(CXXFLAGS) -c $(SRC)/routingHandler.cpp -o $(OBJ)/routingHandler.o $(INCLUDES) -lrt

$(OBJ)/packets.o: $(SRC)/packets.cpp		
	$(GPP) $(CXXFLAGS) -c $(SRC)/packets.cpp -o $(OBJ)/packets.o $(INCLUDES) -lrt

$(OBJ)/kernelQueueControl/sendWithEmptyQueue.o: $(SRC)/kernelQueueControl/sendWithEmptyQueue.cpp		
	$(GPP) $(CXXFLAGS) -c $(SRC)/kernelQueueControl/sendWithEmptyQueue.cpp -o $(OBJ)/kernelQueueControl/sendWithEmptyQueue.o $(INCLUDES) -lrt

$(OBJ)/rmf.o: $(SRC)/rmf.cpp		
	$(GPP) $(CXXFLAGS) -c $(SRC)/rmf.cpp -o $(OBJ)/rmf.o $(INCLUDES) -lrt

64-bit:	ARCH=64	
64-bit:	all
	
32-bit: ARCH=32
32-bit: all

$(TARGET): $(OBJS) $(BIN) 
	@echo ${ARCH}
	@echo "CXXFLAGS="${CXXFLAGS}
	@echo "PGCC="${PGCC}
	$(GPP) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LIBS) -lrt $(GPERF)
	@echo "\nNew rmf at binary/rmf\n"

testing: $(OBJ) $(OBJ)/packetPayloads/TBUSDynamicPayload.o $(OBJ)/packetPayloads/TBUSFixedPayload.o $(OBJ)/timeHandler.o $(BIN)
	@echo ${ARCH}
	@echo "CXXFLAGS="${CXXFLAGS}
	@echo "PGCC="${PGCC}
	$(GPP) $(CXXFLAGS) -o $(TESTING) $(OBJ)/packetPayloads/TBUSDynamicPayload.o $(OBJ)/packetPayloads/TBUSFixedPayload.o $(OBJ)/timeHandler.o $(LIBS) -lrt $(GPERF)
	@echo "\nNew testing binary at binary/testing\n"

display:
			@echo ${ARCH}		
			
clean:
			rm -f 	$(OBJS) $(TARGET)
			rm -rf	$(OBJ) $(BIN)/rmf
binary:
	mkdir binary

$(OBJ):	
	mkdir -p $(OBJ)/measurementMethods
	mkdir -p $(OBJ)/measurementMethods/TBUS
	mkdir -p $(OBJ)/kernelQueueControl 
	mkdir -p $(OBJ)/packetPayloads
