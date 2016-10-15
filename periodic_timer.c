// required header files
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>

#ifdef __APPLE__
#include <dispatch/dispatch.h>
#else
#include <semaphore.h>
#endif

typedef void (*event_cb_t)();

struct rk_sema {
#ifdef __APPLE__
    dispatch_semaphore_t    sem;
#else
    sem_t                   sem;
#endif
    event_cb_t cb;
    void* cb_arg;
};


static inline void
rk_sem_init(struct rk_sema *s, uint32_t value)
{
#ifdef __APPLE__
    dispatch_semaphore_t *sem = &s->sem;

    *sem = dispatch_semaphore_create(value);
#else
    sem_init(&s->sem, 0, value);
#endif
}

static inline void
rk_sem_wait(struct rk_sema *s)
{

#ifdef __APPLE__
    dispatch_semaphore_wait(s->sem, DISPATCH_TIME_FOREVER);
#else
    int r;

    do {
            r = sem_wait(&s->sem);
    } while (r == -1 && errno == EINTR);
#endif
}

static inline void
rk_sem_post(struct rk_sema *s)
{

#ifdef __APPLE__ 
    dispatch_semaphore_signal(s->sem);
#else
    sem_post(&s->sem);
#endif
}
// ----------------------------------------------------------------------------------------
// begin provided code part
// variables needed for timer
static struct rk_sema timer_sem;		// semaphore that's signaled if timer signal arrived
static bool timer_stopped;	// non-zero if the timer is to be timer_stopped
static pthread_t timer_thread;	// thread in which user timer functions execute

/* Timer signal handler.
 * On each timer signal, the signal handler will signal a semaphore.
 */
static void
timersignalhandler() 
{
	/* called in signal handler context, we can only call 
	 * async-signal-safe functions now!
	 */
	rk_sem_post(&timer_sem);	// the only async-signal-safe function pthreads defines
}

/* Timer thread.
 * This dedicated thread waits for posts on the timer semaphore.
 * For each post, timer_func() is called once.
 * 
 * This ensures that the timer_func() is not called in a signal context.
 */
static void *
timerthread(void *_) 
{
  //while (TRUE){
    rk_sem_wait(&timer_sem);		// retry on EINTR
    timer_sem.cb(timer_sem.cb_arg);
    //}
	return 0;
}

/* Initialize timer */
void
init_timer(event_cb_t callback, void* arg) 
{
	/* One time set up */
	rk_sem_init(&timer_sem, 0);
  timer_sem.cb = callback;
  timer_sem.cb_arg = arg;
	pthread_create(&timer_thread, (pthread_attr_t*)0, timerthread, (void*)0);
	signal(SIGALRM, timersignalhandler);
}

/* Shut timer down */
void
shutdown_timer() 
{
	timer_stopped = true;
	rk_sem_post(&timer_sem);
	pthread_join(timer_thread, 0);
}

/* Set a periodic timer.  You may need to modify this function. */
void
set_periodic_timer(long delay) 
{
	struct itimerval tval = { 
		/* subsequent firings */ .it_interval = { .tv_sec = 0, .tv_usec = delay }, 
		/* first firing */       .it_value = { .tv_sec = 0, .tv_usec = delay }};

	setitimer(ITIMER_REAL, &tval, (struct itimerval*)0);
}

// end provided code part