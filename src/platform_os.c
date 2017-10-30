#include "platform_os.h"
 /************************************************************************************/

void* eg_mem_alloc(int sz)
{  
    void *buf = NULL;
    buf = eg_memalloc(sz);
    if(NULL == buf)
        eg_log_debug("failed eg_mem_malloc! size:%d\r\n", sz);
    return buf;
}

void eg_mem_free(void *ptr)
{   
    if(ptr)
        eg_memfree(ptr);
}
 
 void eg_thread_sleep(int ticks)
{

	vTaskDelay(ticks);
	return;
}

 unsigned long eg_msec_to_ticks(unsigned long msecs)
{
	return (msecs) / (portTICK_RATE_MS);
}



int eg_thread_create(eg_thread_t *thandle, const char *name,
		     void (*main_func)(eg_thread_arg_t arg),
		     void *arg, eg_thread_stack_t *stack, int prio)
{
	int ret;

	ret = xTaskCreate(main_func, name, stack->size, arg, prio, thandle);
	return ret == egPASS ? 0 : -1;

}



