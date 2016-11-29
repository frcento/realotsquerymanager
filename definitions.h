#ifndef __DEFINITIONS_H__
#define __DEFINITIONS_H__

#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <stdint.h>
#include <errno.h>

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#else
#include <winsock2.h>
#endif

#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#ifndef SOCKET_ERROR
	#define SOCKET_ERROR -1
#endif

#ifndef ERROR_EINTR
	#define ERROR_EINTR EINTR
#endif

#ifndef SOCKET
	#define SOCKET int
#endif

#ifndef closesocket
	#define closesocket close
#endif

extern int errno;

#define THREAD_RETURN void*

inline void CREATE_THREAD(void *(*a)(void*), void *b)
{
	pthread_attr_t attr;
	pthread_t id;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	pthread_create(&id, &attr, a, b);
}

inline void SLEEP(unsigned long t)
{
	timespec tv;
	tv.tv_sec  = t / 1000;
	tv.tv_nsec = (t % 1000)*1000000;
	nanosleep(&tv, NULL);
}

#endif
