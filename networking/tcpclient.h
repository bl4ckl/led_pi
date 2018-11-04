#ifndef TCPCLIENT_H
#define TCPCLIENT_H

extern int tcpcreatesocket(int* __tcpsockfd);

extern int tcpsetuphandler(int __tcpsockfd, void* __handler_function);

extern int tcpclientstart(int __tcpsockfd, int __server_ip, int __server_port);

extern int tcpclientstop(int __tcpsockfd);

#endif
