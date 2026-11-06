/*
 * Windows pthread compatibility header for TECC
 *
 * Provides pthread-like API using Windows primitives.
 * For use with MSVC builds on Windows.
 */
#ifndef PTHREAD_H_WIN32
#define PTHREAD_H_WIN32

/* Block src/port/pthread-win32.h from being included alongside this header.
 * Both define pthread types but with different signatures (C2371). */
#define __PTHREAD_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

/* Ensure _WIN32_WINNT >= 0x0600 for SRWLock APIs (Vista+).
 * If the project sets it lower, the rwlock fallback uses CriticalSection. */
#ifndef _WIN32_WINNT
#define PTHREAD_WIN32_WINNT_NEEDS_RESTORE
#define _WIN32_WINNT 0x0600
#endif
#if _WIN32_WINNT < 0x0600
#define PTHREAD_RWLOCK_USE_CSECT
#endif

#ifdef PTHREAD_WIN32_WINNT_NEEDS_RESTORE
#undef _WIN32_WINNT
#endif

/* pthread_once */
typedef struct {
	LONG state;         /* 0 = not done, 1 = done, 2 = in progress */
} pthread_once_t;

#define PTHREAD_ONCE_INIT {0}

static __inline int pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
{
	/* Use a simple spin-based approach without InterlockedCompareExchange
	 * to avoid intrinsic mismatch (C2168) on some MSVC versions. */
	if (0 == InterlockedExchange(&once_control->state, 2)) {
		init_routine();
		InterlockedExchange(&once_control->state, 1);
		return 0;
	}
	while (1 == InterlockedExchangeAdd(&once_control->state, 0)) {
		SwitchToThread();
	}
	return 0;
}

/* pthread_mutex */
typedef struct {
	LONG		initstate;	/* 0: not init; 1: init done; 2: init in progress */
	CRITICAL_SECTION csection;
} pthread_mutex_t;

#define PTHREAD_MUTEX_INITIALIZER {0}

static __inline int pthread_mutex_init(pthread_mutex_t *m, void *attr)
{
	(void)attr;
	m->initstate = 1;
	InitializeCriticalSection(&m->csection);
	return 0;
}

static __inline int pthread_mutex_lock(pthread_mutex_t *m)
{
	if (m->initstate != 1) {
		InitializeCriticalSection(&m->csection);
		m->initstate = 1;
	}
	EnterCriticalSection(&m->csection);
	return 0;
}

static __inline int pthread_mutex_unlock(pthread_mutex_t *m)
{
	LeaveCriticalSection(&m->csection);
	return 0;
}

static __inline int pthread_mutex_trylock(pthread_mutex_t *m)
{
	if (m->initstate != 1) {
		InitializeCriticalSection(&m->csection);
		m->initstate = 1;
	}
	return TryEnterCriticalSection(&m->csection) ? 0 : 1;
}

static __inline int pthread_mutex_destroy(pthread_mutex_t *m)
{
	DeleteCriticalSection(&m->csection);
	m->initstate = 0;
	return 0;
}

/* pthread_rwlock - implemented using SRW locks (Vista+) or CriticalSection fallback */
typedef struct {
#ifdef PTHREAD_RWLOCK_USE_CSECT
	CRITICAL_SECTION cs;
#else
	SRWLOCK srwlock;
#endif
} pthread_rwlock_t;

#ifdef PTHREAD_RWLOCK_USE_CSECT
#define PTHREAD_RWLOCK_INITIALIZER {0}
#else
#define PTHREAD_RWLOCK_INITIALIZER {{0}}
#endif

static __inline int pthread_rwlock_init(pthread_rwlock_t *rw, void *attr)
{
	(void)attr;
#ifdef PTHREAD_RWLOCK_USE_CSECT
	InitializeCriticalSection(&rw->cs);
#else
	InitializeSRWLock(&rw->srwlock);
#endif
	return 0;
}

static __inline int pthread_rwlock_rdlock(pthread_rwlock_t *rw)
{
#ifdef PTHREAD_RWLOCK_USE_CSECT
	EnterCriticalSection(&rw->cs);
#else
	AcquireSRWLockShared(&rw->srwlock);
#endif
	return 0;
}

static __inline int pthread_rwlock_wrlock(pthread_rwlock_t *rw)
{
#ifdef PTHREAD_RWLOCK_USE_CSECT
	EnterCriticalSection(&rw->cs);
#else
	AcquireSRWLockExclusive(&rw->srwlock);
#endif
	return 0;
}

static __inline int pthread_rwlock_unlock(pthread_rwlock_t *rw)
{
#ifdef PTHREAD_RWLOCK_USE_CSECT
	LeaveCriticalSection(&rw->cs);
#else
	ReleaseSRWLockExclusive(&rw->srwlock);
#endif
	return 0;
}

static __inline int pthread_rwlock_destroy(pthread_rwlock_t *rw)
{
#ifdef PTHREAD_RWLOCK_USE_CSECT
	DeleteCriticalSection(&rw->cs);
#else
	(void)rw;
#endif
	return 0;
}

/* pthread_key_t for thread-local storage */
typedef DWORD pthread_key_t;

static __inline int pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
	(void)destructor;
	*key = TlsAlloc();
	return (*key == TLS_OUT_OF_INDEXES) ? -1 : 0;
}

static __inline int pthread_setspecific(pthread_key_t key, const void *value)
{
	return TlsSetValue(key, (LPVOID)value) ? 0 : -1;
}

static __inline void *pthread_getspecific(pthread_key_t key)
{
	return TlsGetValue(key);
}

static __inline int pthread_key_delete(pthread_key_t key)
{
	return TlsFree(key) ? 0 : -1;
}

/* pthread_t */
typedef DWORD pthread_t;

/* pthread_self */
static __inline pthread_t pthread_self(void)
{
	return GetCurrentThreadId();
}

#endif /* PTHREAD_H_WIN32 */
