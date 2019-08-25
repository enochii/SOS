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

#define RANK0_TICKS 15
#define RANK1_TICKS 5

// the rank of queue
#define MAX_RANK 2
#define NULL 0

#define MAX_PROC (NR_TASKS+NR_PROCS)

static struct proc* proc_que[MAX_RANK][MAX_PROC];

static int ano_tick_cnt=0;

#define NEED_TO_FLASH 100

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
    printf("hi! im schedule!\n");
    if(p_proc_ready->ticks==0&&p_proc_ready->rank==0){
        //into the next rank
        set_proc_rank(p_proc_ready,1);
        int pos=p_proc_ready-(&FIRST_PROC);
        assert(proc_que[0][pos]==p_proc_ready);
        proc_que[1][pos]=p_proc_ready;
        proc_que[0][pos]=NULL;
    }
    //change the p_proc_ready
    int i=-1;
    for(i=0;i<MAX_PROC;i++){
        if(proc_que[0][i]!=NULL){
            break;
        }
    }
    //now we found one lucky dog to exec next
    if(i!=MAX_PROC){
        p_proc_ready=proc_que[0][i];
        return;
    }
    //we need to find one in RANK 1
    i=-1;
    for(i=0;i<MAX_PROC;i++){
        if(proc_que[1][i]!=NULL){
            break;
        }
    }
    assert(i!=MAX_PROC);
    p_proc_ready=proc_que[1][i];
}

//make the procs into proc_que
PUBLIC void init_pro_que()
{
    // assert(0);
    printf("Init the proc queue!\n");
    struct proc *p;
    for(int i=0;i<NR_TASKS+NR_NATIVE_PROCS;i++){
        p=proc_que[0][i]=&proc_table[i];
        // p->rank=0;//initial rank
        set_proc_rank(p,0);
    }
}

//also, we need to make the new proc(produced by fork) into our pro_que
//when we fork a proc, we need to add it to our queue
//moreover, it's called by flush too
PUBLIC void add_to_que(struct  proc* p)
{
    printf("ANO_SCHEDULE\n");
    assert(p!=NULL);
    // int insert_pos=-1;
    // for(int i=0;i<MAX_PROC;i++){
    //     if(proc_que[0][i]==NULL){
    //         insert_pos=i;
    //     }
    // }
    // assert(insert_pos!=-1);
    proc_que[0][p-(&FIRST_PROC)]=p;
    set_proc_rank(p,0);
}

PUBLIC void del_from_que(struct proc* p)
{
    assert(p!=NULL);
    int rank=p->rank;
    // int pos=p->que_pos;
    assert(rank==0||rank==1);
    // assert(pos>=0&&pos<MAX_PROC);
    proc_que[p->rank][p-(&FIRST_PROC)]=NULL;
}

// we may need to implement this to avoid hunger
//may need to use a global cnt
PUBLIC void flush()
{
    assert(NEED_TO_FLASH<=ano_tick_cnt);

    ano_tick_cnt=0;//reset
    struct proc* p;
    for(int i=0;i<MAX_PROC;i++){
        p=proc_que[1][i];
        if(p==NULL)continue;
        assert(1==p->rank);
        add_to_que(p);
    }
}

PUBLIC void set_proc_rank(struct proc* p, int rank)
{
    assert(rank==0||rank==1);
    p->rank=rank;
    if(rank==0){
        p->ticks=RANK0_TICKS;
    }else{
        p->ticks=RANK1_TICKS;
    }
}