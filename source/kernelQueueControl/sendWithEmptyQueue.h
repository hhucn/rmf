
#ifndef SENDWITHEMPTYQUEUE_H_
#define SENDWITHEMPTYQUEUE_H_

#include <sys/socket.h>


ssize_t sendWithEmptyQueue(int sockfd, const void *buf, size_t len, int flags);
ssize_t writeWithEmptyQueue(int sockfd, const void *buf, size_t len);
int getQueueSize();
int getArpStatus();


#endif /* SENDWITHEMPTYQUEUE_H_ */
