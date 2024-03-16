/**
 * @file thread.c
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，线程机制相关函数
 * @version 1.0
 * @date 2022-07-08
 * @copyright Copyright (c) 2023
 * @revisionHistory 
 *  <table> 
 *   <tr><th> 版本 <th>作者 <th>日期 <th>修改内容 
 *   <tr><td> 0.1 <td>jivin <td>2010-03-08 <td>Created 
 *   <tr><td> 1.0 <td>王彬浩 <td> 2022-07-08 <td>Standardized 
 *  </table>
 */


#include "hal.h"
#include "lsched.h"
#include "timer.h"
#include "mem.h"
#include "thread.h"
#include "int.h"
#include "policy.h"
#include <stdio.h>

extern sht_list_t sht_res_release_queue;
extern void sht_evt_queue_del(sht_thread_t *thread);
sht_list_t sht_threads_queue; ///<SHT全局所有线程队列
sht_pool_ctrl_t sht_thread_pool_ctrl;

int sht_create_thread(void (*route)(void *args),unsigned int stack_size,void *args,char *name,void *stack,unsigned int sched_policy,void *data){
	sht_thread_t *thread;
        /*分配tcb数据块*/
	thread=sht_alloc_thread();
	if(NULL==thread){
		sht_print("Alloc thread:%s fail\n",name);
		sht_print("No Mem Space or Beyond the max thread\n");
		return -1;
	}
	thread->name=name;
	stack_size=stack_size&(~3);
	thread->stack_size=stack_size;
	if(stack!=NULL)
		thread->stack_buttom=(unsigned int *)stack;
	else
		thread->stack_buttom=NULL;
	thread->policy=sched_policy;
	return sht_policy_thread_init(sched_policy,thread,route,args,data);
}


extern int daemon_id;
void sht_release_thread1(sht_thread_t *thread){//SPG这个和sht_release_thread有什么区别
	sht_list_t *head;
	sht_thread_t *daem;
	thread->state=SHT_THREAD_STATE_EXIT;
	head=&sht_res_release_queue;
	sht_list_add2_tail(&thread->waiting,head);

	daem=(sht_thread_t *)sht_get_res_by_id(daemon_id);
	sht_rdy_thread(daem);
}

void sht_release_thread(sht_res_t *res){
	sht_thread_t *thread;
	thread=(sht_thread_t *)res;
	sht_list_del(&thread->global_list);
	sht_policy_thread_release(thread);
  	sht_free((void *)thread->stack_buttom);
	sht_release_res((sht_res_t *)thread);
}

void sht_suspend_thread(sht_thread_t *thread){
	if(!(SHT_THREAD_STATE_READY&thread->state))
		return;

	long level = sht_enter_critical();
	/**/
	sht_rdyqueue_del(thread);
	sht_exit_critical(level);
	/**/

	
	sht_sched();
	
}

void sht_suspend_self(){
	sht_suspend_thread(sht_cur_thread);
}

void sht_suspend_thread_by_id(unsigned int thread_id){
	sht_thread_t *thread=(sht_thread_t *)sht_get_res_by_id(thread_id);
	sht_suspend_thread(thread);
}

void sht_resume_thread_by_id(unsigned int thread_id){
	sht_thread_t *thread=(sht_thread_t *)sht_get_res_by_id(thread_id);
	sht_resume_thread(thread);
}

void sht_resume_thread(sht_thread_t *thread){
	if(!(thread->state&SHT_THREAD_STATE_SUSPEND))
		return;

	long level = sht_enter_critical();
	/**/
	sht_rdyqueue_add(thread);
	sht_exit_critical(level);
	/**/
	sht_sched();
}

static void sht_delay_thread(sht_thread_t* thread,unsigned int time){
	unsigned int real_ticks;
	if(!sht_list_empty(&thread->waiting)){
		return;	
	}

	/*timeticks*/
	/*real_ticks=time*CFG_TICKS_PER_SEC/1000;*/
	real_ticks = TIME_TO_TICKS(time);
	thread->delay=real_ticks;
	/**/
	sht_delayqueue_add(&time_delay_queue,thread);
}

//有问题，需要改
void sht_delay_self(unsigned int time){
	sht_delay_thread(sht_cur_thread,time);
}

void sht_kill_thread(sht_thread_t *thread){
	sht_evt_t *evt;
	long level = sht_enter_critical();
        /*	*/
        /*	*/
	if(thread->state&SHT_THREAD_STATE_SUSPEND){
		evt=thread->evt;
		/**/
		if(thread->state&SHT_THREAD_STATE_DELAY){
			sht_list_del(&thread->waiting);
		}else
		{
			/**/
			if(evt!=NULL){
				sht_evt_queue_del(thread);
			}
		}
	}
	sht_unrdy_thread(thread);
	sht_release_thread1(thread);
    sht_exit_critical(level);
	sht_sched();
}

void sht_kill_thread_by_id(int id){
	sht_thread_t *thread;
	thread=(sht_thread_t *)sht_get_res_by_id(id);
	sht_kill_thread(thread);
}

void sht_thread_exit(){
        sht_kill_thread(sht_cur_thread);
}

void sht_thread_change_prio(sht_thread_t* thread, unsigned int prio){
	long level = sht_enter_critical();
	if(thread->state&SHT_THREAD_STATE_READY){
		sht_rdyqueue_del(thread);
		thread->prio = prio;
		sht_rdyqueue_add(thread);
	}else
		thread->prio = prio;
	sht_exit_critical(level);
}

void sht_change_prio_self(unsigned int prio){
	sht_thread_change_prio(sht_cur_thread, prio);
}

void sht_thread_change_prio_by_id(unsigned int thread_id, unsigned int prio){
	sht_thread_t *thread=(sht_thread_t *)sht_get_res_by_id(thread_id);
	sht_thread_change_prio(thread, prio);
}

void sht_rdy_thread(sht_thread_t *thread){
	if(!(SHT_THREAD_STATE_SUSPEND&thread->state))
		return;

	sht_rdyqueue_add(thread);
}

void sht_unrdy_thread(sht_thread_t *thread){
	if(!(SHT_THREAD_STATE_READY&thread->state))
		return;

	sht_rdyqueue_del(thread);
}

void sht_thread_move2_tail(sht_thread_t *thread){
	long level = sht_enter_critical();
	sht_unrdy_thread(thread);
	sht_rdy_thread(thread);
	sht_exit_critical(level);
	sht_sched();
}

void sht_thread_move2_tail_by_id(int thread_id){
	sht_thread_t *thread=(sht_thread_t *)sht_get_res_by_id(thread_id);
	sht_thread_move2_tail(thread);
}

sht_thread_t *sht_alloc_thread(){
  	return (sht_thread_t *)sht_get_res(&sht_thread_pool_ctrl);
}

unsigned int sht_thread_init(sht_thread_t *thread,void (*route)(void *args),void (*exit)(void),void *args){
	unsigned int stack_size=thread->stack_size;
	if(thread->stack_buttom==NULL){
		if(stack_size<CFG_MIN_STACK_SIZE)
			stack_size=CFG_MIN_STACK_SIZE;
		thread->stack_buttom=(unsigned int *)sht_malloc(stack_size);
		if(thread->stack_buttom==NULL)
			return SHT_ERR_THREAD_NO_STACK;
		thread->stack_size=stack_size;
	}
    
    thread->stack = (void *)hw_stack_init(route, args,
                                          (unsigned char *)((char *)thread->stack_buttom + stack_size - sizeof(unsigned long)),
                                          exit);
	
	thread->data=NULL;
	thread->state=SHT_THREAD_STATE_SUSPEND;
	
	sht_init_list(&thread->waiting);
	sht_init_list(&thread->ready);
	sht_init_list(&thread->timeout);
	sht_init_list(&thread->global_list);

	long level = sht_enter_critical();
	sht_list_add2_tail(&thread->global_list,&sht_threads_queue);
	sht_exit_critical(level);

	return 0;
}

void sht_thread_pool_init(){
	sht_thread_pool_ctrl.type=SHT_RES_THREAD;
	sht_thread_pool_ctrl.size=sizeof(sht_thread_t);
	if(CFG_MAX_THREAD>20)
		sht_thread_pool_ctrl.num_per_pool=20;
	else
		sht_thread_pool_ctrl.num_per_pool=CFG_MAX_THREAD;
	sht_thread_pool_ctrl.max_pools=CFG_MAX_THREAD/sht_thread_pool_ctrl.num_per_pool;
	sht_pool_ctrl_init(&sht_thread_pool_ctrl);
}

void sht_sched_mechanism_init(){
	sht_thread_pool_init();
	sht_thread_runqueue_init();
	sht_init_list(&sht_threads_queue);
}

void sht_thread_sys_init(){
	sht_sched_mechanism_init();
	sht_sched_policy_init();
}
