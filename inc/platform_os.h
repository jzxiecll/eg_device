#ifndef __PLATFORM_OS_H__
#define __PLATFORM_OS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

#include "eg_log.h"
typedef xTaskHandle eg_thread_t;
typedef void *eg_thread_arg_t;
typedef struct eg_thread_stack {
	/** Total stack size */
	int size;
} eg_thread_stack_t;


#define egFALSE			( ( BaseType_t ) 0 )
#define egTRUE			( ( BaseType_t ) 1 )
#define egPASS			( egTRUE )
#define egFAIL			( egFALSE )


#define eg_memcpy  memcpy
#define eg_memset  memset


#define eg_thread_stack_define(stackname, stacksize)		\
	eg_thread_stack_t stackname =				\
		{(stacksize) / (sizeof(portSTACK_TYPE))}

void* eg_mem_alloc(int sz);
void eg_mem_free(void *ptr);
void eg_thread_sleep(int ticks);
unsigned long eg_msec_to_ticks(unsigned long msecs);
int eg_thread_create(eg_thread_t *thandle, const char *name,
		     void (*main_func)(eg_thread_arg_t arg),
		     void *arg, eg_thread_stack_t *stack, int prio);



#ifdef __cplusplus
		}
#endif
		
#endif 

