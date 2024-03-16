#ifndef SHT_PRINT_H
#define SHT_PRINT_H
#include<print.h>
#include<string.h>
#include <stdio.h>
#include <stdarg.h>
int sht_print(const char *fmt, ...);
char sht_putchar(char c);

void vprint(const char *fmt, va_list argp);
void my_printf(const char *fmt, ...); // custom printf() function


#endif
