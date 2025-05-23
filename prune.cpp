#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define MAX_READS 20
#define MAX_LEN 100
#define MAX_SUPERSTRING_LEN (MAX_READS * MAX_LEN)


char reads[MAX_READS][MAX_LEN];
int num_reads = 0;
unsigned long long num_solutions = 0ULL;
unsigned long long num_overlap_verifications    = 0ULL;

int best_len = INT_MAX;
char best_result[MAX_LEN * MAX_READS];



int overlap(const char *__restrict__ a, const char *__restrict__ b) {
    int max = strlen(a) < strlen(b) ? strlen(a) : strlen(b);
    for (int i = max; i > 0; i--) {
        if (strncmp(a + strlen(a) - i, b, i) == 0) {
            return i;
        }
    }
    return 0;
}

void build_superstring(char *__restrict__ current, int used[], int level, int curr_len) {
    
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
                build_superstring(temp, used, level + 1, strlen(temp));
            }

            used[i] = 0;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s reads.txt\n", argv[0]);
        return 1;
    }

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

    printf("\n############## String read OK ##############\n");
    printf("\nNum reads: %d\n", num_reads);


    char initial[MAX_LEN * MAX_READS] = "";
    int used[MAX_READS] = {0};

    for (int i = 0; i < num_reads; i++) {
        used[i] = 1;
        strcpy(initial, reads[i]);
        build_superstring(initial, used, 1, strlen(initial));
        used[i] = 0;
    }

    printf("\nBest superstring: %s\n", best_result);
    printf("Length: %d\n", best_len);
    printf("Number of stringcomp calls: %llu \n", num_overlap_verifications);
    printf("Number of complete solutions found: %llu \n", num_solutions);

    return 0;
}