/*  INCLUDES MAIN */
#include "entity.h"
#include "../spi.h"
#include "../utility.h"

#include <netinet/in.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>

static int entity_free_config(entity_t* __entity);
static int entity_free_effect(entity_t* __entity);

int entity_write_config(entity_t* __entity, char* __data) {
	entity_free(__entity);

	if(pthread_mutex_lock(&(__entity->mutex))<0) {
		perror("entity_write_config pthread_mutex_lock");
		return -1;
	}

	int readBytes = 0;
	__entity->num_bus = ntohs(*(uint16_t*)&__data[readBytes]);
	readBytes += 2;

	printf("entity:\t received config:\n");
	printf("\t\tnum_bus: %d\n", __entity->num_bus);

	//Allocate the memory for all busses
	if((__entity->bus = (entity_bus_t*)malloc(sizeof(entity_bus_t) * __entity->num_bus))==NULL) {
		perror("entity_write_config malloc __entity->bus");
		return -1;
	}
	memset(__entity->bus, 0, sizeof(entity_bus_t) * __entity->num_bus);
	__entity->bus_malloced = true;
	uint16_t num_led_bus;

	for(int i=0; i<(__entity->num_bus); i++) {
		if(pthread_mutex_init(&(__entity->bus[i].mutex), NULL)<0) {
			perror("entity_write_config pthread_mutex_init");
			return -1;
		}

		__entity->bus[i].bus_id = (uint8_t)__data[readBytes];
		readBytes++;

		 __entity->bus[i].num_group = ntohs(*(uint16_t*)&__data[readBytes]);
		readBytes += 2;

		printf("\t\t\tbus_id: %d\n", __entity->bus[i].bus_id);
		printf("\t\t\t\tnum_group: %d\n", __entity->bus[i].num_group);

		//Allocate the memory for all groups
		if((__entity->bus[i].group = (entity_group_t*)malloc(sizeof(entity_group_t)*__entity->bus[i].num_group))==NULL) {
			printf("allocating memory for groups failed.");
			return -1;
		}
		memset(__entity->bus[i].group, 0, sizeof(entity_group_t)*__entity->bus[i].num_group);
		__entity->bus[i].group_malloced = true;
		num_led_bus = 0;

		for(int j=0; j<__entity->bus[i].num_group; j++) {
			__entity->bus[i].group[j].group_id = (uint8_t)__data[readBytes];
			readBytes++;

			__entity->bus[i].group[j].num_led = ntohs(*(uint16_t*)&__data[readBytes]);
			//print_bits(2, &__data[readBytes]);
			readBytes += 2;

			num_led_bus += __entity->bus[i].group[j].num_led;

			printf("\t\t\t\t\tgroup_id: %d\n", __entity->bus[i].group[j].group_id);
			printf("\t\t\t\t\t\tnum_led: %d\n", __entity->bus[i].group[j].num_led);

			//Allocate the memory for all offsets
			if((__entity->bus[i].group[j].led_offset = (uint16_t*)malloc(sizeof(uint16_t) * __entity->bus[i].group[j].num_led))==NULL) {
				printf("allocating memory for groups failed.");
				return -1;
			}
			memset(__entity->bus[i].group[j].led_offset, 0, sizeof(uint16_t)*__entity->bus[i].group[j].num_led);
			__entity->bus[i].group[j].led_offset_malloced = true;

			for(int k=0; k<__entity->bus[i].group[j].num_led; k++) {
				__entity->bus[i].group[j].led_offset[k] = ENTITY_BYTES_START_LED + k * ENTITY_BYTES_PER_LED;
			}
		}

		__entity->num_led += num_led_bus;
		__entity->bus[i].num_led = num_led_bus;
		__entity->bus[i].size_spi_write_out = ENTITY_BYTES_START_LED + __entity->bus[i].num_led * ENTITY_BYTES_PER_LED;

		printf("\t\t\t\tnum_led: %d\n", __entity->bus[i].num_led);
		printf("\t\t\t\tsize_spi_write_out: %d\n", __entity->bus[i].size_spi_write_out);

		//Allocate the memory for the spi_write_out[]
		if((__entity->bus[i].spi_write_out = (unsigned char*)malloc(__entity->bus[i].size_spi_write_out))==NULL) {
			printf("allocating memory for spi_write_out failed.");
			return -1;
		}
		if((__entity->bus[i].intern_spi_write_out = (unsigned char*)malloc(__entity->bus[i].size_spi_write_out))==NULL) {
			printf("allocating memory for intern_spi_write_out failed.");
			return -1;
		}
		memset(__entity->bus[i].spi_write_out, 0, __entity->bus[i].size_spi_write_out);
		__entity->bus[i].spi_write_out_malloced = true;
	}

	__entity->config_init = true;

	if(pthread_mutex_unlock(&(__entity->mutex))<0) {
		perror("entity_write_config pthread_mutex_unlock");
		return -1;
	}

	return 0;
}

static int entity_free_config(entity_t* __entity) {
	__entity->config_init = false;

	if(__entity->bus_malloced) {
		for(int i=0; i<(__entity->num_bus); i++) {
			if(pthread_mutex_destroy(&(__entity->bus[i].mutex))<0) {
				perror("entity_free_config pthread_mutex_destroy");
			}

			if(__entity->bus[i].group_malloced) {
				for(int j=0; j<(__entity->bus[i].num_group); j++) {
					if(__entity->bus[i].group[j].led_offset_malloced) {
						free(__entity->bus[i].group[j].led_offset);
						__entity->bus[i].group[j].led_offset = NULL;
						__entity->bus[i].group[j].led_offset_malloced = false;
					}
				}
				free(__entity->bus[i].group);
				__entity->bus[i].group = NULL;
				__entity->bus[i].group_malloced = false;
			}

			if(__entity->bus[i].spi_write_out_malloced) {
				free(__entity->bus[i].spi_write_out);
				__entity->bus[i].spi_write_out = NULL;
				free(__entity->bus[i].intern_spi_write_out);
				__entity->bus[i].intern_spi_write_out = NULL;
				__entity->bus[i].spi_write_out_malloced = false;
			}
		}

		free(__entity->bus);
		__entity->bus = NULL;
		__entity->bus_malloced = false;
	}
	__entity->num_bus = 0;
	__entity->num_led = 0;

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
	__entity->num_frame = ntohs(*(uint16_t*)&__data[readBytes]);
	readBytes += 2;

	__entity->fps = (uint8_t)__data[readBytes];
	readBytes++;

	__entity->nsec = (long)(1000000000/__entity->fps);

	__entity->num_second = (uint16_t)(__entity->num_frame / __entity->fps);
	if(__entity->num_frame % __entity->fps > 0) {
		__entity->num_second += 1;
	}

	printf("entity:\t received effects:\n");
	printf("\t\tnum_frame: %d\n", __entity->num_frame);
	printf("\t\tfps: %d\n", __entity->fps);
	printf("\t\tnsec: %ld\n", __entity->nsec);
	printf("\t\tnum_second: %d\n", __entity->num_second);

	//Allocate the memory for all seconds (full images)
	for(int i=0; i<__entity->num_bus; i++) {
//		printf("\tallocating seconds\n");
		if((__entity->bus[i].second = (entity_second_t*)malloc(sizeof(entity_second_t) * __entity->num_second))==NULL) {
			printf("allocating memory for seconds (full images) failed.");
			return -1;
		}
		memset(__entity->bus[i].second, 0, sizeof(entity_second_t)*__entity->num_second);
		__entity->bus[i].second_malloced = true;

		for(int j=0; j<__entity->num_second; j++) {
//			printf("\t\tallocating changes\n");
			if((__entity->bus[i].second[j].spi_write_out = (unsigned char*)malloc(__entity->bus[i].size_spi_write_out))==NULL) {
				printf("allocating memory for seconds spi_write_out (full images) failed.");
				return -1;
			}
			memset(__entity->bus[i].second[j].spi_write_out, 0, __entity->bus[i].size_spi_write_out);
			__entity->bus[i].second[j].spi_write_out_malloced = true;
		}
	}

	//At first write out the full seconds in the corresponding bus
	for(int i=0; i<(__entity->num_second); i++) {
		for(int m=0; m<(__entity->num_led); m++) {
			uint8_t bus_id = (uint8_t)__data[readBytes];
			readBytes++;

			uint8_t group_id = (uint8_t)__data[readBytes];
			readBytes++;

			uint16_t led_id = ntohs(*(uint16_t*)&__data[readBytes]);
			readBytes += 2;

			for(int j=0; j<(__entity->num_bus); j++) {
				if(__entity->bus[j].bus_id==bus_id) {
					for(int k=0; k<(__entity->bus[j].num_group); k++) {
						if(__entity->bus[j].group[k].group_id==group_id) {
							uint16_t offset = __entity->bus[j].group[k].led_offset[led_id];
							memcpy(&__entity->bus[j].second[i].spi_write_out[offset], &__data[readBytes], 4);
							readBytes += 4;
							break;
						}
					}
					break;
				}
			}
		}
	}

	//Allocate space for all frames in all busses and write the first images into the spi_write_out of the bus
	for(int i=0; i<__entity->num_bus; i++) {
		//If we don't do this, spi_write_out is not preloaded with default for all leds (111 00000)
		memcpy(&__entity->bus[i].intern_spi_write_out[0], &__entity->bus[i].second[0].spi_write_out[0], __entity->bus[i].size_spi_write_out);

		printf("\tallocating frames for bus: %d\n", __entity->bus[i].bus_id);
		if((__entity->bus[i].frame=(entity_frame_t*)malloc(sizeof(entity_frame_t)*__entity->num_frame))<0) {
			perror("entity_write_effect malloc frame");
			return -1;
		}
		memset(__entity->bus[i].frame, 0, sizeof(entity_frame_t)*__entity->num_frame);
		__entity->bus[i].frame_malloced = true;
	}

	uint16_t frames_with_changes = 0;
	int32_t total_changes = 0;
	//Loop over all frames
	printf("\tlooping all frames\n");
	fflush(stdout);
	for(int i=0; i<__entity->num_frame; i++) {
		//Read the changes of the current frame
		uint16_t num_changes = ntohs(*(uint16_t*)&__data[readBytes]);
		readBytes += 2;

		if(num_changes>0) {
			frames_with_changes ++;
			total_changes += num_changes;

			//Loop over all changes in this frame
			//We need to loop 2 times here
			//First iteration: count which changes belong to which bus
			int readBytesSave = readBytes;
			for(int j=0; j<num_changes; j++) {
				//Read values of current change

				uint8_t bus_id = (uint8_t)__data[readBytes];
				readBytes += 8;

				//Loop over all busses
				for(int k=0; k<__entity->num_bus; k++) {
					if(__entity->bus[k].bus_id==bus_id) {
						//Increment the number of changes in this bus in the current frame
						__entity->bus[k].frame[i].num_change++;
						break;
					}
				}
			}

			//Get heap space for changes
			for(int j=0; j<__entity->num_bus; j++) {
				if((__entity->bus[j].frame[i].change=(entity_change_t*)malloc(sizeof(entity_change_t)*__entity->bus[j].frame[i].num_change))<0) {
					perror("entity_write_effect malloc");
					return -1;
				}
				__entity->bus[j].frame[i].change_malloced = true;
				__entity->bus[j].frame[i].num_change=0;
			}

			//Second iteration: malloc and write data
			readBytes = readBytesSave;
			for(int j=0; j<num_changes; j++) {
				//Read values of current change
				uint8_t bus_id = (uint8_t)__data[readBytes];
				readBytes++;
				uint8_t group_id = (uint8_t)__data[readBytes];
				readBytes++;
				uint16_t led_id = ntohs(*(uint16_t*)&__data[readBytes]);
				readBytes += 2;

				//Loop over all busses
				for(int k=0; k<__entity->num_bus; k++) {
					if(__entity->bus[k].bus_id==bus_id) {

						//Loop over all groups
						for(int m=0; m<__entity->bus[k].num_group; m++) {
							if(__entity->bus[k].group[m].group_id==group_id) {
								//Save led_offset
								uint16_t offset = __entity->bus[k].group[m].led_offset[led_id];
								__entity->bus[k].frame[i].change[__entity->bus[k].frame[i].num_change].led_offset = offset;
								//Save color
								memcpy(&(__entity->bus[k].frame[i].change[__entity->bus[k].frame[i].num_change].color[0]), &__data[readBytes], 4);
								readBytes += 4;
								__entity->bus[k].frame[i].num_change++;
//								printf("Change:\tOffset: %d\tA: %d\tB: %d\t G: %d\t R: %d\n", offset, __data[readBytes-4], __data[readBytes-3], __data[readBytes-2], __data[readBytes-1]);
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

	printf("\t\tnum_frames_with_changes: %d\n", frames_with_changes);
	printf("\t\tnum_changes: %d\n", total_changes);
	fflush(stdout);

	if(pthread_mutex_unlock(&(__entity->mutex))<0) {
		perror("entity_write_effects pthread_mutex_unlock");
		return -1;
	}

	return 0;
}

static int entity_free_effect(entity_t* __entity) {
	__entity->effects_init = false;

	if(__entity->bus_malloced) {
		for(int i=0; i<__entity->num_bus; i++) {
			if(__entity->bus[i].frame_malloced) {
				for(int j=0; j<__entity->num_frame; j++) {
					if(__entity->bus[i].frame[j].change_malloced) {
						free(__entity->bus[i].frame[j].change);
						__entity->bus[i].frame[j].change = NULL;
						__entity->bus[i].frame[j].change_malloced = false;
					}
				}
				free(__entity->bus[i].frame);
				__entity->bus[i].frame = NULL;
				__entity->bus[i].frame_malloced = false;
			}

			if(__entity->bus[i].second_malloced) {
				for(int j=0; j<__entity->num_second; j++) {
					if(__entity->bus[i].second[j].spi_write_out_malloced) {
						free(__entity->bus[i].second[j].spi_write_out);
						__entity->bus[i].second[j].spi_write_out = NULL;
						__entity->bus[i].second[j].spi_write_out_malloced = false;
					}
				}
				free(__entity->bus[i].second);
				__entity->bus[i].second = NULL;
				__entity->bus[i].second_malloced = false;
			}
		}
	}

	__entity->num_frame = 0;
	__entity->fps = 0;
	__entity->nsec = 0;
	__entity->num_second = 0;

	return 0;
}

int entity_free(entity_t* __entity) {
	if(pthread_mutex_lock(&(__entity->mutex))<0) {
		perror("entity_free pthread_mutex_lock");
		return -1;
	}

	if(entity_free_effect(__entity)<0) {
		printf("free_effect() failed.");
		return -1;
	}

	if(entity_free_config(__entity)<0) {
		printf("free_config() failed.");
		return -1;
	}

	if(pthread_mutex_unlock(&(__entity->mutex))<0) {
		perror("entity_free pthread_mutex_unlock");
		return -1;
	}

	return 0;
}

int entity_full(entity_t* __entity, unsigned char* __color) {
        if((__entity == NULL) || (__entity->bus == NULL)) {
                unsigned char data[2006];
                memset(&data[0], 0, 6);
		for(int i=0; i<sizeof(data)-6; i+=4) {
	                memset(&data[6+i], 0b11100000 | __color[0]>>3, 1);
	                memset(&data[7+i], __color[1], 1);
	                memset(&data[8+i], __color[2], 1);
	                memset(&data[9+i], __color[3], 1);
		}
		for(int i=0; i<8;i++) {
	                spi_write(i, &data[0], sizeof(data));
		}
        } else {
                for(int i=0;i<__entity->num_bus;i++) {
                        unsigned char data[__entity->bus[i].size_spi_write_out];
                        memset(&data[0], 0, 6);
           		for(int j=0; j<sizeof(data)-6; j+=4) {
		                memset(&data[6+j], __color[0], 1);
		                memset(&data[7+j], __color[1], 1);
	        	        memset(&data[8+j], __color[2], 1);
	                	memset(&data[9+j], __color[3], 1);
			}
                        spi_write(__entity->bus[i].bus_id, &data[0], sizeof(data));
                }
        }

	return 0;
}
