#ifndef SPI_H
#define SPI_H

#include <stdint.h>
extern int spi_setup(int* __spifd, int __channel, uint32_t __speed);
extern int spi_write(int __channel,unsigned char* __data, int __length);

#endif /* SPI_H */
