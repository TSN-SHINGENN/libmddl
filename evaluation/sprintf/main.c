#include <stdio.h>

#include "mddl_printf.h"

static int putch(int code)
{
    return fputc(code, stdout);
}


int
main(int ac, char **av)
{
    int a = 12345, b = 987654;
    char C = 'C';
    const char *txt="chat put acbde";
    size_t n;

    double d[] = {
        0.0,
        42.0,
        1234567.89012345,
        0.000000000000018,
        555555.55555555555555555,
        -888888888888888.8888888,
        111111111111111111111111.2222222222
    };

    mddl_printf_init(putch);

    for(n=0;n<100;n++) {
        mddl_printf("%d %c %d %s %d\n", a, n, b, txt, a);
        mddl_printf("%f %f %f\n", d[0], d[1], d[2]);
    }

    return 0;
}
