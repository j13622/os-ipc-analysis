/* CS 519, Spring 2025: Project 1 - Part 2
 * IPC using pipes to perform matrix multiplication.
 * Feel free to extend or change any code or functions below.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sched.h>


//Add all your global variables and definitions here.
#ifndef MATRIX_SIZE
#define MATRIX_SIZE 1000
#endif
#ifndef NUM_CHILD_PROCS
#define NUM_CHILD_PROCS 8
#endif
#ifndef NUM_CORES
#define NUM_CORES 8
#endif

float arr1[MATRIX_SIZE][MATRIX_SIZE];
float arr2[MATRIX_SIZE][MATRIX_SIZE];
float out[MATRIX_SIZE][MATRIX_SIZE];

struct timeval begin;
struct timeval end;

union semun {
    int val;               // Value for SETVAL
    struct semid_ds *buf;  // Buffer for IPC_STAT, IPC_SET
    unsigned short *array; // Array for GETALL, SETALL
    struct seminfo *__buf; // Buffer for IPC_INFO (Linux-specific)
};

void semaphore_init(int sem_id, int sem_num, int init_valve)
{
    semctl(sem_id, sem_num, SETVAL, init_valve);
}

void semaphore_release(int sem_id, int sem_num)
{
    //Use semop to release a semaphore
    struct sembuf release_op = {sem_num, 1, 0};
    semop(sem_id, &release_op, 1);
}

void semaphore_reserve(int sem_id, int sem_num)
{
    //Use semop to acquire a semaphore
    struct sembuf reserve_op = {sem_num, -1, 0};
    semop(sem_id, &reserve_op, 1);
}

/* Time function that calculates time between start and end */
double getdeltatimeofday(struct timeval *begin, struct timeval *end)
{
    return (end->tv_sec + end->tv_usec * 1.0 / 1000000) -
           (begin->tv_sec + begin->tv_usec * 1.0 / 1000000);
}


/* Stats function that prints the time taken and other statistics that you wish
 * to provide.
 */
void print_stats() {
    printf("Input size: %d columns, %d rows\n", MATRIX_SIZE, MATRIX_SIZE);
    printf("Total cores: %d\n", NUM_CORES);
    double runtime = getdeltatimeofday(&begin, &end);
    printf("Total runtime: %lf\n", runtime);
}


int main(int argc, char const *argv[])
{
    gettimeofday(&begin, NULL);

    //initializing semaphores. semaphore 0 = "buf empty", 1 = "buf full"
    int sem_id = semget(IPC_PRIVATE, 2, IPC_CREAT | 0666);
    semaphore_init(sem_id, 0, 1);
    semaphore_init(sem_id, 1, 0);

    //loading matrices from file to memory
    FILE *fp1 = fopen("mat1.csv", "r");
    FILE *fp2 = fopen("mat2.csv", "r");
    for(int i = 0; i < MATRIX_SIZE; i++) {
        for(int j = 0; j < MATRIX_SIZE; j++) {
            float f; 
            fscanf(fp1, "%f,", &f);
            arr1[i][j] = f;
            fscanf(fp2, "%f,", &f);
            arr2[i][j] = f;    
        }
    }
    fclose(fp1);
    fclose(fp2);

    //setting up pipe
    int pipefd[2];
    pipe(pipefd);
    size_t pipesize = fpathconf(pipefd[0], _PC_PIPE_BUF);
    int buf_size = (pipesize-4*sizeof(int))/sizeof(float);

    //setting up core count
    cpu_set_t mask;
    CPU_ZERO(&mask);
    for(int i = 0; i < NUM_CORES; i++) {
        CPU_SET(i, &mask);
    }
    sched_setaffinity(0, sizeof(cpu_set_t), &mask);

    //forking procs
    int proc_num = -1;
    int p;
    for(int i = 0; i < NUM_CHILD_PROCS; i++) {
        p = fork();
        if (!p) {
            proc_num = i;
            break;
        }
    }

    //parent
    if (p) {
        close(pipefd[1]); //close write end

        int total_calculated = 0;

        //waits until buffer is full, then writes
        int row_start;
        int col_start;
        int row_end;
        int col_end;
        float *buf = malloc(buf_size*sizeof(float));
        while(1) {
            semaphore_reserve(sem_id, 1);
            read(pipefd[0], &row_start, sizeof(int));
            read(pipefd[0], &col_start, sizeof(int));
            read(pipefd[0], &row_end, sizeof(int));
            read(pipefd[0], &col_end, sizeof(int));
            read(pipefd[0], buf, buf_size*sizeof(float));
            int buf_pos = 0;
            for(int i = row_start; i <= row_end; i++) {
                int end = MATRIX_SIZE-1;
                int start = 0;
                if (i == row_start) {
                    start = col_start;
                }
                if (i == row_end) {
                    end = col_end;
                }
                for(int j = start; j <= end; j++) {
                    out[i][j] = buf[buf_pos];
                    buf_pos++;
                    total_calculated++;
                }
            }
            semaphore_release(sem_id, 0);
            if (total_calculated >= MATRIX_SIZE*MATRIX_SIZE) {
                break;
            }
        }
        FILE *out_fd = fopen("mat-out-pipe.csv", "w");
        for(int i = 0; i < MATRIX_SIZE; i++) {
            for(int j = 0; j < MATRIX_SIZE; j++) {
                if (MATRIX_SIZE - 1 == j) {
                    fprintf(out_fd, "%.2f\n", out[i][j]);
                } else {
                    fprintf(out_fd, "%.2f,", out[i][j]);
                }
            }
        }
        gettimeofday(&end, NULL);
        print_stats();
        free(buf);
        fclose(out_fd);
        semctl(sem_id, 0, IPC_RMID);
        semctl(sem_id, 1, IPC_RMID);
    //child
    } else {
        close(pipefd[0]); //close read end

        //each process gets a number of rows from start (inclusive) to end (exclusive), 0-indexed.
        int start;
        int end;
        
        //calculating start and end positions is easy if the matrix size is easily divisible by the number of child processes
        if (MATRIX_SIZE % NUM_CHILD_PROCS == 0) {
            start = proc_num * MATRIX_SIZE/NUM_CHILD_PROCS;
            end = (proc_num+1) * MATRIX_SIZE/NUM_CHILD_PROCS;
        }
        //otherwise it's a bit harder. I do this in the child processes rather than in parent, for speed
        else if (MATRIX_SIZE % NUM_CHILD_PROCS >= proc_num) {
            start = proc_num * (MATRIX_SIZE/NUM_CHILD_PROCS + 1);
            if (MATRIX_SIZE % NUM_CHILD_PROCS == proc_num) {
                end = start + (MATRIX_SIZE/NUM_CHILD_PROCS);
            } else {
                end = start + (MATRIX_SIZE/NUM_CHILD_PROCS + 1);
            }
        } else {
            start = (MATRIX_SIZE % NUM_CHILD_PROCS) * (MATRIX_SIZE/NUM_CHILD_PROCS + 1) + (proc_num - (MATRIX_SIZE % NUM_CHILD_PROCS)) * (MATRIX_SIZE/NUM_CHILD_PROCS);
            end = start + (MATRIX_SIZE/NUM_CHILD_PROCS);
        }

        //calculates until buffer full. then write to pipe for parent process to do its job
        int row_start;
        int col_start;
        int start_flag = 1;
        float *buf = malloc(buf_size*sizeof(float));
        int buf_pos = 0;
        for(int i = start; i < end; i++) {
            for(int j = 0; j < MATRIX_SIZE; j++) {
                if (start_flag) {
                    start_flag = 0;
                    row_start = i;
                    col_start = j;
                }
                float curr = 0;
                for(int k = 0; k < MATRIX_SIZE; k++) {
                    curr+=(arr1[i][k]*arr2[k][j]);
                }
                buf[buf_pos] = curr;
                buf_pos++;
                if (buf_pos == buf_size) {
                    semaphore_reserve(sem_id, 0);
                    write(pipefd[1], &row_start, sizeof(int));
                    write(pipefd[1], &col_start, sizeof(int));
                    write(pipefd[1], &i, sizeof(int));
                    write(pipefd[1], &j, sizeof(int));
                    write(pipefd[1], buf, buf_size*sizeof(float));
                    semaphore_release(sem_id, 1);
                    start_flag = 1;
                    buf_pos = 0;
                }
            }
        }
        //write remaining work in buffer
        int last_i = end-1;
        int last_j = MATRIX_SIZE-1;
        if (buf_pos) {
            semaphore_reserve(sem_id, 0);
            write(pipefd[1], &row_start, sizeof(int));
            write(pipefd[1], &col_start, sizeof(int));
            write(pipefd[1], &last_i, sizeof(int));
            write(pipefd[1], &last_j, sizeof(int));
            write(pipefd[1], buf, buf_size*sizeof(float));
            semaphore_release(sem_id, 1);
        }
        free(buf);
    }
    return 0;
}


