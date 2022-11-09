#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/time.h>

typedef struct Matrix Matrix;
typedef struct Index Index;

struct Matrix
{
    int row;
    int col;
    int** element;
};

Matrix A, B, CPerMatrix, CPerRow, CPerElement;

struct Index
{
    int row;
    int col;
};

// char* a = "a", *b = "b", *c = "c";
// Files names
char a[20] = "a", b[20] = "b", c[20] = "c";

// This function to print matrix in console for debugging
void console(Matrix* matrix)
{
    printf("row = %d col = %d\n", matrix->row, matrix->col);
    for(int i = 0; i < matrix->row; i++) {
        for(int j = 0; j < matrix->col; j++) {
            printf("%d ", matrix->element[i][j]);

        }
        printf("\n");
    }
    printf("\n");
}

// Print matrix in file 
void write_file(Matrix* matrix, char* file, int det)
{
    if(det > 2 || det < 0) return;
    char str[30];
    char *type[] = {"_per_matrix.txt", "_per_row.txt", "_per_element.txt"};
    // printf("%s", type[det]);
    strcpy(str, file);
    strcat(str, type[det]);
    FILE* fd = fopen(str, "w");
    if (fd == NULL){
        printf("File %s Is Not Found\n", str);
        return;
    }
    int row = matrix->row, col = matrix->col;
    for(int i = 0; i < row; i++) {
            for(int j = 0; j < col; j++) {
                fprintf(fd, "%d ", matrix->element[i][j]);
            }
            fprintf(fd, "\n");
        }

    fclose(fd);
}

// Construct matrix and Allocate it in heap
void construct(Matrix* matrix ,int row, int col) 
{
    matrix->row = row;
    matrix->col = col;
    matrix->element = (int **)malloc(matrix->row * sizeof(int*));
    for (int i = 0; i < matrix->row; i++)
        matrix->element[i] = (int*)malloc(matrix->col * sizeof(int));
}

// Take matrix from file path
void read_from(Matrix* matrix, char* file)
{
    char str[20];
    strcpy(str, file);
    strcat(str, ".txt");
    FILE* fd = fopen(str, "r");
    if (fd == NULL){
        printf("File %s Is Not Found\n", str);
        return;
    }
    int element, row, col;
    if(fscanf(fd, "row=%d col=%d\n", &row, &col)) {
        // printf("row = %d col = %d\n", matrix->row, matrix->col);

        construct(matrix, row, col);
        for(int i = 0; i < row; i++) {
            for(int j = 0; j < col; j++) {
                if (fscanf(fd, "%d ", &element)) {
                    (matrix->element)[i][j] = element;
                } else {
                    printf("Error in parsing element\n");
                    return;
                }
            }
        }
    }
    fclose(fd);
}

void parse_argv(int argc, char* argv[])
{
    for(int i = 1; i < argc; i++) {

        if(i == 1)strcpy(a, argv[i]);
        if(i == 2)strcpy(b, argv[i]);
        if(i == 3)strcpy(c, argv[i]);
    }
    printf("Files ==> Matrix1: %s  Matrix2: %s  Output: %s\n", a, b, c);

    read_from(&A, a);
    read_from(&B, b);

    construct(&CPerMatrix, A.row, B.col);
    construct(&CPerRow, A.row, B.col);
    construct(&CPerElement, A.row, B.col);
}

void* thread_per_matrix(void* arg) 
{

    for(int i = 0; i < A.row; i++) {
        for(int j = 0; j < B.col; j++) {
            long long sum = 0;
            for(int k = 0; k < B.row; k++) {
                sum += A.element[i][k] * B.element[k][j];
            }
            CPerMatrix.element[i][j] = sum;
        }
    }
    return (void*) 1;
}

void* thread_per_row(void* arg)
{
    int row = *(int*) arg;
    // printf("row :  %d\n", row);
    for(int i = 0; i < B.col; i++) {
        long long sum = 0;
        for(int j = 0; j < B.row; j++) {
            sum += A.element[row][j] * B.element[j][i];
        }
        CPerRow.element[row][i] = sum;
    }
    free(arg);
    return (void*) 1;
}

void* thread_per_element(void* arg)
{
    int row = ((Index*) arg)->row;
    int col = ((Index*) arg)->col;
    // printf("row : %d ", row);
    // printf("col : %d\n", col);
    long long sum = 0;
    for(int i = 0; i < B.row; i++) {
        sum += A.element[row][i] * B.element[i][col];
    }
    CPerElement.element[row][col] = sum;
    free(arg);
    return (void*) 1;
}

int main (int argc, char* argv[])
{
    // struct timeval stop, start;

    // take the arguments and assign it to file variable if exist
    parse_argv(argc, argv);

    // console(&A);
    // console(&B);

    // Declare threads 
    pthread_t threadPerMatrix, threadPerRow[A.row], threadPerElement[A.row][B.col];

    // gettimeofday(&start, NULL);

    pthread_create(&threadPerMatrix, NULL, &thread_per_matrix, NULL);

    int* rowIndex;
    for(int i = 0; i < A.row; i++) {
        rowIndex = (int*) malloc(sizeof(int));
        *rowIndex = i;
        pthread_create(&threadPerRow[i], NULL, &thread_per_row, rowIndex);
    }

    Index* elementIndex;
    for(int i = 0; i < A.row; i++) {
        for(int j = 0; j < B.col; j++) {
            elementIndex = (Index*) malloc(sizeof(Index));
            elementIndex->row = i;
            elementIndex->col = j;
            pthread_create(&threadPerElement[i][j], NULL, &thread_per_element, elementIndex);
        }
    }
    // // printf("dbg\n");

    if(pthread_join(threadPerMatrix, NULL) != 0) {
        return 1;
    } 
    for(int i = 0; i < A.row; i++) {
        if(pthread_join(threadPerRow[i], NULL) != 0) {
            return 2;
        } 
    }

    for(int i = 0; i < A.row; i++) {
        for(int j = 0; j < B.col; j++) {
            if(pthread_join(threadPerElement[i][j], NULL) != 0) {
                return 3;
            }
        }
    }
    
    // gettimeofday(&stop, NULL);
    // printf("Seconds taken %lu\n", stop.tv_sec - start.tv_sec);
    // printf("Microseconds taken: %lu\n", stop.tv_usec - start.tv_usec);
    
    write_file(&CPerMatrix, c, 0);
    write_file(&CPerRow, c, 1);
    write_file(&CPerElement, c, 2);

    // console(&CPerMatrix);
    // console(&CPerRow);
    // console(&CPerElement);

    pthread_exit(0);
}
