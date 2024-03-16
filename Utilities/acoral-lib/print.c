#include<print.h>
#include<string.h>
#include <stdio.h>
#include <stdarg.h>
#include "stm32wlxx_hal.h"


int sht_print(const char *fmt, ...)
{
	va_list argp;
    va_start(argp, fmt);
    vprint(fmt, argp);
    va_end(argp);
	return 0;
}

extern UART_HandleTypeDef huart2;
char sht_putchar(char c){
	HAL_UART_Transmit(&huart2, &c, 1, 0);
	return c;
}


void vprint(const char *fmt, va_list argp)
{
    char string[200];
    if(0 < vsprintf(string,fmt,argp)) // build string
    {
        HAL_UART_Transmit(&huart2, (uint8_t*)string, strlen(string), 0xffff); // send message via UART
    }
}



int _read(int fd, char* ptr, int len) {
    
    HAL_StatusTypeDef hstatus;

    hstatus = HAL_UART_Receive(&huart2, (uint8_t *) ptr, 1, HAL_MAX_DELAY);
    if (hstatus == HAL_OK)
    {
        sht_print(ptr);
        return 1;
    }
   
    else
      return 0;
}

int getc(FILE *f)
{
    sht_print("in getc");
    uint8_t  ch;
    HAL_UART_Receive(&huart2,(uint8_t *)&ch, 1, 0xFFFF);
    return  ch;
}
