#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <string.h>


//#define MAX_READS 12
//#define MAX_LEN 100
#define STACK_SIZE 100000
#define MAX_READS 1024
#define MAX_LEN 256

char *reads[MAX_READS];
int n_reads = 0;

void read_file(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("File open failed");
        exit(EXIT_FAILURE);
    }

    char buffer[MAX_LEN];
    while (fgets(buffer, sizeof(buffer), fp)) {
        buffer[strcspn(buffer, "\r\n")] = 0;  // Strip newline
        reads[n_reads] = strdup(buffer);
        n_reads++;
    }

    printf("\nNum reads: %d \n", n_reads);
    fclose(fp);
}


/* 
char *reads[MAX_READS] = {
    "ATTAGACCTG",
    "ATTAGACCTC",
    "CCTGCCGGAG",
    "AGACCTGCCG",
    "GCCGGAATAC",
    "GCCGGAATAG",
    "AGACCTGGCG",
    "AGACGTGCCG",
    "CCTCCCGGAA",
    "GGTGCCGGAA",
    "GGTGGGGGAA",
    "AATGCCGGAA"
};
 */



int overlap[MAX_READS][MAX_READS];
int best_len = 1e9;
char best_result[MAX_READS * MAX_LEN];

// Compute maximum overlap between two strings
int compute_overlap(const char *__restrict__ a, const char *__restrict__b) {
    int max = 0;
    int len_a = strlen(a);
    int len_b = strlen(b);
    for (int i = 1; i <= len_a && i <= len_b; i++) {
        if (strncmp(a + len_a - i, b, i) == 0)
            max = i;
    }
    return max;
}

// Build the overlap matrix once
void build_overlap_matrix() {
    for (int i = 0; i < n_reads; i++) {
        for (int j = 0; j < n_reads; j++) {
            if (i != j)
                overlap[i][j] = compute_overlap(reads[i], reads[j]);
            else
                overlap[i][j] = 0;
        }
    }
}

// Greedy upper bound: build an initial (suboptimal) superstring
int greedy_superstring_length() {
    int used[MAX_READS] = {0};
    int curr = 0, total_len = strlen(reads[0]);
    used[0] = 1;

    for (int step = 1; step < n_reads; step++) {
        int max_ov = -1, next = -1;
        for (int i = 0; i < n_reads; i++) {
            if (!used[i] && overlap[curr][i] > max_ov) {
                max_ov = overlap[curr][i];
                next = i;
            }
        }
        total_len += strlen(reads[next]) - max_ov;
        used[next] = 1;
        curr = next;
    }
    return total_len;
}

void dfs(int last, int used_mask, int curr_len, char *curr_str) {
    if (used_mask == (1 << n_reads) - 1) {
        if (curr_len < best_len) {
            best_len = curr_len;
            strcpy(best_result, curr_str);
        }
        return;
    }

    for (int i = 0; i < n_reads; i++) {
        if (!(used_mask & (1 << i))) {
            int ov = overlap[last][i];
            int new_len = curr_len + strlen(reads[i]) - ov;

            if (new_len >= best_len) continue; // pruning

            char new_str[MAX_READS * MAX_LEN];
            strcpy(new_str, curr_str);
            strcat(new_str, reads[i] + ov);

            dfs(i, used_mask | (1 << i), new_len, new_str);
        }
    }
}

void solve() {
    build_overlap_matrix();
    best_len = greedy_superstring_length();  // initial bound

    for (int i = 0; i < n_reads; i++) {
        char curr[MAX_READS * MAX_LEN];
        strcpy(curr, reads[i]);
        dfs(i, 1 << i, strlen(curr), curr);
    }

    printf("Best superstring (%d chars):\n%s\n", best_len, best_result);
}

int main(int argc, char *argv[]){

    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    read_file(argv[1]);
    solve();
    return 0;

}

