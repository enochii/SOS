/*
    ano_schdule.h
 */
#ifndef __ANO_SCHEDULE
#define __ANO_SCHEDULE

#include "type.h"

PUBLIC void init_pro_que();
PUBLIC void add_to_que(struct  proc* p);
PUBLIC void del_from_que(struct proc* p);
PUBLIC void set_proc_rank(struct proc* p, int rank);
PUBLIC void flush_proc();
PUBLIC int need_to_flush();
PUBLIC void ano_inc_tick();
PUBLIC void ano_schdule();

#endif // __ANO_SCHEDULE