#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <sys/time.h>
#include "DET.h"

#define NUM_PROCESSES 3
#define MICRO_SEC_IN_SEC 1000000


static int sem_id;
static int process_num = 0;
static int num_signals = 0;
//static int sem_id;
//  TODO: Refactor child/parent conditional separation
/*
    1. Create 3 processes, each with an id value (1, 2, 3) corresponding
        to the values each process will need to compute
    2. Compute each value corresponding to the id value
    3. Use semaphores to control access to shared memory and add the
        result of each process' computation to memory
    4. Wait for all processes to finish and print the result
*/

/*
    New design:
    1. Create shared memory
    2. Fork 2 chid processes (EDIT: 3 child processes)
    3. Attach shared memory to all processes
    4. Make all processes WAIT until the parent process has initialized
        the value of the data in shared memory (DEPRECATED, shared data init
        vefore child processes forked)
    5. Compute determinant portion for all processes
    6. Use semaphore to control loading of data to shared memory
    7a. Let the parent WAIT until it receives TWO SIGALRM before displaying
        and returning the result (EDIT: waits until all child processes are killed)
    7b. The child processes will detach from shared memory and then send
        an SIGALRM to parent process and then kill itself
    8. The parent will save and print the result
    9. The parent will detach and delete shared memory
    10. The parent will return the result

*/

/*

*/

/*
    FIXME:
        - child process 3 enters critical area TWICE????

*/

int main(int argc, char *argv[])
{
    int D[3][3] = {
        {20, 20, 50},
        {10, 6, 70},
        {40, 3, 2}
    };
    int E[3][3] = {
        {2, 7, 9},
        {7, 4, 2},
        {60, 55, 1}
    };
    int det_result1, det_result2;

    printf("\n");
    printf("--TEST INPUT 1--\n");
    det_result1 = compute_determinant(D);
    printf("Expected output: determinant = 41140, largest = 70\n");
    printf("--TEST INPUT 2--\n");
    det_result2 = compute_determinant(E);
    printf("Expected output: determinant = 1884, largest = 60\n");

}

/**
 * Computes the determinant of 3x3 matrix D
 * using 3 processes.
 * 
 * @param   int**, input 3x3 matrix
 * @return  int, determinant of matrix D
 * */
int compute_determinant(int D[3][3])
{
    // Remember: Fail hard, fail fast!
    //pid_t pid_1, pid_2, pid_3; // process id
    //int compute_id = 1; // id for computation todo
    int shmid; // shared memory id
    struct shared_use_st *shared_memory = (void *)0;
    int result;
    int stat_val1, stat_val2, stat_val3;
    struct timeval start, end;

    gettimeofday(&start, NULL);
    /*
    struct sigaction act;
    act.sa_handler = handle_signal;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    */

    // 1. Create shared memory, attach to parent process, & initialize
    create_shared_memory(&shmid);
    attach_shared_memory(&shared_memory, shmid);
    shared_memory->determinant = 0;
    shared_memory->largest = D[0][0];

    // 1a. Set semaphore
    sem_id = semget((key_t)1234, 1, 0666 | IPC_CREAT);
    if (!set_semvalue())
    {
        fprintf(stderr, "Failed to initialize semaphore\n");
        exit(EXIT_FAILURE);
    }

    // 2. Fork 3 child processes
    //create_processes(&pid_1, &pid_2, &pid_3);
    create_processes();     // forks 3 child processes

    // 3. Attach shared memory for all child processes
    if (process_num > 0)
    {
        attach_shared_memory(&shared_memory, shmid);
    //}

    // 4. Make all processes WAIT until the parent process has
    //  initialized the value of the data in shared memory
    //      - OMITTED since initialized before forking

    // 5. Compute the determinant portion of all processes
    //if (process_num > 0) {
        printf("%d: Child Process: working with element %d of D\n", process_num, process_num-1);
        int a, b, c, d;
        int largest_row_val = D[process_num-1][0];
        result = 0;
        //printf("%d: Child process: working with element D[%d]\n", process_num, process_num);
        a = (process_num == 2) ? -1 : 1;
        b = process_num - 1;
        c = (process_num == 1) ? 1 : 0;
        d = (process_num == 3) ? 1 : 2;
        result = a * D[0][b] * (D[1][c] * D[2][d] - D[2][c] * D[1][d]);
        //printf("%d: Result of D[%d] = %d", process_num, process_num, result);

        for (int i=1; i<NUM_PROCESSES; i++) {
            // iterate through array
            int arr_elm = D[process_num-1][i];
            largest_row_val = (largest_row_val < arr_elm) ? arr_elm : largest_row_val;
        }

    //}

    // 6. Use semaphore to control loading of data to shared memory
    //
    //if (process_num > 0)
    //{
        if (!semaphore_p())
        {
            exit(EXIT_FAILURE);
        }
        //sleep(2);
        //printf("%d: about to add %d to %d\n", process_num, result, shared_memory->determinant);
        shared_memory->determinant = shared_memory->determinant + result;
        int largest_val = shared_memory->largest;
        shared_memory->largest = (largest_row_val > largest_val) ? largest_row_val : largest_val;
        //sleep(2);
        if (!semaphore_v())
        {
            exit(EXIT_FAILURE);
        }
    }

    // 7a. Let the parent WAIT until it receives THREE SIGALRM before
    //  displaying and returning the result (DEPRECATED)
    // EDIT: wait until all child processes are killed
    if (process_num == 0)
    {
       // waits until there are no child processes
        while ((wait(&stat_val1)) > 0)
            ; // wait? idk
        //printf("%d:Done waiting!\n", process_num);
    }
    // 7b. The child processes will detach from shared memory and then
    //  send a SIGALRM to parent process and then kill itself
    // EDIT: no Sigaction/Signal used. Process ends self
    else if (process_num > 0)
    {
        detach_shared_memory(shared_memory);
        //printf("%d:Ending child process %d\n", process_num, process_num);
        //kill(getppid(), SIGALRM);
        exit(EXIT_SUCCESS);
    }

    // 8. The parent will save the result
    result = shared_memory->determinant;
    int largest_elm = shared_memory->largest;
    printf("%d: Determinant of matrix = %d\nlargest element: %d\n", 
        process_num, result, largest_elm);

    // 9. The parent will detach and delete the shared memory
    detach_shared_memory(shared_memory);
    delete_shared_memory(shmid);

    // 10. The parent will return the result
    gettimeofday(&end, NULL);
    printf("%d: Elapsed time: %ld microseconds\n", process_num, end.tv_sec * MICRO_SEC_IN_SEC + 
            end.tv_usec - (start.tv_sec * MICRO_SEC_IN_SEC + start.tv_usec));
    return result;
}

/**
 * Forks three child processes,
 * where each process' global compute_id
 * value is set to the the value of id.
 * 
 * @param   none
 * @return  none
 * */
void create_processes()
{
    pid_t pid;
    for (int i=1; i<=3; i++) {
        pid = fork();
        switch (pid) {
            case -1:
                // fork fails
                perror("fork failed");
                exit(EXIT_FAILURE);
            case 0:
                // is parent
                break;
            default:
                // is child process i
                process_num = i;
                return;
        }
    }

    return;
}

/**
 * Creates shared memory.
 * Exits on failure.
 * 
 * @param   int*, shared memory id pointer
 * @return  none
 * */
void create_shared_memory(int *shmid)
{
    // Create shared memory
    //int shmid
    *shmid = shmget((key_t)1245, sizeof(struct shared_use_st), 0666 | IPC_CREAT);
    if (*shmid == -1)
    {
        // Exit if shared memory creation fails
        fprintf(stderr, "%d:shmget failed\n", process_num);
        exit(EXIT_FAILURE);
    }
}

/**
 * Attaches shared memory to the
 * address space of current process.
 * Exits on failure.
 * 
 * @param   int**, pointer to shared memory pointer
 * @param   int, shared memory id
 * @return  none
 * */
void attach_shared_memory(struct shared_use_st **shared_memory, int shmid)
{
    // Attach shared memory to address space of all processes
    *shared_memory = shmat(shmid, (void *)0, 0);
    if (shared_memory == (void *)-1)
    {
        fprintf(stderr, "%d:shmat failed\n", process_num);
        exit(EXIT_FAILURE);
    }
    //printf("%d: Memory attached at %X\n", process_num, (int)shared_memory);
}

/**
 * Detaches shared memory segment from
 * current process.
 * Exits on failure.
 * 
 * @param   int*, shared_memory pointer
 * @return  none
 * */
void detach_shared_memory(struct shared_use_st *shared_memory)
{
    if (shmdt(shared_memory) == -1)
    {
        fprintf(stderr, "%d:shmdt failed\n", process_num);
        exit(EXIT_FAILURE);
    }
    //printf("%d: shmdt success\n", process_num);
}

/**
 * Deletes shared memory segment.
 * Exits on failure.
 * 
 * @param   int, shared memory id
 * @return  none
 * */
void delete_shared_memory(int shmid)
{
    if (shmctl(shmid, IPC_RMID, 0) == -1)
    {
        fprintf(stderr, "%d:shmctl(IPC_RMID) failed\n", process_num);
        exit(EXIT_FAILURE);
    }
    //printf("%d:shared mem delete success\n", process_num);
}

/**
 * Empty function for handling signal.
 * */
void handle_signal(int sig)
{
    num_signals++;
}

/**
 * Adds portion of calculated 
 * determinant to shared memory.
 * 
 * @param   int*, pointer to shared memory address
 * @param   int, portion of determinant
 * @return  none
 * */
void add_to_shared_memory(int *shared_memory, int data)
{
    shared_memory += data;
}

static int set_semvalue(void)
{
    union semun sem_union;
    sem_union.val = 1;
    if (semctl(sem_id, 0, SETVAL, sem_union) == -1)
        return (0);
    return (1);
}

static void del_semvalue(void)
{
    union semun sem_union;
    if (semctl(sem_id, 0, IPC_RMID, sem_union) == -1)
        fprintf(stderr, "Failed to delete semaphore\n");
}

static int semaphore_p(void)
{
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = -1; /* P() */
    sem_b.sem_flg = SEM_UNDO;
    //printf("\n%d:Waiting...\n", process_num);
    if (semop(sem_id, &sem_b, 1) == -1)
    {
        fprintf(stderr, "semaphore_p failed\n");
    }
    //printf("%d:Critical section ENTERED!\n", process_num);
    return (1);
}

static int semaphore_v(void)
{
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = 1; /* V() */
    sem_b.sem_flg = SEM_UNDO;
    //printf("\nLeaving critical section!\n");
    if (semop(sem_id, &sem_b, 1) == -1)
    {
        fprintf(stderr, "semaphore_v failed\n");
        return (0);
    }
    //printf("\n%d:Critical section EXITED!\n", process_num);
    return (1);
}