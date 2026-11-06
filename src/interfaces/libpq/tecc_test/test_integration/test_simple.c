#include <stdio.h>
#include <stdarg.h>

int pg_vprintf(const char *fmt, va_list args)
{
    printf("pg_vprintf called\n");
    return vprintf(fmt, args);
}

int pg_printf(const char *fmt, ...)
{
    va_list args;
    int result;
    
    va_start(args, fmt);
    result = pg_vprintf(fmt, args);
    va_end(args);
    
    return result;
}

int main() {
    printf("Test program starting\n");
    pg_printf("Testing pg_printf\n");
    printf("Test program ending\n");
    return 0;
}
