/* CS 519, Spring 2025: Project 1 - Part 2
 * IPC using shared memory to perform matrix multiplication.
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
#include <sys/wait.h>
#include <sched.h>

//Add all your global variables and definitions here.
#define MATRIX_SIZE 1000
#define NUM_CHILD_PROCS 8
#define NUM_CORES 8

float arr1[MATRIX_SIZE][MATRIX_SIZE];
float arr2[MATRIX_SIZE][MATRIX_SIZE];

struct timeval begin;
struct timeval end;

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

    //setting up shmem
    key_t key = 1;
    int shmid = shmget(key, MATRIX_SIZE*MATRIX_SIZE*sizeof(float), 0666 | IPC_CREAT);
    float *out = shmat(shmid, NULL, 0);

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
        for(int i = 0; i < NUM_CHILD_PROCS; i++) {
            wait(NULL);
        }

        FILE *out_fd = fopen("mat-out-shmem.csv", "w");
        for(int i = 0; i < MATRIX_SIZE; i++) {
            for(int j = 0; j < MATRIX_SIZE; j++) {
                if (MATRIX_SIZE - 1 == j) {
                    fprintf(out_fd, "%.2f\n", out[i*MATRIX_SIZE + j]);
                } else {
                    fprintf(out_fd, "%.2f,", out[i* MATRIX_SIZE + j]);
                }
            }
        }
        gettimeofday(&end, NULL);
        print_stats();
        fclose(out_fd);
        shmdt(out);
        shmctl(shmid, IPC_RMID, NULL);
    //child
    } else {
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

        int row_start;
        int col_start;
        for(int i = start; i < end; i++) {
            for(int j = 0; j < MATRIX_SIZE; j++) {
                float curr = 0;
                for(int k = 0; k < MATRIX_SIZE; k++) {
                    curr+=(arr1[i][k]*arr2[k][j]);
                }
                out[i*MATRIX_SIZE + j] = curr;
            }
        }
    }
    shmdt(out);
    return 0;
}


