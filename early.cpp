#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

#define MAX_READS 100
#define MAX_LENGTH 1000

// Global variables
char *reads[MAX_READS];
int read_count = 0;
int best_length = INT_MAX;
char best_superstring[MAX_LENGTH];

// Calculate overlap between two strings a and b
int overlap(char *a, char *b) {
    int len_a = strlen(a);
    int len_b = strlen(b);
    int max_overlap = 0;
    for (int i = 1; i <= len_a && i <= len_b; i++) {
        if (strncmp(a + len_a - i, b, i) == 0) {
            max_overlap = i;
        }
    }
    return max_overlap;
}

// Memoization of overlaps between all pairs of reads
int overlap_cache[MAX_READS][MAX_READS];

void compute_overlap_cache() {
    for (int i = 0; i < read_count; i++) {
        for (int j = 0; j < read_count; j++) {
            if (i != j) {
                overlap_cache[i][j] = overlap(reads[i], reads[j]);
            }
        }
    }
}

// Function to simulate DFS with a stack (non-recursive)
void dfs_iterative() {
    typedef struct {
        char superstring[MAX_LENGTH];
        int used[MAX_READS];
        int depth;
        int current_length;
    } StackFrame;

    // Dynamically allocate memory for the stack
    int stack_capacity = MAX_READS * MAX_READS;
    StackFrame *stack = (StackFrame *)malloc(stack_capacity * sizeof(StackFrame));
    if (!stack) {
        perror("Failed to allocate memory for the stack");
        exit(EXIT_FAILURE);
    }

    int stack_size = 0;

    // Push the initial state onto the stack
    StackFrame initial = { "", { 0 }, 0, 0 };
    stack[stack_size++] = initial;

    while (stack_size > 0) {
        StackFrame current = stack[--stack_size];

        // If we have used all reads, check if the current superstring is the best
        if (current.depth == read_count) {
            if (current.current_length < best_length) {
                best_length = current.current_length;
                strcpy(best_superstring, current.superstring);
            }
            continue;
        }

        // Try adding each unused read to the current superstring
        for (int i = 0; i < read_count; i++) {
            if (current.used[i]) continue;

            // Calculate the overlap between the current superstring and the new read
            int overlap_size = overlap_cache[current.depth][i];
            char new_superstring[MAX_LENGTH];
            strcpy(new_superstring, current.superstring);
            strncat(new_superstring, reads[i] + overlap_size, strlen(reads[i]) - overlap_size);

            int new_length = strlen(new_superstring);

            // Prune if the new superstring exceeds the best length found so far
            if (new_length >= best_length) {
                continue;
            }

            // Estimate the potential remaining overlap reductions
            int remaining_overlap_reduction = 0;
            for (int j = 0; j < read_count; j++) {
                if (!current.used[j]) {
                    remaining_overlap_reduction += overlap_cache[i][j];
                }
            }

            // Only continue exploring if we can still potentially improve the superstring
            if (new_length + remaining_overlap_reduction >= best_length) {
                continue;
            }

            // Mark the current read as used and push the new state onto the stack
            current.used[i] = 1;

            StackFrame new_frame = { "", { 0 }, current.depth + 1, new_length };
            strcpy(new_frame.superstring, new_superstring);
            memcpy(new_frame.used, current.used, sizeof(current.used));

            stack[stack_size++] = new_frame;
        }
    }

    // Free the allocated memory for the stack
    free(stack);
}

int main() {
    // Define reads (example reads you provided)
    reads[0] = "ATTAGACCTG";
    reads[1] = "ATTAGACCTC";
    reads[2] = "CCTGCCGGAG";
    reads[3] = "AGACCTGCCG";
    reads[4] = "GCCGGAATAC";
    reads[5] = "GCCGGAATAG";
    reads[6] = "AGACCTGGCG";
    reads[7] = "AGACGTGCCG";
    reads[8] = "CCTCCCGGAA";
    reads[9] = "GGTGCCGGAA";
    reads[10] = "GGTGGGGGAA";
    reads[11] = "AATGCCGGAA";
    
    read_count = 12;

    // Compute all pairwise overlaps once for efficiency
    compute_overlap_cache();

    // Run the DFS search
    dfs_iterative();

    // Output the best superstring found
    printf("Best superstring: %s\n", best_superstring);
    printf("Length of best superstring: %d\n", best_length);

    return 0;
}