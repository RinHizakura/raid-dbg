#include <stdio.h>

int add(int a, int b)
{
    return a + b;
}

int minus(int a, int b)
{
    return a - b;
}

int main()
{
    printf("hello world %d\n", add(2, 3));
    printf("hello happy world %d\n", minus(4, 2));
    return 0;
}
