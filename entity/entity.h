#ifndef ENTITY_H
#define ENTITY_H

#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>

#define ENTITY_BYTES_START_LED 6
#define ENTITY_BYTES_PER_LED 4

//entity_group
typedef struct {
	//if of this group (position on the bus)
        uint8_t         group_id;
	//total number of leds in this group
        uint16_t         num_led;
	//array of offsets of the corresponding led in the spi_write_out[]
	//of the corresponding bus
	bool		led_offset_malloced;
        uint16_t*       led_offset;
}entity_group_t;

//entity_second (Full Image)
typedef struct {
	//byte[] to write out to the spi bus
	bool		spi_write_out_malloced;
        unsigned char*	spi_write_out;
}entity_second_t;

//entity_change
typedef struct {
	//offset in bus->spi_write_out
	uint16_t	led_offset;
	//color to change to
        uint8_t		color[4];
}entity_change_t;

//entity_frame
typedef struct {
	//total number of led changes in this frame
        uint16_t        	num_change;
	bool			change_malloced;
        entity_change_t*	change;
}entity_frame_t;

//entity_bus
typedef struct {
	//mutex for struct access
	pthread_mutex_t		mutex;
	//id of this bus
        uint8_t        	 	bus_id;
	//total number of groups on this bus
        uint8_t		        num_group;
	bool			group_malloced;
        entity_group_t*		group;
	//total number of leds in this bus
        uint16_t		num_led;
	//size of spi_write_out = num_led * 4
	uint16_t		size_spi_write_out;
	//byte[] to write out to the spi bus
	//we need 2 of them, cause or byte[] will get overridden
	bool			spi_write_out_malloced;
	unsigned char*		intern_spi_write_out;
        unsigned char*		spi_write_out;
	//struct to hold spi_write_out for the full seconds / images
	bool			second_malloced;
        entity_second_t*	second;
	bool			frame_malloced;
        entity_frame_t*   	frame;
}entity_bus_t;

//entity
typedef struct {
	//mutex for struct access
	pthread_mutex_t	mutex;
	//id of this entity
	char		id[13];
	//need to check for this before working with the struct
	bool		config_init;
	bool		effects_init;
	//total number of different bus in the system
        uint16_t        num_bus;
	bool		bus_malloced;
        entity_bus_t*   bus;
	//total number of leds
	uint16_t	num_led;
	//total number of seconds
        uint16_t        num_second;
	//frames per second
	uint8_t		fps;
	//timer intervall in nano sec
	long		nsec;
	//total number of frames
        uint16_t       	num_frame;
}entity_t;


int entity_write_config(entity_t* __entity, char* __data);
int entity_write_effects(entity_t* __entity, char* __data);

int entity_free(entity_t* __entity);

int entity_full(entity_t* __entity, unsigned char color[4]);

#endif /* LED_H */
