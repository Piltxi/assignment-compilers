#define ARRAY_SIZE 1000

void populate(int a[ARRAY_SIZE], int b[ARRAY_SIZE], int c[ARRAY_SIZE]) {
    for (int i = 0; i < ARRAY_SIZE; i++) {
        a[i] = i;
    }

    for (int i = 0; i < ARRAY_SIZE; i++) {
        b[i] = a[i] * 2;
    }

    for (int i = 0; i < ARRAY_SIZE; i++) {
        c[i] = b[i] + 1;
    }
}