#ifndef THREADS_H
#define THREADS_H

#include "entity/entity.h"

#include <stdint.h>
#include <stdbool.h>

int threads_init();

int threads_execute_entity(long __current_frame);
int threads_start_entity(entity_t* __entity);
int threads_exit_entity();

int threads_execute_tcp();
int threads_start_tcp(int __sockfd, entity_t* __entity,
        void (*__entity_timesync)(uint32_t),
	void (*__entity_config_effects_pre)(void),
	void (*__entity_effects_post)(void),
        void (*__entity_play)(void),
        void (*__entity_pause)(void),
        void (*__entity_show)(void),
	void (*__entity_color)(uint8_t*));
int threads_tcp_update_sockfd(int __sockfd);
int threads_exit_tcp();

#endif /* THREADS_H */
