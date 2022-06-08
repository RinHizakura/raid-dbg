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

void print_int(char *str, int num)
{
    printf("%s %d\n", str, num);
}

int main()
{
    print_int("hello world", add(2, 3));
    print_int("hello happy world", minus(4, 2));
    print_int("ans", calc(4, 2, 3));
    return 0;
}
