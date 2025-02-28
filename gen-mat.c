#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MATRIX_SIZE 100

//generates matrices with values from -100.00 to +100.00
int main() {
    FILE *fp1 = fopen("mat1.csv", "w");
    FILE *fp2 = fopen("mat2.csv", "w");
    srand(time(NULL));
    for(int i = 0; i < MATRIX_SIZE; i++) {
        for(int j = 0; j < MATRIX_SIZE; j++) {
            float rand1 = (float) ((rand() % 20001) - 10000)/100;
            float rand2 = (float) ((rand() % 20001) - 10000)/100;
            if (MATRIX_SIZE - 1 == j) {
                fprintf(fp1, "%.2f\n", rand1);
                fprintf(fp2, "%.2f\n", rand2);
            } else {
                fprintf(fp1, "%.2f,", rand1);
                fprintf(fp2, "%.2f,", rand2);
            }
        }
    }
    fclose(fp1);
    fclose(fp2);
    return 0;
}