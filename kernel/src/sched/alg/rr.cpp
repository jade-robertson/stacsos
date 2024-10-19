/* SPDX-License-Identifier: MIT */

/* StACSOS - Kernel
 *
 * Copyright (c) University of St Andrews 2024
 * Tom Spink <tcs6@st-andrews.ac.uk>
 */
#include <stacsos/kernel/sched/alg/rr.h>

// *** COURSEWORK NOTE *** //
// This will be where you are implementing your round-robin scheduling algorithm.
// Please edit this file in any way you see fit.  You may also edit the rr.h
// header file.

using namespace stacsos::kernel::sched;
using namespace stacsos::kernel::sched::alg;

void round_robin::add_to_runqueue(tcb &tcb) {
    runqueue_.append(&tcb);
 }

void round_robin::remove_from_runqueue(tcb &tcb) { 
    runqueue_.remove(&tcb);
 }

tcb *round_robin::select_next_task(tcb *current) { 
    if (runqueue_.empty()) {return nullptr;}
    //get tcb at front of list and move it to the back of the queue, then also return it
    tcb* cur = runqueue_.pop();
    runqueue_.append( cur);
    
    return cur;

}
