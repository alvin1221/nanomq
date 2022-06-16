#ifndef NANO_PLATFORM_H
#define NANO_PLATFORM_H

#ifdef NANO_PLATFORM_WINDOWS
#include <windows.h>
#include <winsock2.h>

#include <mswsock.h>
#include <process.h>
#include <ws2tcpip.h>

struct nano_rwlock {
	SRWLOCK rwl;
	BOOLEAN exclusive;
};

#else

#include <pthread.h>

struct nano_rwlock {
	pthread_rwlock_t rwl;
};

#endif

typedef struct nano_rwlock nano_rwlock;

extern void nano_rwlock_init(nano_rwlock *);
extern void nano_rwlock_fini(nano_rwlock *);
extern void nano_rwlock_rdlock(nano_rwlock *);
extern void nano_rwlock_wrlock(nano_rwlock *);
extern void nano_rwlock_unlock(nano_rwlock *);

#endif