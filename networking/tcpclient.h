#ifndef TCPCLIENT_H
#define TCPCLIENT_H

int tcp_client_init(int* __tcpsockfd);
int tcp_client_start(int __tcpsockfd, int __server_ip, int __server_port);
int tcp_client_stop(int __tcpsockfd);

#endif
