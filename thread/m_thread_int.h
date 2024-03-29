/* The MIT License (MIT)
 *
 * Copyright (c) 2015 Monetra Technologies, LLC.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __M_THREAD_THREAD_INT_H__
#define __M_THREAD_THREAD_INT_H__

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include <mstdlib/mstdlib.h>
#include <mstdlib/thread/m_thread.h>
#include <mstdlib/thread/m_thread_system.h>
#include "platform/m_platform.h"
#ifndef _WIN32
#  include <signal.h>
#  include <poll.h>
#endif
#ifdef __linux__
#  include <sched.h>
#endif

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

__BEGIN_DECLS

typedef void M_thread_t;

typedef struct {
	void (*init)(void);
	void (*deinit)(void);
	/* Thread */
	M_thread_t   *(*thread_create)       (const M_thread_attr_t *attr, void *(*func)(void *), void *arg);
	M_bool        (*thread_join)         (M_thread_t *thread, void **value_ptr);
	M_threadid_t  (*thread_self)         (M_thread_t **thread);
	void          (*thread_yield)        (M_bool force);
	void          (*thread_sleep)        (M_uint64 usec);
	M_bool        (*thread_set_priority) (M_thread_t *thread, M_threadid_t tid, M_uint8 priority);
	M_bool        (*thread_set_processor)(M_thread_t *thread, M_threadid_t tid, int     processor);
	/* System */
	int    (*thread_poll) (struct pollfd fds[], nfds_t nfds, int timeout);
#ifndef _WIN32
	M_bool (*thread_sigmask)(int how, const sigset_t *set, sigset_t *oldset);
#endif
	/* Mutex */
	M_thread_mutex_t *(*mutex_create) (M_uint32 attr);
	void              (*mutex_destroy)(M_thread_mutex_t *mutex);
	M_bool            (*mutex_lock)   (M_thread_mutex_t *mutex);
	M_bool            (*mutex_trylock)(M_thread_mutex_t *mutex);
	M_bool            (*mutex_unlock) (M_thread_mutex_t *mutex);
	/* Cond */
	M_thread_cond_t *(*cond_create)   (M_uint32 attr);
	void             (*cond_destroy)  (M_thread_cond_t *cond);
	M_bool           (*cond_timedwait)(M_thread_cond_t *cond, M_thread_mutex_t *mutex, const M_timeval_t *abstime);
	M_bool           (*cond_wait)     (M_thread_cond_t *cond, M_thread_mutex_t *mutex);
	void             (*cond_broadcast)(M_thread_cond_t *cond);
	void             (*cond_signal)   (M_thread_cond_t *cond);
	/* Read Write Lock */
	M_thread_rwlock_t *(*rwlock_create) (void);
	void               (*rwlock_destroy)(M_thread_rwlock_t *rwlock);
	M_bool             (*rwlock_lock)   (M_thread_rwlock_t *rwlock, M_thread_rwlock_type_t type);
	M_bool             (*rwlock_unlock) (M_thread_rwlock_t *rwlock);
} M_thread_model_callbacks_t;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

struct M_thread_attr {
	M_bool  create_joinable;
	size_t  stack_size;
	M_uint8 priority;
	int     processor;
};

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void M_thread_coop_register(M_thread_model_callbacks_t *cbs);
#if defined(_WIN32)
void M_thread_win_register(M_thread_model_callbacks_t *cbs);
#elif defined(HAVE_PTHREAD)
void M_thread_pthread_register(M_thread_model_callbacks_t *cbs);
#endif

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

M_thread_rwlock_t *M_thread_rwlock_emu_create(void);
void M_thread_rwlock_emu_destroy(M_thread_rwlock_t *rwlock);
M_bool M_thread_rwlock_emu_lock(M_thread_rwlock_t *rwlock, M_thread_rwlock_type_t type);
M_bool M_thread_rwlock_emu_unlock(M_thread_rwlock_t *rwlock);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void M_thread_tls_init(void);
void M_thread_tls_deinit(void);
void M_thread_tls_purge_thread(void);

#  ifdef __linux__
/* Map index of CPU to available CPU core.  This is due to cgroup restrictions in containers, we can't
 * use a normal index. */
void M_thread_linux_cpu_set(cpu_set_t *set, int cpu);
#  endif

__END_DECLS

#endif /* __M_THREAD_THREAD_INT_H__ */
