#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

// #define MAX_LEN 128

int scanf(const char *fmt, ...)
{
    int i;
	char buf[STR_DEFAULT_LEN];

    int c = read(0, buf, STR_DEFAULT_LEN);
    buf[c]=0;
    // printf("???:");
    // printf(buf);
    // printf("\n");

	va_list arg = (va_list)((char*)(&fmt) + 4);        /* 4 是参数 fmt 所占堆栈中的大小 */
	i = nscanf(buf, fmt, arg);
    // printf("arg: %d", (int)arg);
	
    return i;
}