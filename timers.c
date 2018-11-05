#include "timers.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <signal.h>

int timers_init(entity_timer_t* __timer_id, int __signal_id, void (*__handler)(int, siginfo_t*, void*)) {
	struct sigaction sigac;

	sigac.sa_flags = SA_SIGINFO;
	sigac.sa_sigaction = __handler;
	sigemptyset(&sigac.sa_mask);
	if(sigaction(__signal_id, &sigac, NULL)<0) {
		perror("timers init_timer sigaction");
		return -1;
	}

	__timer_id->sig_event.sigev_notify = SIGEV_SIGNAL;
	__timer_id->sig_event.sigev_signo = __signal_id;
	__timer_id->sig_event.sigev_value.sival_ptr = &__timer_id->timer_id;

	if(timer_create(CLOCK_REALTIME, &__timer_id->sig_event, &__timer_id->timer_id)<0) {
		perror("timers init_timer timer_create");
		return -1;
	}

	return 0;
}

int timers_start(entity_timer_t* __timer_id) {
	if(timer_settime(__timer_id->timer_id, 0, &__timer_id->timer_spec, NULL)<0) {
		perror("timers start_timer timer_settime");
		return -1;
	}

	return 0;
}

int timers_stop(entity_timer_t* __timer_id) {
        struct itimerspec it_spec;
        memset(&it_spec, 0, sizeof(it_spec));

	if(timer_settime(__timer_id->timer_id, 0, &it_spec, NULL)<0) {
		perror("timers stop_timer timer_settime");
		return -1;
	}

	return 0;
}
