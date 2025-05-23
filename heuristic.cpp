#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_READS 12
#define MAX_LEN 100

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




int overlap(const char *a, const char *b) {
    int max = 0;
    int len_a = strlen(a), len_b = strlen(b);
    int min_len = len_a < len_b ? len_a : len_b;

    for (int i = 1; i <= min_len; i++) {
        if (strncmp(a + len_a - i, b, i) == 0)
            max = i;
    }
    return max;
}

void merge(char *result, const char *a, const char *b) {
    int ov = overlap(a, b);
    strcpy(result, a);
    strcat(result, b + ov);
}

void copy_permutation(char **perm, char **src, int n) {
    for (int i = 0; i < n; i++)
        perm[i] = src[i];
}

// Swap helper for permutations
void swap(char **a, char **b) {
    char *tmp = *a;
    *a = *b;
    *b = tmp;
}

int calculate_superstring_length(char **arr, int n) {
    char buffer[MAX_LEN * MAX_READS];
    strcpy(buffer, arr[0]);
    for (int i = 1; i < n; i++) {
        char temp[MAX_LEN * MAX_READS];
        merge(temp, buffer, arr[i]);
        strcpy(buffer, temp);
    }
    return strlen(buffer);
}

void generate_permutations(char **arr, int start, int end, char *shortest, int *min_len) {
    if (start == end) {
        // Calculate the superstring length for this permutation
        int current_len = calculate_superstring_length(arr, end + 1);

        // Prune the branch if current solution is worse
        if (current_len >= *min_len)
            return;

        // Update the shortest solution found
        char buffer[MAX_LEN * MAX_READS];
        strcpy(buffer, arr[0]);
        for (int i = 1; i < end + 1; i++) {
            char temp[MAX_LEN * MAX_READS];
            merge(temp, buffer, arr[i]);
            strcpy(buffer, temp);
        }

        if (current_len < *min_len) {
            strcpy(shortest, buffer);
            *min_len = current_len;
        }

        return;
    }

    for (int i = start; i <= end; i++) {
        swap(&arr[start], &arr[i]);
        generate_permutations(arr, start + 1, end, shortest, min_len);
        swap(&arr[start], &arr[i]);
    }
}

int main() {
    char *perm[MAX_READS];
    copy_permutation(perm, reads, MAX_READS);

    char shortest[MAX_LEN * MAX_READS] = "";
    int min_len = MAX_LEN * MAX_READS;

    generate_permutations(perm, 0, MAX_READS - 1, shortest, &min_len);

    printf("Shortest superstring: %s\n", shortest);
    printf("Length: %d\n", min_len);

    return 0;
}
