#ifndef UDPCLIENT_H
#define UDPCLIENT_H

extern unsigned int ipv4AddressToInt(char* __ip_address);

extern int udpclientstart(unsigned int* __server_ip, int __server_port);

#endif

