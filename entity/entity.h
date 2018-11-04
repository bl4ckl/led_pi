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
        uint8_t         num_led;
	//array of offsets of the corresponding led in the spi_write_out[]
	//of the corresponding bus
        uint16_t*       led_offset;
}entity_group_t;

//entity_second (Full Image)
typedef struct {
	//size of spi_write_out
        uint16_t        size_spi_write_out;
	//byte[] to write out to the spi bus
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
        entity_group_t*		group;
	//total number of leds in this bus
        uint16_t		num_led;
	//size of spi_write_out = num_led * 4
	uint16_t		size_spi_write_out;
	//byte[] to write out to the spi bus
        unsigned char*		spi_write_out;
	//struct to hold spi_write_out for the full seconds / images
        entity_second_t*	second;
        entity_frame_t*   	frame;
}entity_bus_t;

//entity
typedef struct {
	//mutex for struct access
	pthread_mutex_t	mutex;
	//id of this entity
	char[13]	id;
	//need to check for this before working with the struct
	bool		config_init;
	bool		effects_init;
	//total number of different bus in the system
        uint16_t        num_bus;
        entity_bus_t*   bus;
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

static int entity_free_config(entity_t* __entity);
static int entity_free_effect(entity_t* __entity);
int entity_free(entity_t* __entity);

int entity_setup_play_handler(void* __handler);
int entity_setup_timer(timer_t* __timer_id);
int entity_play(entity_t* __entity, timer_t* __timer_id, struct itimerspec* __it_spec);
int entity_stop(timer_t* __timer_id);
int entity_full(entity_t* __entity, unsigned char color[4]);

#endif /* LED_H */
