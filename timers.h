#ifndef TIMERS_H
#define TIMERS_H

#include <time.h>
#include <signal.h>

typedef struct {
	timer_t			timer_id;
	struct sigevent		sig_event;
	struct itimerspec	timer_spec;
}entity_timer_t;

int timers_init(entity_timer_t* __timer_id, int __signal, void (*__handler)(int, siginfo_t*, void*));
int timers_start(entity_timer_t* __timer_id);
int timers_stop(entity_timer_t* __timer_id);

#endif /* TIMERS_H */
