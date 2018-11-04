/*  INCLUDES MAIN */
#include "entity.h"
#include "spi.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>

int entity_write_config(entity_t* __entity, char* __data) {
	entity_free(__entity);

	if(pthread_mutex_lock(&(__entity->mutex))<0) {
		perror("entity_write_config pthread_mutex_lock");
		return -1;
	}

	int readBytes = 0;
	__entity->num_bus = (uint16_t)__data[readBytes];
	readBytes += 2;

	//Allocate the memory for all busses
	if((__entity->bus = (entity_bus_t*)malloc(sizeof(entity_bus_t) * __entity->num_bus))==NULL) {
		perror("entity_write_config malloc __entity->bus");
		return -1;
	}
	uint16_t num_led_bus;

	for(int i=0; i<(__entity->num_bus); i++) {
		if(pthread_mutex_init(&(__entity->bus[i].mutex), NULL)<0) {
			perror("entity_write_config pthread_mutex_init");
			return -1;
		}

		__entity->bus[i].bus_id = (uint8_t)__data[readBytes];
		readBytes++;

		 __entity->bus[i].num_group = (uint16_t)__data[readBytes];
		readBytes += 2;

		//Allocate the memory for all groups
		if((__entity->bus[i].group = (entity_group_t*)malloc(sizeof(entity_group_t)*__entity->bus[i].num_group))==NULL) {
			printf("allocating memory for groups failed.");
			return -1;
		}
		num_led_bus = 0;

		for(int j=0; j<__entity->bus[i].num_group; j++) {
			__entity->bus[i].group[j].group_id = (uint8_t)__data[readBytes];
			readBytes++;

			__entity->bus[i].group[j].num_led = (uint16_t)__data[readBytes];
			readBytes += 2;

			num_led_bus += __entity->bus[i].group[j].num_led;

			//Allocate the memory for all offsets
			if((__entity->bus[i].group[j].led_offset = (uint16_t*)malloc(sizeof(uint16_t) * __entity->bus[i].group[j].num_led))==NULL) {
				printf("allocating memory for groups failed.");
				return -1;
			}

			for(int k=0; k<__entity->bus[i].group[j].num_led; k++) {
				__entity->bus[i].group[j].led_offset[k] = ENTITY_BYTES_START_LED + k * ENTITY_BYTES_PER_LED;
			}
		}

		__entity->bus[i].num_led = num_led_bus;
		__entity->bus[i].size_spi_write_out = ENTITY_BYTES_START_LED + __entity->bus[i].num_led * ENTITY_BYTES_PER_LED;

		//Allocate the memory for all seconds (full images)
		if((__entity->bus[i].second = (entity_second_t*)malloc(__entity->bus[i].size_spi_write_out * __entity->num_second))==NULL) {
			printf("allocating memory for seconds (full images) failed.");
			return -1;
		}

		//Allocate the memory for the spi_write_out[]
		if((__entity->bus[i].spi_write_out = (unsigned char*)malloc(__entity->bus[i].size_spi_write_out))==NULL) {
			printf("allocating memory for spi_write_out failed.");
			return -1;
		}
		memset(__entity->bus[i].spi_write_out, 0, __entity->bus[i].size_spi_write_out);
	}

	__entity->config_init = true;

	if(pthread_mutex_unlock(&(__entity->mutex))<0) {
		perror("entity_write_config pthread_mutex_unlock");
		return -1;
	}

	return 0;
}

static int entity_free_config(entity_t* __entity) {
	__entity-config_init = false;

	for(int i=0; i<(__entity->num_bus); i++) {
		for(int j=0; j<(__entity->bus[i].num_group); j++) {
			free(__entity->bus[i].group[j].led_offset);
		}
		if(pthread_mutex_destroy(&(__entity->bus[i].mutex))<0) {
			perror("entity_free_config pthread_mutex_destroy");
		}

		free(__entity->bus[i].group);
		free(__entity->bus[i].spi_write_out);
		free(__entity->bus[i].second);
	}

	free(__entity->bus);
	__entity->bus = NULL;

	memset(&(__entity->num_bus), 0, sizeof(__entity->num_bus));

	return 0;
}

int entity_write_effects(entity_t* __entity, char* __data) {
	if(pthread_mutex_lock(&(__entity->mutex))<0) {
		perror("entity_write_effects pthread_mutex_lock");
		return -1;
	}

	if(!(__entity->config_init)) {
		if(pthread_mutex_unlock(&(__entity->mutex))<0) {
			perror("entity_write_effects pthread_mutex_unlock");
			return -1;
		}
		perror("entity_write_effect config_init false");
		return -1;
	}

	entity_free_effect(__entity);

	int readBytes = 0;
	__entity->num_frame = (uint16_t)__data[readBytes];
	readBytes += 2;

	__entity->fps = (uint8_t)__data[readBytes];
	readBytes++;

	__entity->nsec = (long)(1000000000/__entity->fps);

	__entity->num_second = (uint16_t)(__entity->num_frame / __entity->fps);

	//At first write out the full seconds in the corresponding bus
	for(int i=0; i<(__entity->num_second); i++) {
		uint8_t bus_id = (uint8_t)__data[readBytes];
		readBytes++;

		uint8_t group_id = (uint8_t)__data[readBytes];
		readBytes++;

		uint16_t led_id = (uint16_t)__data[readBytes];
		readBytes += 2;

		for(int j=0; j<(__entity->num_bus); j++) {
			if(__entity->bus[j].bus_id==bus_id) {
				for(int k=0; k<(__entity->bus[j].num_group); k++) {
					if(__entity->bus[j].group[k].group_id==group_id) {
						uint16_t offset = __entity->bus[j].group[k].led_offset[led_id];
						memcpy((void*)&(__entity->bus[j].second[i].spi_write_out[offset]), (void*)&__data[readBytes], 4);
						readBytes += 4;
						break;
					}
				}
				break;
			}
		}
	}

	//Allocate space for all frames in all busses
	for(int i=0; i<__entity->num_bus; i++) {
		if((__entity->bus[i].frame=(entity_frame_t*)malloc(sizeof(entity_frame_t)*__entity->num_frame))<0) {
			perror("entity_write_effect malloc frame");
			return -1;
		}
		if(memset(__entity->bus[i].frame, 0, sizeof(entity_frame_t)*__entity->num_frame)<0) {
			perror("entity_write_effect, memset frame");
			return -1;
		}
	}

	//Loop over all frames
	for(int i=0; i<__entity->num_frame; i++) {
		//Read the changes of the current frame
		uint16_t num_changes = (uint16_t)__data[readBytes];
		readBytes += 2;

		if(num_changes>0) {
			//Loop over all changes in this frame
			for(int j=0; j<num_changes; j++) {
				//Read values of current change
				uint8_t bus_id = (uint8_t)__data[readBytes];
				readBytes++;
				uint8_t group_id = (uint8_t)__data[readBytes];
				readBytes++;
				uint16_t led_id = (uint16_t)__data[readBytes];
				readBytes += 2;

				//Loop over all busses
				for(int k=0; k<__entity->num_bus; k++) {
					if(__entity->bus[k].bus_id==bus_id) {
						//Increment the number of changes in this bus in the current frame
						__entity->bus[k].frame[i].num_change++;

						//Get heap space for changes
						//If already allocate realloc, else just allocate one
						if(__entity->bus[k].frame[i].change!=NULL) {
							if((__entity->bus[k].frame[i].change=(entity_change_t*)realloc(__entity->bus[k].frame, sizeof(entity_change_t)*__entity->bus[k].frame[i].num_change))<0) {
								perror("entity_write_effect realloc");
								return -1;
							}
						} else {
							if((__entity->bus[k].frame[i].change=(entity_change_t*)malloc(sizeof(entity_change_t)))<0) {
								perror("entity_write_effect malloc");
								return -1;
							}
						}

						//Loop over all groups
						for(int m=0; m<__entity->bus[k].num_group; m++) {
							if(__entity->bus[k].group[m].group_id==group_id) {
								//Save led_offset
								uint16_t offset = __entity->bus[k].group[m].led_offset[led_id];
								__entity->bus[k].frame[i].change[__entity->bus[k].frame[i].num_change-1].led_offset = offset;
								//Save color
								memcpy(&(__entity->bus[k].frame[i].change[__entity->bus[k].frame[i].num_change-1].color[0]), &__data[readBytes], 4);
								readBytes += 4;
								break;
							}
						}
						break;
					}
				}
			}
		}
	}

	__entity->effects_init = true;

	if(pthread_mutex_unlock(&(__entity->mutex))<0) {
		perror("entity_write_effects pthread_mutex_unlock");
		return -1;
	}

	return 0;
}

static int entity_free_effect(entity_t* __entity) {
	__entity->effects_init = false;

	for(int i=0; i<__entity->num_bus; i++) {
		for(int j=0; j<__entity->num_frame; j++) {
			free(__entity->bus[i].frame[j].change);
		}
		free(__entity->bus[i].frame);
		__entity->bus[i].frame = NULL;
		memset(__entity->bus[i].second, 0, __entity->bus[i].size_spi_write_out);
	}

	memset(&(__entity->num_second), 0, sizeof(__entity->num_second));
	memset(&(__entity->fps), 0, sizeof(__entity->fps));
	memset(&(__entity->num_frame), 0, sizeof(__entity->num_frame));

	return 0;
}

int entity_free(entity_t* __entity) {
	if(pthread_mutex_lock(&(__entity->mutex))<0) {
		perror("entity_free pthread_mutex_lock");
		return -1;
	}

	if(entity_free_config(__entity)<0) {
		printf("free_config() failed.");
		return -1;
	}

	if(entity_free_effect(__entity)<0) {
		printf("free_effect() failed.");
		return -1;
	}

	if(pthread_mutex_unlock(&(__entity->mutex))<0) {
		perror("entity_free pthread_mutex_unlock");
		return -1;
	}

	return 0;
}

int entity_setup_play_handler(void* __handler) {
	struct sigaction act;
	memset(&act, 0, sizeof(act));

	act.sa_handler = __handler;
	if (sigaction(SIGALRM, &act, NULL)<0) {
		perror("entity_setup_play_handler sigaction()");
		return -1;
	}

	return 0;
}

int entity_setup_timer(timer_t* __timer_id) {
	if(timer_create(CLOCK_REALTIME, NULL, __timer_id)) {
		perror("entity_setup_timer timer_create");
		return -1;
	}

	return 0;
}

int entity_play(entity_t* __entity, timer_t* __timer_id, struct itimerspec* __it_spec) {
	memset(__it_spec, 0, sizeof(struct itimerspec));

	__it_spec->it_value.tv_nsec = __entity->nsec;
	__it_spec->it_interval = __it_spec->it_value;

	if(timer_settime(__timer_id, 0, __it_spec, NULL)<0) {
		perror("entity_play timer_settime");
		return -1;
	}

	return 0;
}

int entity_stop(timer_t* __timer_id) {
	struct itimerspec it_spec;
	memset((void*)&it_spec, 0, sizeof(it_spec));

	if(timer_settime(__timer_id, 0, &it_spec, NULL)<0) {
		perror("entity_stop timer_settime");
		return -1;
	}
//	if(timer_delete(__timer_id)<0) {
//		perror("entity_stop timer_delete");
//		return -1;
//	}

	return 0;
}

int entity_full(entity_t* __entity, unsigned char color[4]) {
        if((__entity == NULL) | (__entity->bus == NULL)) {
                unsigned char data[2006];
                memset(&data[0], 0, 6);
		for(int i=0; i<sizeof(data)-6; i++) {
	                memset(&data[6+i], 0b11100000 | color[0]>>3, 1);
	                memset(&data[7+i], color[1], 1);
	                memset(&data[8+i], color[2], 1);
	                memset(&data[9+i], color[3], 1);
		}
		for(int i=0; i<8;i++) {
	                spi_write(i, &data[0], sizeof(data));
		}
        } else {
                for(int i=0;i<__entity->num_bus;i++) {
                        unsigned char data[__entity->bus[i].size_spi_write_out];
                        memset(&data[0], 0, 6);
           		for(int j=0; j<sizeof(data)-6; j++) {
		                memset(&data[6+j], color[0], 1);
		                memset(&data[7+j], color[1], 1);
	        	        memset(&data[8+j], color[2], 1);
	                	memset(&data[9+j], color[3], 1);
			}
                        spi_write(__entity->bus[i].bus_id, &data[0], sizeof(data));
                }
        }

	return 0;
}
