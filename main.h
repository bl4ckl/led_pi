#ifndef MAIN_H
#define MAIN_H

#ifndef SERVER_PORT
#define SERVER_PORT 31313
#endif

#include <signal.h>

#define TIME_BROADCAST_SEC 	1
#define TIME_HEARTBEAT_NSEC	500000000
#define TIME_SHOW_SEC		2

#define TIME_SIGNAL_HIGHEST	SIGRTMIN
#define TIME_SIGNAL_HIGH	SIGRTMIN+1
#define TIME_SIGNAL_LOW		SIGRTMIN+2
#define TIME_SIGNAL_LOWER	SIGRTMIN+3
#define TIME_SIGNAL_UNIMPORTANT	SIGRTMIN+4

void black();
void cleanup();

#endif /* MAIN_H */
