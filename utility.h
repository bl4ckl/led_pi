#ifndef UTILITY_H
#define UTILITY_H

#include <stdlib.h>
#include <stdint.h>

void get_mac(char* __iface, unsigned char __mac_str[13]);
void print_bits(size_t const size, void const* const ptr);

uint16_t btois(void const* const ptr);

#endif /* UTILIT_H */
