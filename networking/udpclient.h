#ifndef UDPCLIENT_H
#define UDPCLIENT_H

unsigned int ipv4AddressToInt(char* __ip_address);

int udp_client_init(int __server_port);
int udp_client_send_broadcast();
int udp_client_receive(unsigned int* __server_ip);

#endif

