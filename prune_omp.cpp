#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <string.h>
#include <time.h> 


#define MAX_READS 20
#define MAX_LEN 100
#define POOL_SIZE 10000
#define MAX_SUPERSTRING_LEN (MAX_READS * MAX_LEN)


char reads[MAX_READS][MAX_LEN];

int num_reads = 0;
unsigned int num_subproblems = 0;
unsigned long long num_solutions = 0ULL;
unsigned long long num_overlap_verifications    = 0ULL;

int best_len = INT_MAX;
char best_result[MAX_LEN * MAX_READS];


typedef struct subproblem{
    char current[MAX_LEN * MAX_READS] = "";
    int used[MAX_READS] = {0};
} Subproblems;


int overlap(const char *__restrict__ a, const char *__restrict__ b) {
    int max = strlen(a) < strlen(b) ? strlen(a) : strlen(b);
    for (int i = max; i > 0; i--) {
        if (strncmp(a + strlen(a) - i, b, i) == 0) {
            return i;
        }
    }
    return 0;
}


void generate_initial_load_get_subproblems(char *__restrict__ current,  
    int *__restrict__ used, const int level, 
    const int cutoff_level, const int curr_len, 
    Subproblems *__restrict__ pool_subproblems) {
    
    if (level == cutoff_level) {
       
        memcpy(pool_subproblems[num_subproblems].current,current,sizeof(char)*curr_len);
        memcpy(pool_subproblems[num_subproblems].used,used,sizeof(int)*MAX_READS);
        ++num_subproblems;
        return;
    }

    for (int i = 0; i < num_reads; i++) {
        if (!used[i]) {

            used[i] = 1;

            char temp[MAX_LEN * MAX_READS];
            ++num_overlap_verifications;
            int ov = overlap(current, reads[i]);

            strcpy(temp, current);
            strcat(temp, reads[i] + ov);

            // Prune: if current length is already worse than best
            if ((int)strlen(temp) < best_len) {
                generate_initial_load_get_subproblems(temp, used, level + 1, cutoff_level, strlen(temp),pool_subproblems);
            }

            used[i] = 0;
        }
    }
}



void solve_build_superstring(char *__restrict__ current, int *__restrict__ used, const int level, const int curr_len) {
    
    if (level == num_reads) {
        ++num_solutions;
        if (curr_len < best_len) {
            best_len = curr_len;
            strcpy(best_result, current);
        }
        return;
    }

    for (int i = 0; i < num_reads; i++) {
        if (!used[i]) {
            used[i] = 1;

            char temp[MAX_LEN * MAX_READS];
            ++num_overlap_verifications;
            int ov = overlap(current, reads[i]);

            strcpy(temp, current);
            strcat(temp, reads[i] + ov);

            // Prune: if current length is already worse than best
            if ((int)strlen(temp) < best_len) {
                solve_build_superstring(temp, used, level + 1, strlen(temp));
            }

            used[i] = 0;
        }
    }
}

void solve_launch_parallel_search(Subproblems *__restrict__ pool_of_subproblems, 
    const unsigned int num_subproblems, const int cutoff_level){

    for(int sub = 0; sub<num_subproblems; ++sub){
        solve_build_superstring( pool_of_subproblems[sub].current, pool_of_subproblems[sub].used, cutoff_level, strlen(pool_of_subproblems[sub].current));
    }

}


Subproblems* generate_initial_load_start_pool(const int cutoff_level){

    Subproblems *pool_of_subproblems = (Subproblems*)malloc(POOL_SIZE * sizeof(Subproblems));
    char initial[MAX_LEN * MAX_READS] = "";
    int used[MAX_READS] = {0};
    
    for (int i = 0; i < num_reads; i++) {
        used[i] = 1;
        strcpy(initial, reads[i]);
        generate_initial_load_get_subproblems(initial, used, 1, cutoff_level, strlen(initial),pool_of_subproblems);
        used[i] = 0;
    }
    return pool_of_subproblems;
}



int main(int argc, char *argv[]) {

    if (argc != 3) {
        fprintf(stderr, "Usage: %s reads.txt\n cutoff_level", argv[0]);
        return 1;
    }

    int cutoff_level = atoi(argv[2]);


    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    while (fgets(reads[num_reads], MAX_LEN, fp)) {
        size_t len = strlen(reads[num_reads]);
        if (len > 0 && reads[num_reads][len - 1] == '\n') {
            reads[num_reads][len - 1] = '\0';
        }
        num_reads++;
    }
    fclose(fp);

    printf("\n############## Problem Read -- OK ##############\n");
    printf("\nNum reads: %d\n", num_reads);
    
    
    Subproblems *pool_of_subproblems = generate_initial_load_start_pool(atoi(argv[2]));

    printf("\nCutoff depth: %d, Num subproblems: %u", cutoff_level, num_subproblems);

    solve_launch_parallel_search(pool_of_subproblems, num_subproblems,  cutoff_level);

    printf("\nBest superstring: %s\n", best_result);
    printf("Length: %d\n", best_len);
    printf("Number of stringcomp calls: %llu \n", num_overlap_verifications);
    printf("Number of complete solutions found: %llu \n", num_solutions);

    free(pool_of_subproblems);
    return 0;
}