#include "nano_platform.h"
#include <time.h>

#ifdef NANO_PLATFORM_WINDOWS

void
nano_rwlock_init(nano_rwlock *rwl)
{
	InitializeSRWLock(&rwl->rwl);
}

void
nano_rwlock_fini(nano_rwlock *rwl)
{
	rwl->exclusive = FALSE;
}

void
nano_rwlock_rdlock(nano_rwlock *rwl)
{
	AcquireSRWLockShared(&rwl->rwl);
}

void
nano_rwlock_wrlock(nano_rwlock *rwl)
{
	AcquireSRWLockExclusive(&rwl->rwl);
	rwl->exclusive = TRUE;
}

void
nano_rwlock_unlock(nano_rwlock *rwl)
{
	if (rwl->exclusive) {
		rwl->exclusive = FALSE;
		ReleaseSRWLockExclusive(&rwl->rwl);
	} else {
		ReleaseSRWLockShared(&rwl->rwl);
	}
}

#else

void
nano_rwlock_init(nano_rwlock *rwl)
{
	// TODO wait until initialization is completed
	pthread_rwlock_init(&rwl->rwl, NULL);
}

void
nano_rwlock_fini(nano_rwlock *rwl)
{
	int rv;
	if ((rv = pthread_rwlock_destroy(&rwl->rwl)) != 0) {
		// nni_panic("pthread_rwlock_destroy: %s", strerror(rv));
	}
}

void
nano_rwlock_rdlock(nano_rwlock *rwl)
{
	int rv;
	if ((rv = pthread_rwlock_rdlock(&rwl->rwl)) != 0) {
		// nni_panic("pthread_rwlock_rdlock: %s", strerror(rv));
	}
}

void
nano_rwlock_wrlock(nano_rwlock *rwl)
{
	int rv;
	if ((rv = pthread_rwlock_wrlock(&rwl->rwl)) != 0) {
		// nni_panic("pthread_rwlock_wrlock: %s", strerror(rv));
	}
}

void
nano_rwlock_unlock(nano_rwlock *rwl)
{
	int rv;
	if ((rv = pthread_rwlock_unlock(&rwl->rwl)) != 0) {
		// nni_panic("pthread_rwlock_unlock: %s", strerror(rv));
	}
}

#endif