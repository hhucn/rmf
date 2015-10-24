#include "sendWithEmptyQueue.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>

int maxQueueSize = 0;
int useQueueBlocking = 0;

ssize_t sendWithEmptyQueue(int sockfd, const void *buf, size_t len, int flags){
	if(useQueueBlocking){
		while(getQueueSize() > maxQueueSize){
			// active wait
		}
	}
	return send(sockfd, buf, len, flags);
}

ssize_t writeWithEmptyQueue(int sockfd, const void *buf, size_t len){
	if(useQueueBlocking){
		while(getQueueSize() > maxQueueSize){
			// active wait
		}
	}
	return write(sockfd, buf, len);
}


int getQueueSize(){
	   FILE *fp;

	   fp = fopen("/proc/queueLengthDevice","r"); // read mode

	   char read[20];
	   int counter=0;
	   while( ( read[counter++] = fgetc(fp) ) != EOF ){
	      //printf("%c",read[counter-1]);
	   }
	   read[counter] = 0;

	   int value;
	   sscanf(read, "%d", &value);
	   fclose(fp);
	   return value;
}

int getArpStatus(){
	   FILE *fp;

	   fp = fopen("/proc/arpStatus","r"); // read mode

	   char read[20];
	   int counter=0;
	   while( ( read[counter++] = fgetc(fp) ) != EOF ){
	      //printf("%c",read[counter-1]);
	   }
	   read[counter] = 0;

	   int value;
	   sscanf(read, "%d", &value);
	   fclose(fp);
	   return value;
}

