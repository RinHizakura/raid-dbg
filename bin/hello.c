#include <stdio.h>

int add(int a, int b)
{
    return a + b;
}

int minus(int a, int b)
{
    return a - b;
}

int mul(int a, int b)
{
    return a * b;
}

int calc(int a, int b, int c)
{
    int x = add(a, b);
    int y = minus(b, c);
    return mul(x, y);
}

int main()
{
    printf("hello world %d\n", add(2, 3));
    printf("hello happy world %d\n", minus(4, 2));
    printf("ans %d\n", calc(4, 2, 3));
    return 0;
}
