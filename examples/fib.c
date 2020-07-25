#include <stdio.h>

int fib(int n) {
     if (n == 0) return 0;
     if (n == 1) return 1;
     return fib(n - 1) + fib(n - 2);
 }

 int main() {
     int c = 20;
     for (int i = 0; i < c; i++)
         printf("fib(%d): %d\n", i, fib(i));
 }

