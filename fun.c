#include <stdio.h>       /* standard I/O routines                 */
#include <pthread.h>     /* pthread functions and data structures */
#include <stdlib.h>

enum { STATE_A, STATE_B } state = STATE_A;
pthread_cond_t      condA  = PTHREAD_COND_INITIALIZER;
pthread_cond_t      condB  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t     mutex = PTHREAD_MUTEX_INITIALIZER;

/* function to be executed by the new thread */
void* PrintHello1(void* data) {
    int my_data = (int)data;     	/* data received by thread */

    pthread_detach(pthread_self());
    printf("Hello from new thread - got %d\n", my_data);
    for (int i = 0; i < 20; i++) {
        pthread_mutex_lock(&mutex);
        while (state != STATE_A)
            pthread_cond_wait(&condA, &mutex);
        pthread_mutex_unlock(&mutex);

        printf("In thread %d: %d\n", my_data, i);
        state = STATE_B;
        pthread_cond_signal(&condB);
    }
    pthread_exit(NULL);			/* terminate the thread */
}

void* PrintHello2(void* data) {
    int my_data = (int)data;     	/* data received by thread */

    pthread_detach(pthread_self());
    printf("Hello from new thread - got %d\n", my_data);
    for (int i = 0; i < 20; i++) {
        pthread_mutex_lock(&mutex);
        while (state != STATE_B)
            pthread_cond_wait(&condB, &mutex);
        pthread_mutex_unlock(&mutex);

        printf("In thread %d: %d\n", my_data, i);
        state = STATE_A;
        pthread_cond_signal(&condA);
    }
    pthread_exit(NULL);			/* terminate the thread */
}

/* like any C program, program's execution begins in main */
int main(int argc, char* argv[]) {
    int        rc;         	/* return value                           */
    pthread_t  thread_id;     	/* thread's ID (just an integer)          */
    int        t         = 11;  /* data passed to the new thread          */
    int t2 = 22;

    /* create a new thread that will execute 'PrintHello' */
    rc = pthread_create(&thread_id, NULL, PrintHello1, (void*)t);
    rc = pthread_create(&thread_id, NULL, PrintHello2, (void*)t2);  
    if(rc)			/* could not create thread */
    {
        printf("\n ERROR: return code from pthread_create is %d \n", rc);
        exit(1);
    }
    
    printf("\n Created new thread (%u) ... \n", thread_id);
  
    if(rc)			/* could not create thread */
    {
        printf("\n ERROR: return code from pthread_create is %d \n", rc);
        exit(1);
    }
    
    pthread_exit(NULL);		/* terminate the thread */
}