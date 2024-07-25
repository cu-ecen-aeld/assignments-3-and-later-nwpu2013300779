#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
//#define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    DEBUG_LOG("threadfunc start\n");
    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    int ret;
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    usleep(thread_func_args->obtain_delay_ms*1000);
    ret=pthread_mutex_lock(thread_func_args->thread_mutex);
    if (ret != 0){
        perror("pthread_mutex_lock error");
        ERROR_LOG("pthread_mutex_lock faild! ret:%d\n", ret);
    }
    
    usleep(thread_func_args->release_delay_ms*1000);
    ret=pthread_mutex_unlock(thread_func_args->thread_mutex);
    if (ret != 0){
        perror("pthread_mutex_unlock error");
        ERROR_LOG("pthread_mutex_unlock faild! ret:%d\n", ret);
    }
    thread_func_args->thread_complete_success=true;
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    DEBUG_LOG("[start] wait_to_obtain_ms:%d wait_to_release_ms:%d\n", wait_to_obtain_ms, wait_to_release_ms);
    int ret;
    struct thread_data *tdata=malloc(sizeof(struct thread_data));
    if (tdata == NULL){
        perror("malloc error");
        ERROR_LOG("thread_data mem akllocation failed\n");
        free(tdata);
        return false;
    }
    tdata->obtain_delay_ms=wait_to_obtain_ms;
    tdata->release_delay_ms=wait_to_release_ms;
    tdata->thread_mutex=mutex;
    tdata->thread_complete_success=false;
    ret=pthread_create(thread, NULL, threadfunc, tdata);
    if (ret != 0){
        perror("pthread_create error");
        ERROR_LOG("thread create faild!\n");
        free(thread);
        return false;
    }
    return true;
}

