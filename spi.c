#include "spi.h"
#include <wiringPiSPI.h>

int spi_setup(int* __spifd, int __channel, uint32_t __speed) {
        *__spifd = wiringPiSPISetup(__channel, __speed);
        if(*__spifd < 0) {
                return -1;
        }
        return 0;
}

int spi_write(int __channel,unsigned char* __data, int __length) {
        return wiringPiSPIDataRW(__channel, __data, __length);
}
