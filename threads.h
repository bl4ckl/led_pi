#ifndef THREADS_H
#define THREADS_H

#include "entity/entity.h"

#include <stdint.h>

int threads_init((void*) __clean_add);

int threads_execute_entity(long __current_frame);
int threads_start_entity(entity_t __entity);
int threads_exit_entity();
int threads_cleanup_entity();

int threads_execute_tcp();
int threads_start_tcp();
int threads_exit_tcp();

#endif /* THREADS_H */
