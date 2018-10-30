#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#define MESSAGE_ID 		0
#define MESSAGE_CONFIG 		1
#define MESSAGE_EFFECTS 	2
#define MESSAGE_TIME		3
#define MESSAGE_PLAY		4
#define MESSAGE_PAUSE		5
#define MESSAGE_PREVIEW		6
#define MESSAGE_SHOW		7

extern int tcpcreatesocket(int* __tcpsockfd);

extern int tcpsetuphandler(int __tcpsockfd, void* __handler_function);

extern int tcpclientstart(int __tcpsockfd, int __server_ip, int __server_port);

extern int tcpclientstop(int __tcpsockfd);

#endif
