/*
    @author: Shi Chenghang, 2019/08
    
    ano_schedule, meaning "another shedule", tries to add a multi-queue schdule policy which
    based on the original codes to SOS
 */

// #include "ano_schedule.h"
#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "tty.h"
#include "console.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "global.h"
#include "proto.h"

#include "ano_schdule.h"


// the rank of queue
#define MAX_RANK 3
#define NULL 0

// #define MAX_PROC (NR_TASKS+NR_PROCS)

// static struct proc* proc_que[2][MAX_PROC];

PRIVATE int ano_tick_cnt=0;

#define NEED_TO_FLASH 100

int RANK_TICKS[MAX_RANK]={
    15, 10, 5
};


static int cur_schd_policy=POLICY_PRI;

PUBLIC int need_to_flush()
{
    return ano_tick_cnt>=NEED_TO_FLASH;
}

PUBLIC void ano_inc_tick()
{
    ++ano_tick_cnt;
}

//when the ticks of a proc runs out, we need to choose next proc to run
PUBLIC void ano_schdule()
{
    struct proc*	p;

    if(
        //p_proc_ready!=NULL &&
        p_proc_ready->ticks==0
        &&p_proc_ready->p_flags==0
    ){
        if(p_proc_ready->rank==MAX_RANK-1){
            // p_proc_ready->ticks=RANK_TICKS[p_proc_ready->rank];
        }else{
        //     //lower rank when ticks runs out
            p_proc_ready->rank++;
        //     p_proc_ready->ticks=RANK_TICKS[p_proc_ready->rank];
        }
        p_proc_ready->ticks=RANK_TICKS[p_proc_ready->rank];
    }

    // struct proc* p;
    //rower rank, higher priority

    int min_rank=MAX_RANK;
    while (min_rank==MAX_RANK) {
		for (p = &FIRST_PROC; p <= &LAST_PROC; p++) {
			if (p->p_flags == 0) {
				if(min_rank>p->rank && p->ticks>0){
                    min_rank=p->rank;
                    p_proc_ready=p;
                }
			}
		}
		if (min_rank==MAX_RANK){
            for (p = &FIRST_PROC; p <= &LAST_PROC; p++)
				if (p->p_flags == 0){
                    p->rank=0;
                    p->ticks=RANK_TICKS[0];//dnmd, 0 and p->rank differs????
                }
        }
	}
}

// we may need to implement this to avoid hunger of proc with low rank
PUBLIC void flush_proc()
{
    assert(NEED_TO_FLASH<=ano_tick_cnt);

    ano_tick_cnt=0;//reset
    struct proc* p;
    
    for(p=&FIRST_PROC;p<=&LAST_PROC;p++){
        set_proc_rank(p,0);
    }
}

PUBLIC void set_proc_rank(struct proc* p, int rank)
{
    assert(rank_valid(rank));
    p->rank=rank;
    p->ticks=RANK_TICKS[rank];
}

PUBLIC void reset_proc_ticks(struct proc* p)
{
    assert(p!=NULL);
    // int rk=p->rank;
    // if(rank_valid(rk)){
    //     p->rank=rk=0;//reset
    // }
    p->ticks=RANK_TICKS[p->rank];
}

int rank_valid(int rank)
{
    return rank>=0&&rank<MAX_RANK;
}

// int ticks_of_rank(int rank)
// {
//     return RANK_TICKS[rank];
// }

void change_schd_policy()
{
    cur_schd_policy=1-cur_schd_policy;
}

int get_schd_policy()
{
    return cur_schd_policy;
}