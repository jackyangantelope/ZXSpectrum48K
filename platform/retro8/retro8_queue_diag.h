/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef JACK_RETRO8_QUEUE_DIAG_H
#define JACK_RETRO8_QUEUE_DIAG_H
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
typedef struct {
    uint64_t at_us;
    uint32_t frame,late_us,due_run,since_service_us,service_gap_us;
    uint16_t q,pc;
    uint8_t in_frame,buffering,valid;
} r8_queue_event;
typedef struct {
    uint32_t offered,enqueued,dequeued,dropped,zero,enter,leave,flushed;
    uint32_t q0,qmin,qmax,services,gap_max,burst_max;
    uint32_t due,late_frames,late_max,run_max,resets;
    r8_queue_event first_under,first_over;
} r8_queue_window;
typedef struct {
    r8_queue_window w;
    uint64_t last_service_us;
    uint32_t run,win,late_us,due_run,service_gap_us;
    bool started,active,in_frame;
} r8_queue_diag;
static inline uint32_t r8_queue_u32(uint64_t n){return n>UINT32_MAX?UINT32_MAX:(uint32_t)n;}
static inline void r8_queue_reset(r8_queue_diag* d,uint64_t now,unsigned q,bool active,bool buffering){
    uint32_t run=d->run+1;memset(d,0,sizeof *d);
    d->run=run;d->win=1;d->started=true;d->active=active;d->last_service_us=now;
    d->w.q0=d->w.qmin=d->w.qmax=q;
    d->w.enter=active&&buffering;
}
static inline void r8_queue_capture(r8_queue_diag* d,bool overflow,uint64_t now,
        uint32_t frame,unsigned q,unsigned pc,bool buffering){
    r8_queue_event* e=overflow?&d->w.first_over:&d->w.first_under;
    if(e->valid)return;
    *e=(r8_queue_event){.at_us=now,.frame=frame,.late_us=d->late_us,
        .due_run=d->due_run,.since_service_us=r8_queue_u32(now-d->last_service_us),.service_gap_us=d->service_gap_us,
        .q=(uint16_t)q,.pc=(uint16_t)pc,.in_frame=d->in_frame,
        .buffering=buffering,.valid=1};
}
static inline void r8_queue_service_begin(r8_queue_diag* d,uint64_t now){
    if(!d->active)return;
    uint32_t gap=r8_queue_u32(now-d->last_service_us);
    d->service_gap_us=gap;
    if(gap>d->w.gap_max)d->w.gap_max=gap;
    d->last_service_us=now;d->w.services++;
}
static inline void r8_queue_service_end(r8_queue_diag* d,unsigned before,unsigned after,uint32_t sent){
    if(!d->active)return;
    uint32_t removed=before-after;
    d->w.dequeued+=removed;d->w.zero+=sent-removed;
    if(after<d->w.qmin)d->w.qmin=after;
    if(sent>d->w.burst_max)d->w.burst_max=sent;
}
static inline void r8_queue_flush(r8_queue_diag* d,unsigned q){
    if(!d->started)return;
    d->w.flushed+=q;d->w.qmin=0;d->active=false;d->in_frame=false;d->due_run=0;
}
static inline void r8_queue_due(r8_queue_diag* d,uint64_t now,uint64_t next,uint64_t step,uint64_t clock){
    if(!d->active)return;
    uint64_t late=now*clock-next;
    d->late_us=r8_queue_u32(late/clock);
    d->due_run++;d->w.due++;
    if(late>=step)d->w.late_frames++;
    if(d->late_us>d->w.late_max)d->w.late_max=d->late_us;
    if(d->due_run>d->w.run_max)d->w.run_max=d->due_run;
}
static inline bool r8_queue_valid(const r8_queue_diag* d,unsigned q){
    return (uint32_t)(d->w.q0+d->w.enqueued-d->w.dequeued-d->w.flushed)==q &&
        d->w.offered==(uint32_t)(d->w.enqueued+d->w.dropped);
}
static void r8_queue_event_report(const r8_queue_diag* d,const r8_queue_event* e,char kind){
    if(!e->valid)return;
    r8_profile_printf("R8_QEV run=%lu win=%lu kind=%c at_us=%llu frame=%lu q=%u pc=%04x in_frame=%u buffering=%u late_us=%lu due_run=%lu since_svc_us=%lu svc_gap_us=%lu\n",
        (unsigned long)d->run,(unsigned long)d->win,kind,(unsigned long long)e->at_us,
        (unsigned long)e->frame,(unsigned)e->q,(unsigned)e->pc,(unsigned)e->in_frame,
        (unsigned)e->buffering,(unsigned long)e->late_us,(unsigned long)e->due_run,
        (unsigned long)e->since_service_us,(unsigned long)e->service_gap_us);
}
static void r8_queue_report(r8_queue_diag* d,unsigned q,uint32_t frame){
    if(!d->started)return;
    const r8_queue_window* w=&d->w;
    r8_profile_printf("R8_Q run=%lu win=%lu frame=%lu active=%u q0=%lu q=%u offered=%lu enq=%lu deq=%lu drop=%lu zero=%lu enter=%lu leave=%lu flush=%lu min=%lu max=%lu valid=%u\n",
        (unsigned long)d->run,(unsigned long)d->win,(unsigned long)frame,(unsigned)d->active,
        (unsigned long)w->q0,q,(unsigned long)w->offered,(unsigned long)w->enqueued,
        (unsigned long)w->dequeued,(unsigned long)w->dropped,(unsigned long)w->zero,
        (unsigned long)w->enter,(unsigned long)w->leave,(unsigned long)w->flushed,
        (unsigned long)w->qmin,(unsigned long)w->qmax,(unsigned)r8_queue_valid(d,q));
    r8_profile_printf("R8_QS run=%lu win=%lu services=%lu gap_max_us=%lu burst_max=%lu due=%lu late_frames=%lu late_max_us=%lu due_run_max=%lu resets=%lu\n",
        (unsigned long)d->run,(unsigned long)d->win,(unsigned long)w->services,
        (unsigned long)w->gap_max,(unsigned long)w->burst_max,(unsigned long)w->due,
        (unsigned long)w->late_frames,(unsigned long)w->late_max,(unsigned long)w->run_max,
        (unsigned long)w->resets);
    r8_queue_event_report(d,&w->first_under,'U');
    r8_queue_event_report(d,&w->first_over,'O');
    memset(&d->w,0,sizeof d->w);d->w.q0=d->w.qmin=d->w.qmax=q;d->win++;
}
#endif
