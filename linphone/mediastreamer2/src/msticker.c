/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2006  Simon MORLAT (simon.morlat@linphone.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/



#include "mediastreamer2/msticker.h"


void * ms_ticker_run(void *s);
static uint64_t get_cur_time(void *);

void ms_ticker_start(MSTicker *s){
	s->run=TRUE;
	ms_thread_create(&s->thread,NULL,ms_ticker_run,s);
}


void ms_ticker_init(MSTicker *ticker)
{
	ms_mutex_init(&ticker->lock,NULL);
	ticker->execution_list=NULL;
	ticker->ticks=1;
	ticker->time=0;
	ticker->interval=10;
	ticker->run=FALSE;
	ticker->exec_id=0;
	ticker->get_cur_time_ptr=&get_cur_time;
	ticker->get_cur_time_data=NULL;
#ifdef WIN32_TIMERS
	ticker->TimeEvent=NULL;
#endif
	ticker->name=ms_strdup("MSTicker");
	ms_ticker_start(ticker);
}

MSTicker *ms_ticker_new(){
	MSTicker *obj=(MSTicker *)ms_new(MSTicker,1);
	ms_ticker_init(obj);
	return obj;
}

void ms_ticker_stop(MSTicker *s){
	ms_mutex_lock(&s->lock);
	s->run=FALSE;
	ms_mutex_unlock(&s->lock);
	ms_thread_join(s->thread,NULL);
}

void ms_ticker_set_name(MSTicker *s, const char *name){
	if (s->name) ms_free(s->name);
	s->name=ms_strdup(name);
}

void ms_ticker_uninit(MSTicker *ticker)
{
	ms_ticker_stop(ticker);
	ms_free(ticker->name);
	ms_mutex_destroy(&ticker->lock);
}

void ms_ticker_destroy(MSTicker *ticker){
	ms_ticker_uninit(ticker);
	ms_free(ticker);
}


static MSList *get_sources(MSList *filters){
	MSList *sources=NULL;
	MSFilter *f;
	for(;filters!=NULL;filters=filters->next){
		f=(MSFilter*)filters->data;
		if (f->desc->ninputs==0){
			sources=ms_list_append(sources,f);
		}
	}
	return sources;
}

int ms_ticker_attach(MSTicker *ticker,MSFilter *f)
{
	MSList *sources=NULL;
	MSList *filters=NULL;
	MSList *it;
	
	if (f->ticker!=NULL) {
		ms_message("Filter %s is already being scheduled; nothing to do.",f->desc->name);
		return 0;
	}

	filters=ms_filter_find_neighbours(f);
	sources=get_sources(filters);
	if (sources==NULL){
		ms_fatal("No sources found around filter %s",f->desc->name);
		ms_list_free(filters);
		return -1;
	}
	/*run preprocess on each filter: */
	for(it=filters;it!=NULL;it=it->next)
		ms_filter_preprocess((MSFilter*)it->data,ticker);
	ms_mutex_lock(&ticker->lock);
	ticker->execution_list=ms_list_concat(ticker->execution_list,sources);
	ms_mutex_unlock(&ticker->lock);
	ms_list_free(filters);
	return 0;
}



int ms_ticker_detach(MSTicker *ticker,MSFilter *f){
	MSList *sources=NULL;
	MSList *filters=NULL;
	MSList *it;

	if (f->ticker==NULL) {
		ms_message("Filter %s is not scheduled; nothing to do.",f->desc->name);
		return 0;
	}

	ms_mutex_lock(&ticker->lock);

	filters=ms_filter_find_neighbours(f);
	sources=get_sources(filters);
	if (sources==NULL){
		ms_fatal("No sources found around filter %s",f->desc->name);
		ms_list_free(filters);
		ms_mutex_unlock(&ticker->lock);
		return -1;
	}

	for(it=sources;it!=NULL;it=ms_list_next(it)){
		ticker->execution_list=ms_list_remove(ticker->execution_list,it->data);
	}
	ms_mutex_unlock(&ticker->lock);
	ms_list_for_each(filters,(void (*)(void*))ms_filter_postprocess);
	ms_list_free(filters);
	ms_list_free(sources);
	return 0;
}


static bool_t filter_can_process(MSFilter *f, int tick){
	/* look if filters before this one have run */
	int i;
	MSQueue *l;
	for(i=0;i<f->desc->ninputs;i++){
		l=f->inputs[i];
		if (l!=NULL){
			if (l->prev.filter->last_tick!=tick) return FALSE;
		}
	}
	return TRUE;
}

static void call_process(MSFilter *f){
	bool_t process_done=FALSE;
	if (f->desc->ninputs==0 || f->desc->flags & MS_FILTER_IS_PUMP){
		ms_filter_process(f);
	}else{
		while (ms_filter_inputs_have_data(f)) {
			if (process_done){
				ms_warning("Re-scheduling filter %s: all data should be consumed in one process call, so fix it.",f->desc->name);
			}
			ms_filter_process(f);
			process_done=TRUE;
		}
	}
}

static void run_graph(MSFilter *f, MSTicker *s, MSList **unschedulable, bool_t force_schedule){
	int i;
	MSQueue *l;
	if (f->last_tick!=s->ticks ){
		if (filter_can_process(f,s->ticks) || force_schedule) {
			/* this is a candidate */
			f->last_tick=s->ticks;
			call_process(f);	
			/* now recurse to next filters */		
			for(i=0;i<f->desc->noutputs;i++){
				l=f->outputs[i];
				if (l!=NULL){
					run_graph(l->next.filter,s,unschedulable, force_schedule);
				}
			}
		}else{
			/* this filter has not all inputs that have been filled by filters before it. */
			*unschedulable=ms_list_prepend(*unschedulable,f);
		}
	}
}

static void run_graphs(MSTicker *s, MSList *execution_list, bool_t force_schedule){
	MSList *it;
	MSList *unschedulable=NULL;
	for(it=execution_list;it!=NULL;it=it->next){
		run_graph((MSFilter*)it->data,s,&unschedulable,force_schedule);
	}
	/* filters that are part of a loop haven't been called in process() because one of their input refers to a filter that could not be scheduled (because they could not be scheduled themselves)... Do you understand ?*/
	/* we resolve this by simply assuming that they must be called anyway 
	for the loop to run correctly*/
	/* we just recall run_graphs on them, as if they were source filters */
	if (unschedulable!=NULL) {
		run_graphs(s,unschedulable,TRUE);
		ms_list_free(unschedulable);
	}
}

#ifdef __MACH__
#include <sys/types.h>
#include <sys/timeb.h>
#endif

static uint64_t get_cur_time(void *unused){
#if defined(_WIN32_WCE)
	DWORD timemillis = GetTickCount();
	return timemillis;
#elif defined(WIN32)
	return timeGetTime() ;
#elif defined(__MACH__) && defined(__GNUC__) && (__GNUC__ >= 3)
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec*1000LL) + (tv.tv_usec/1000LL);
#elif defined(__MACH__)
	struct timespec ts;
	struct timeb time_val;
	
	ftime (&time_val);
	ts.tv_sec = time_val.time;
	ts.tv_nsec = time_val.millitm * 1000000;
	return (ts.tv_sec*1000LL) + (ts.tv_nsec/1000000LL);
#else
	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC,&ts)<0){
		ms_fatal("clock_gettime() doesn't work: %s",strerror(errno));
	}
	return (ts.tv_sec*1000LL) + (ts.tv_nsec/1000000LL);
#endif
}

static void sleepMs(int ms){
#ifdef WIN32
	Sleep(ms);
#else
	struct timespec ts;
	ts.tv_sec=0;
	ts.tv_nsec=ms*1000000LL;
	nanosleep(&ts,NULL);
#endif
}

static int set_high_prio(void){
	int precision=2;
	int result=0;
#ifdef WIN32
	MMRESULT mm;
	TIMECAPS ptc;
	mm=timeGetDevCaps(&ptc,sizeof(ptc));
	if (mm==0){
		if (ptc.wPeriodMin<(UINT)precision)
			ptc.wPeriodMin=precision;
		else
			precision = ptc.wPeriodMin;
		mm=timeBeginPeriod(ptc.wPeriodMin);
		if (mm!=TIMERR_NOERROR){
			ms_warning("timeBeginPeriod failed.");
		}
		ms_message("win32 timer resolution set to %i ms",ptc.wPeriodMin);
	}else{
		ms_warning("timeGetDevCaps failed.");
	}

	if(!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST)){
		ms_warning("SetThreadPriority() failed (%d)\n", GetLastError());
	}
#else
	struct sched_param param;
	memset(&param,0,sizeof(param));
#ifdef TARGET_OS_MAC
	int policy=SCHED_RR;
#else
	int policy=SCHED_OTHER;
#endif
	param.sched_priority=sched_get_priority_max(policy);
	if((result=pthread_setschedparam(pthread_self(),policy, &param))) {
		ms_warning("Set sched param failed with error code(%i)\n",result);
	} else {
		ms_message("MS ticker priority set to max");
	}
#endif
	return precision;
}

static void unset_high_prio(int precision){
#ifdef WIN32
	MMRESULT mm;

	if(!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL)){
		ms_warning("SetThreadPriority() failed (%d)\n", GetLastError());
	}

	mm=timeEndPeriod(precision);
#endif
}

#ifndef WIN32_TIMERS

void * ms_ticker_run(void *arg)
{
	uint64_t realtime;
	int64_t diff;
	MSTicker *s=(MSTicker*)arg;
	int lastlate=0;
	int precision=2;
	int late;
	
	precision = set_high_prio();


	s->ticks=1;
	ms_mutex_lock(&s->lock);
	s->orig=s->get_cur_time_ptr(s->get_cur_time_data);

	while(s->run){
		s->ticks++;
		run_graphs(s,s->execution_list,FALSE);
		s->time+=s->interval;
		while(1){
			realtime=s->get_cur_time_ptr(s->get_cur_time_data)-s->orig;
			ms_mutex_unlock(&s->lock);
			diff=s->time-realtime;
			if (diff>0){
				/* sleep until next tick */
				sleepMs((int)diff);
			}else{
				late=(int)-diff;
				if (late>s->interval*5 && late>lastlate){
					ms_warning("%s: We are late of %d miliseconds.",s->name,late);
				}
				lastlate=late;
				break; /*exit the while loop */
			}
			ms_mutex_lock(&s->lock);
		}
		ms_mutex_lock(&s->lock);
	}
	ms_mutex_unlock(&s->lock);
	unset_high_prio(precision);
	ms_message("%s thread exiting",s->name);

	ms_thread_exit(NULL);
	return NULL;
}

#else

void * ms_ticker_run(void *arg)
{
	MSTicker *s=(MSTicker*)arg;
	uint64_t realtime;
	int precision=2;
	UINT timerId;

	precision = set_high_prio();

	s->TimeEvent = CreateEvent (NULL, FALSE, FALSE, NULL);

	s->ticks=1;
	ms_mutex_lock(&s->lock);
	s->orig=s->get_cur_time_ptr(s->get_cur_time_data);

	timerId = timeSetEvent (s->interval, precision, (LPTIMECALLBACK)s->TimeEvent, 0,
				  TIME_PERIODIC | TIME_CALLBACK_EVENT_SET);
	while(s->run){
		DWORD err;

		s->ticks++;
		run_graphs(s,s->execution_list,FALSE);

		/* elapsed time since origin */
		s->time = s->get_cur_time_ptr(s->get_cur_time_data)- s->orig;

		ms_mutex_unlock(&s->lock);
		err = WaitForSingleObject (s->TimeEvent, s->interval*1000 ); /* wake up each diff */
		if (err==WAIT_FAILED)
			ms_message("WaitForSingleObject is failing");

		ms_mutex_lock(&s->lock);
	}
	ms_mutex_unlock(&s->lock);
	timeKillEvent (timerId);
	CloseHandle (s->TimeEvent);
	s->TimeEvent=NULL;
	unset_high_prio(precision);
	ms_message("MSTicker thread exiting");
	ms_thread_exit(NULL);
	return NULL;
}

#endif

void ms_ticker_set_time_func(MSTicker *ticker, MSTickerTimeFunc func, void *user_data){
	if (func==NULL) func=get_cur_time;
	/*ms_mutex_lock(&ticker->lock);*/
	ticker->get_cur_time_ptr=func;
	ticker->get_cur_time_data=user_data;
	/*re-set the origin to take in account that previous function ptr and the
	new one may return different times*/
	ticker->orig=func(user_data)-ticker->time;
	/*ms_mutex_unlock(&ticker->lock);*/
	ms_message("ms_ticker_set_time_func: ticker updated.");
}

static void print_graph(MSFilter *f, MSTicker *s, MSList **unschedulable, bool_t force_schedule){
	int i;
	MSQueue *l;
	if (f->last_tick!=s->ticks ){
		if (filter_can_process(f,s->ticks) || force_schedule) {
			/* this is a candidate */
			f->last_tick=s->ticks;
			ms_message("print_graphs: %s", f->desc->name);
			/* now recurse to next filters */		
			for(i=0;i<f->desc->noutputs;i++){
				l=f->outputs[i];
				if (l!=NULL){
					print_graph(l->next.filter,s,unschedulable, force_schedule);
				}
			}
		}else{
			/* this filter has not all inputs that have been filled by filters before it. */
			*unschedulable=ms_list_prepend(*unschedulable,f);
		}
	}
}

static void print_graphs(MSTicker *s, MSList *execution_list, bool_t force_schedule){
	MSList *it;
	MSList *unschedulable=NULL;
	for(it=execution_list;it!=NULL;it=it->next){
		print_graph((MSFilter*)it->data,s,&unschedulable,force_schedule);
	}
	/* filters that are part of a loop haven't been called in process() because one of their input refers to a filter that could not be scheduled (because they could not be scheduled themselves)... Do you understand ?*/
	/* we resolve this by simply assuming that they must be called anyway 
	for the loop to run correctly*/
	/* we just recall run_graphs on them, as if they were source filters */
	if (unschedulable!=NULL) {
		print_graphs(s,unschedulable,TRUE);
		ms_list_free(unschedulable);
	}
}

void ms_ticker_print_graphs(MSTicker *ticker){
	print_graphs(ticker,ticker->execution_list,FALSE);
}
