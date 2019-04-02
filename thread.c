#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

# define NUM_THREADS    3

pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER;

void* perform_work(void* argument)
{
    pthread_mutex_lock(&myMutex);
    int passedValue;

    passedValue = *((int *) argument);
    printf("perform_work: starting thread %d\n", passedValue);
    pthread_mutex_unlock(&myMutex);
    return NULL;
}


int main(void)
{
    pthread_t timet;
    int result_code;

    // user input
    char *buffer;
    size_t bufferSize = 10;
    size_t characters;
    buffer = (char *)malloc(bufferSize * sizeof(char));
    if (buffer == NULL)
    {
        perror("unable to allocate dynamic buffer");
        exit(1);
    }

    //pthread_mutex_destroy(&myMutex);

    // create threads
    pthread_mutex_lock(&myMutex);

    // create time thread
    printf("main: creating time thread\n");
    int t = 1;
    result_code = pthread_create(&timet, NULL, perform_work, (void *) &t);
    assert(0 == result_code);

    while (1)
    {

        printf("WHERE TO? > ");
        characters = getline(&buffer, &bufferSize, stdin);


        if (strcmp(buffer, "time\n") == 0)
        {
            printf("time entered\n");

            // unlock mutex
            pthread_mutex_unlock(&myMutex);

            // wait for each thread to complete
            result_code = pthread_join(timet, NULL);
            printf("in main: thread %s is complete\n", "timet");
            assert(0 == result_code);

            // lock mutex again
            pthread_mutex_lock(&myMutex);

            // create new time thread
            result_code = pthread_create(&timet, NULL, perform_work, (void *) &t);
        }

        else
            printf("time not entered\n");

    }



    printf("in main: all threads complete\n");
    return 0;

}

