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
#include "proto.h"

PUBLIC int date(struct time *buf) {
    MESSAGE msg;
    msg.type = GET_RTC_TIME;
    msg.BUF = buf;

    send_recv(BOTH, TASK_SYS, &msg);
    assert(msg.type == SYSCALL_RET);

    return 0;
}