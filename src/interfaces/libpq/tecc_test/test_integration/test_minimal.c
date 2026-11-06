#include <stdio.h>
#include <stdarg.h>

int pg_vsnprintf(char *str, size_t count, const char *fmt, va_list args)
{
    const char *msg = "[pg_vsnprintf]";
    size_t msg_len = 14;
    size_t copy_len = msg_len < count - 1 ? msg_len : count - 1;
    
    if (copy_len > 0) {
        for (size_t i = 0; i < copy_len; i++) {
            str[i] = msg[i];
        }
        str[copy_len] = '\0';
    }
    
    return (int)copy_len;
}

int pg_vprintf(const char *fmt, va_list args)
{
    char buffer[1024];
    int result = pg_vsnprintf(buffer, sizeof(buffer), fmt, args);
    if (result >= 0 && result < (int)sizeof(buffer)) {
        fputs(buffer, stdout);
    }
    return result;
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
    printf("Test program starting (using real printf)\n");
    pg_printf("Testing pg_printf\n");
    printf("Test program ending (using real printf)\n");
    return 0;
}
