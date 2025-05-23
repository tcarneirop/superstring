#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_READS 10
#define MAX_LEN 100
#define MAX_SUPERSTRING_LEN (MAX_READS * MAX_LEN)

char reads[MAX_READS][MAX_LEN];
int num_reads;

int compute_overlap(const char *a, const char *b) {
    int max = strlen(a) < strlen(b) ? strlen(a) : strlen(b);
    for (int len = max; len > 0; len--) {
        if (strncmp(a + strlen(a) - len, b, len) == 0) {
            return len;
        }
    }
    return 0;
}

void build_overlap_matrix(int overlap[MAX_READS][MAX_READS]) {
    for (int i = 0; i < num_reads; i++) {
        for (int j = 0; j < num_reads; j++) {
            if (i != j) {
                overlap[i][j] = compute_overlap(reads[i], reads[j]);
            } else {
                overlap[i][j] = 0;
            }
        }
    }
}

int used[MAX_READS];
int perm[MAX_READS];
int best_len = MAX_SUPERSTRING_LEN;
char best_superstring[MAX_SUPERSTRING_LEN];

void build_superstring(int overlap[MAX_READS][MAX_READS]) {
    char temp[MAX_SUPERSTRING_LEN];
    strcpy(temp, reads[perm[0]]);
    int len = strlen(temp);

    for (int i = 1; i < num_reads; i++) {
        int prev = perm[i - 1];
        int curr = perm[i];
        int ov = overlap[prev][curr];
        strcat(temp, reads[curr] + ov);
        len += strlen(reads[curr]) - ov;
    }

    if (len < best_len) {
        best_len = len;
        strcpy(best_superstring, temp);
    }
}

void generate_permutations(int depth, int overlap[MAX_READS][MAX_READS]) {
    if (depth == num_reads) {
        build_superstring(overlap);
        return;
    }

    for (int i = 0; i < num_reads; i++) {
        if (!used[i]) {
            used[i] = 1;
            perm[depth] = i;
            generate_permutations(depth + 1, overlap);
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

    int overlap[MAX_READS][MAX_READS];
    build_overlap_matrix(overlap);

    generate_permutations(0, overlap);

    printf("Shortest superstring: %s\n", best_superstring);
    printf("Shortest superstring length: %d\n", best_len);
    return 0;
}