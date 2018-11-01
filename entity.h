#ifndef ENTITY_H
#define ENTITY_H

#include <stdint.h>

#define ENTITY_BYTES_START_LED 6
#define ENTITY_BYTES_PER_LED 4

//entity_group
typedef struct {
	//if of this group (position on the bus)
        uint8_t         group_id;
	//total number of leds in this group
        uint8_t         num_led;
	//array of offsets of the corresponding led in the spi_write_out[]
	//of the corresponding bus
        uint16_t*       led_offset;
}entity_group;

//entity_second (Full Image)
typedef struct {
	//size of spi_write_out
        uint16_t        size_spi_write_out;
	//byte[] to write out to the spi bus
        unsigned char*	spi_write_out;
}entity_second;

//entity_bus
typedef struct {
	//id of this bus
        uint8_t         bus_id;
	//total number of groups on this bus
        uint8_t         num_group;
        entity_group*   group;
	//total number of leds in this bus
        uint16_t        num_led;
	//size of spi_write_out = num_led * 4
	uint16_t	size_spi_write_out;
	//byte[] to write out to the spi bus
        unsigned char*  spi_write_out;
	//struct to hold spi_write_out for the full seconds / images
        entity_second*  second;
}entity_bus;

//entity_change
typedef struct {
	//bus + group + led are the led_id of the change
        uint8_t         bus_index;
	uint16_t	led_offset;
	//color to change to
        uint8_t		color[4];
}entity_change;

//entity_frame
typedef struct {
	//total number of led changes in this frame
        uint16_t        num_change;
        entity_change*  change;
}entity_frame;

//entity
typedef struct {
	//total number of different bus in the system
        uint16_t        num_bus;
        entity_bus*     bus;
	//total number of seconds
        uint16_t        num_second;
	//frames per second
	uint8_t		fps;
	//total number of frames
        uint16_t        num_frame;
        entity_frame*   frame;
}entity;


int entity_write_config(entity* __entity, char* __data);
int entity_write_effects(entity* __entity, char* __data);

int entity_free_config(entity* __entity);
int entity_free_effect(entity* __entity);
int entity_free(entity* __entity);

#endif /* LED_H */
