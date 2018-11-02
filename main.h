#ifndef MAIN_H
#define MAIN_H

#ifndef SERVER_PORT
#define SERVER_PORT 31313
#endif

#include <stdint.h>

struct ledmessage {
	uint8_t		messageID;
	uint32_t	dataLength;
	uint32_t	dataReceived;
	char*		data;
};

void black();
void cleanup();

#endif /* MAIN_H */
