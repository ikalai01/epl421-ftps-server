#include "config.h"

// Task struct
typedef struct Task {
    int a, b;
} Task;

// FIFO queue of tasks, max size MAX_POOLSIZE
Task taskQueue[THREADPOOL_MAXSIZE];
// Number of tasks in the queue
int taskQueueSize = 0;
// Mutex to protect the queue
pthread_mutex_t taskQueueMutex;
// Condition variable to signal when a task is available
pthread_cond_t taskQueueCond;

void * start_thread(void* args);
void submit_task(Task task);
void execute_task(Task *task);
void run_thread_pool();

void * start_thread(void* args) {
    // Run endless loop
    while(1) {
        // Assign task to current thread
        Task task;
        pthread_mutex_lock(&taskQueueMutex);
        // Wait for task to be available, if no task is available, wait for signal
        while(taskQueueSize == 0) {
            pthread_cond_wait(&taskQueueCond, &taskQueueMutex);
        }
        // Get task from queue
        task = taskQueue[0];
        // Move all tasks in queue to the left
        int i;
        for(i = 0; i < taskQueueSize - 1; i++) {
            taskQueue[i] = taskQueue[i + 1];
        }
        // Decrease queue size
        taskQueueSize--;
        pthread_mutex_unlock(&taskQueueMutex);
        // Execute task
        execute_task(&task);
    }
}

/**
 * @brief Function to submit a task to the thread pool
 * 
 * @param task - task to be submitted
 */
void submit_task(Task task) {
    pthread_mutex_lock(&taskQueueMutex);
    taskQueue[taskQueueSize] = task;
    taskQueueSize++;
    pthread_mutex_unlock(&taskQueueMutex);
    pthread_cond_signal(&taskQueueCond);
}

/**
 * @brief This function is executed by a thread.
 * 
 * @param task - task to be executed
 */
void execute_task(Task *task) {
    usleep(50000); // Sleep for 50 milliseconds
    int result = task->a + task->b; // Do some work
    printf("%d + %d = %d\n", task->a, task->b, result); // Print result
}

/**
 * @brief Create a task object and submit it to the thread pool
 */
void create_task(){
    int i;
    srand(time(NULL));
    for (i = 0; i < 100; i++) {
        submit_task((Task) {
            .a = rand() % 100,
            .b = rand() % 100
        });
        submit_task((Task) {
            .a = rand() % 100,
            .b = rand() % 100
        });
    }
    sleep(100); // Sleep for 100 seconds
        for (i = 0; i < 100; i++) {
        submit_task((Task) {
            .a = rand() % 100,
            .b = rand() % 100
        });
        submit_task((Task) {
            .a = rand() % 100,
            .b = rand() % 100
        });
    }
}

void run_thread_pool(int poolSize) {

    #ifdef DEBUG
    Task task = {
        .a = 1,
        .b = 2
    };
    taskQueue[taskQueueSize++] = task;
    execute_task(&taskQueue[0]);
    #endif

    pthread_t threads[THREADPOOL_NUM_THREADS];
    pthread_mutex_init(&taskQueueMutex, NULL);
    pthread_cond_init(&taskQueueCond, NULL);
    int i;
    for (i = 0; i < THREADPOOL_NUM_THREADS; i++) {
        if(pthread_create(&threads[i], NULL, &start_thread, NULL) != 0) {
            printf("Error creating thread\n");
            exit(1);
        }
    }

    /************************************************************
    * Create tasks
    ************************************************************/
    create_task();

    for(i = 0; i < THREADPOOL_NUM_THREADS; i++) {
        if(pthread_join(threads[i], NULL) != 0) {
            printf("Error joining thread\n");
            exit(1);
        }
    }
    pthread_mutex_destroy(&taskQueueMutex);
    pthread_cond_destroy(&taskQueueCond);
}