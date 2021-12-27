#include <stdlib.h>
#include <stdio.h>

#define N 10000000

void rec(int n) {
    if (n < N)
        rec(n+1);
    else {
        printf("'rec' made it to: %d\n", n);
    }
}

int main() {
    // rec(0);
    int i;
    for (i = 0; i < N; i++);
    printf("'for loop' made it to : %d\n", i);
}