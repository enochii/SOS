// /*
//         ano_schedule.h
//         @author: Shi Chenghang 2019/08
//  */


// #include "global.h"
// #include "stdio.h"
// #include "type.h"

// #define RANK0_TICKS 15
// #define RANK1_TICKS 5

// //set the rank and ticks
// PUBLIC void set_proc_rank(struct proc* p, int rank);

// //need to flush? to avoid hunger of proc with low rank?
// PUBLIC int need_to_flush();

// PUBLIC void ano_inc_tick();

// //when the ticks of a proc runs out, we need to choose next proc to run
// PUBLIC void ano_schdule();

// //make the procs into proc_que
// PUBLIC void init_pro_que();

// //also, we need to make the new proc(produced by fork) into our pro_que
// //when we fork a proc, we need to add it to our queue
// PUBLIC void add_to_que(struct  proc* p);

// PUBLIC void del_from_que(struct proc* p);

// PUBLIC void flush();