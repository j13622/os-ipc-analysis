#include <stdio.h>
#include <math.h>

#ifndef MATRIX_SIZE
#define MATRIX_SIZE 1000
#endif

float arr1[MATRIX_SIZE][MATRIX_SIZE];
float arr2[MATRIX_SIZE][MATRIX_SIZE];
float pipeArr[MATRIX_SIZE][MATRIX_SIZE];
float shmemArr[MATRIX_SIZE][MATRIX_SIZE];
float out[MATRIX_SIZE][MATRIX_SIZE];

int main(int argc, char const *argv[]) {
    FILE *fp1 = fopen("mat1.csv", "r");
    FILE *fp2 = fopen("mat2.csv", "r");
    FILE *fp3 = fopen("mat-out-pipe.csv", "r");
    FILE *fp4 = fopen("mat-out-shmem.csv", "r");
    for(int i = 0; i < MATRIX_SIZE; i++) {
        for(int j = 0; j < MATRIX_SIZE; j++) {
            float f; 
            fscanf(fp1, "%f,", &f);
            arr1[i][j] = f;
            fscanf(fp2, "%f,", &f);
            arr2[i][j] = f;
            fscanf(fp3, "%f,", &f);
            pipeArr[i][j] = f;
            fscanf(fp4, "%f,", &f);
            shmemArr[i][j] = f;    
        }
    }
    fclose(fp1);
    fclose(fp2);
    fclose(fp3);
    fclose(fp4);
    
    for(int i = 0; i < MATRIX_SIZE; i++) {
        for(int j = 0; j < MATRIX_SIZE; j++) {
            float curr = 0;
            for(int k = 0; k < MATRIX_SIZE; k++) {
                curr+=(arr1[i][k]*arr2[k][j]);
            }
            out[i][j] = curr;
        }
    }

    //there's weird errors with floating point precision so I just output to verify.csv with 2 decimal places and then read back
    FILE *out_fd = fopen("mat-out-verify.csv", "w");
    for(int i = 0; i < MATRIX_SIZE; i++) {
        for(int j = 0; j < MATRIX_SIZE; j++) {
            if (MATRIX_SIZE - 1 == j) {
                fprintf(out_fd, "%.2f\n", out[i][j]);
            } else {
                fprintf(out_fd, "%.2f,", out[i][j]);
            }
        }
    }
    fclose(out_fd);
    FILE *out_fd2 = fopen("mat-out-verify.csv", "r");
    for(int i = 0; i < MATRIX_SIZE; i++) {
        for(int j = 0; j < MATRIX_SIZE; j++) {
            float f; 
            fscanf(out_fd, "%f,", &f);
            out[i][j] = f;
        }
    }
    fclose(out_fd2);
    
    int pipeGood = 1;
    int shmemGood = 1;
    for(int i = 0; i < MATRIX_SIZE; i++) {
        for(int j = 0; j < MATRIX_SIZE; j++) {
            //rounding to 2 decimal places
            if (out[i][j] != shmemArr[i][j] && shmemGood) {
                shmemGood = 0;
                printf("IPC-shmem verification failed\n");
                printf("%f %f\n", out[i][j], shmemArr[i][j]);
                if (!pipeGood) {
                    break;
                }
            }
            if (out[i][j] != pipeArr[i][j] && pipeGood) {
                pipeGood = 0;
                printf("IPC-pipe verification failed\n");
                printf("%f %f\n", out[i][j], pipeArr[i][j]);
                if (!shmemGood) {
                    break;
                }
            }
        }
    }
    if (shmemGood) {
        printf("IPC-shmem verification succeeded\n");
    }
    if (pipeGood) {
        printf("IPC-pipe verification succeeded\n");
    }
}