#include <stdio.h>

#define ARRAY_SIZE 1000

void populate(int a[ARRAY_SIZE], int b[ARRAY_SIZE], int c[ARRAY_SIZE]);

int main(void) {
    int a[ARRAY_SIZE], b[ARRAY_SIZE], c[ARRAY_SIZE];
    populate(a, b, c);
    
    // Stampa qualche valore per verificare il corretto funzionamento
    printf("a[0] = %d, b[0] = %d, c[0] = %d\n", a[0], b[0], c[0]);
    printf("a[%d] = %d, b[%d] = %d, c[%d] = %d\n", ARRAY_SIZE-1, a[ARRAY_SIZE-1], ARRAY_SIZE-1, b[ARRAY_SIZE-1], ARRAY_SIZE-1, c[ARRAY_SIZE-1]);
    
    return 0;
}
