/*  INCLUDES MAIN */
#include "led.h"
#include <stdint.h>

typedef struct {
	uint8_t		num_led;
	uint16_t*	led_offset;
}entity_group;

typedef struct {
	uint16_t	num_spi;
	char*		spi;
}entity_second;

typedef struct {
	uint8_t		num_group;
	entity_group*	group;
	uint16_t	num_second;
	entity_second*	second;
	uint8_t		num_led;
	char*		spi;
}entity_bus;

typedef struct {
	uint8_t		bus;
	uint8_t		group;
	char		color[4];
}entity_change;

typedef struct {
	uint16_t	num_change;
	entity_change*	change;
}entity_frame;

typedef struct {
	uint8_t		num_bus;
	entity_bus*	bus;
	uint16_t	num_frame;
	entity_frame*	frame;
}entity;
