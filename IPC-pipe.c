/* CS 519, Spring 2025: Project 1 - Part 2
 * IPC using pipes to perform matrix multiplication.
 * Feel free to extend or change any code or functions below.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/time.h>

//Add all your global variables and definitions here.
#define MATRIX_SIZE 1000
#define NUM_CHILD_PROCS 10

float arr1[MATRIX_SIZE][MATRIX_SIZE];
float arr2[MATRIX_SIZE][MATRIX_SIZE];
float out[MATRIX_SIZE][MATRIX_SIZE];

void semaphore_init(int sem_id, int sem_num, int init_valve)
{

   //Use semctl to initialize a semaphore
}

void semaphore_release(int sem_id, int sem_num)
{
  //Use semop to release a semaphore
}

void semaphore_reserve(int sem_id, int sem_num)
{

  //Use semop to acquire a semaphore
}

/* Time function that calculates time between start and end */
double getdetlatimeofday(struct timeval *begin, struct timeval *end)
{
    return (end->tv_sec + end->tv_usec * 1.0 / 1000000) -
           (begin->tv_sec + begin->tv_usec * 1.0 / 1000000);
}


/* Stats function that prints the time taken and other statistics that you wish
 * to provide.
 */
void print_stats() {



}


int main(int argc, char const *argv[])
{
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

        //waits until buffer is full, then writes
        int row_start;
        int col_start;
        int row_end;
        int col_end;
        float *buf = malloc(512*sizeof(float));
        while(1) {
            read(pipefd[0], &row_start, sizeof(int));
            read(pipefd[0], &col_start, sizeof(int));
            read(pipefd[0], &row_end, sizeof(int));
            read(pipefd[0], &col_end, sizeof(int));
            read(pipefd[0], buf, 512*sizeof(float));
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
                }
            }
        }
        free(buf);

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
        int row_start = start;
        int col_start = 0;
        float *buf = malloc(512*sizeof(float));
        int buf_pos = 0;
        for(int i = start; i < end; i++) {
            for(int j = 0; j < MATRIX_SIZE; j++) {
                float curr = 0;
                for(int k = 0; k < MATRIX_SIZE; k++) {
                    curr+=(arr1[i][k]*arr2[k][j]);
                }
                buf[buf_pos] = curr;
                buf_pos++;
                if (buf_pos == 512) {
                    write(pipefd[1], &row_start, sizeof(int));
                    write(pipefd[1], &col_start, sizeof(int));
                    write(pipefd[1], &i, sizeof(int));
                    write(pipefd[1], &j, sizeof(int));
                    write(pipefd[1], buf, 512*sizeof(float));
                    buf_pos = 0;
                }
            }
        }
        free(buf);
    }
    return 0;
}


