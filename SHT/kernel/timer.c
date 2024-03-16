/**
 * @file timer.c
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，定时器
 * @version 1.0
 * @date 2023-04-21
 * @copyright Copyright (c) 2023
 * @revisionHistory
 *  <table>
 *   <tr><th> 版本 <th>作者 <th>日期 <th>修改内容
 *   <tr><td> 0.1 <td>jivin <td>2010-03-08 <td>Created
 *   <tr><td> 1.0 <td>王彬浩 <td> 2023-04-21 <td>Standardized
 *  </table>
 */

#include "hal.h"
#include "policy.h"
#include "comm_thrd.h"
#include "timer.h"
#include "int.h"
#include "lsched.h"
#include "list.h"
#include "stm32wlxx_hal.h"
#include "user.h"
#include <stdbool.h>

sht_list_t time_delay_queue; ///<SHT线程延时队列，调用线程delay相关函数的线程都会被加到这个队列上，等待一段具体时间后重新被唤醒
/*----------------*/
/*  延时处理队列timeout*/
/*  pegasus   0719*/
/*----------------*/
sht_list_t timeout_queue; ///<SHT获取资源（互斥量等）超时等待队列，即在timeout时间内获取即成功，否则超时失败
static unsigned int ticks;
void sht_time_sys_init(){
  	sht_init_list(&time_delay_queue);

	/*---------------*/
	/*  新增延时初始化 timeout_queue*/
	/*  pegasus   0719*/
	/*---------------*/
	sht_init_list(&timeout_queue);
}


unsigned int sht_get_ticks(){
	return ticks;
}

void sht_set_ticks(unsigned int time){
  	ticks=time;
}

extern HAL_TickFreqTypeDef uwTickFreq;
void sht_ticks_init(){
	HAL_SYSTICK_Config(SystemCoreClock / (1000U / uwTickFreq)); //SPG 用这个
}


void sht_ticks_entry(int vector){

    ticks++;
	if(sht_start_sched==true){
		time_delay_deal();
		sht_policy_delay_deal();
		timeout_delay_deal();
	}
}

void sht_delayqueue_add(sht_list_t *queue, sht_thread_t *new){
	sht_list_t   *tmp, *head;
	sht_thread_t *thread;
	int  delay2;
	int  delay= new->delay;
	head=queue;
	long level = sht_enter_critical();
	/*这里采用关ticks中断，不用关中断，是为了减少最大关中断时间，下面是个链表，时间不确定。*/
	/*这里可以看出，延时函数会有些误差，因为ticks中断可能被延迟*/

	new->state|=SHT_THREAD_STATE_DELAY;
	for (tmp=head->next;delay2=delay,tmp!=head; tmp=tmp->next){
		thread = list_entry (tmp, sht_thread_t, waiting);
		delay  = delay-thread->delay;
		if (delay < 0)
			break;
	}
	new->delay = delay2;
	sht_list_add(&new->waiting,tmp->prev);
	/* 插入等待任务后，后继等待任务时间处理*/
	if(tmp != head){
		thread = list_entry(tmp, sht_thread_t, waiting);
		thread->delay-=delay2;
	}
	sht_unrdy_thread(new);

	sht_exit_critical(level);

	sht_sched();
	
	return;
}

void time_delay_deal(){
	sht_list_t   *tmp,*tmp1,*head;
	sht_thread_t *thread;
	head = &time_delay_queue;
	if(sht_list_empty(head))
	  	return;
	thread=list_entry(head->next,sht_thread_t,waiting);
	thread->delay--;
	for(tmp=head->next;tmp!=head;){
		thread=list_entry(tmp,sht_thread_t,waiting);
		if(thread->delay>0)
		    break;
		/*防止add判断delay时取下thread*/

		tmp1=tmp->next;
		sht_list_del(&thread->waiting);

		tmp=tmp1;
		thread->state&=~SHT_THREAD_STATE_DELAY;
		sht_rdy_thread(thread);
	}
}

void timeout_queue_add(sht_thread_t *new)
{
	sht_list_t   *tmp ,*head;
	sht_thread_t *thread;
	int  delay2;
	int  delay= new->delay; //SPG用tcb的delay，delay线程也用delay成员，冲突？
	head=&timeout_queue;
	long level = sht_enter_critical();

	for (tmp=head->next;delay2=delay,tmp!=head; tmp=tmp->next){
		thread = list_entry (tmp, sht_thread_t, timeout);
		delay  = delay-thread->delay;
		if (delay < 0)
			break;
	}
	new->delay = delay2;
	sht_list_add(&new->timeout,tmp->prev);
	/* 插入等待任务后，后继等待任务时间处理*/
	if(tmp != head){
		thread = list_entry(tmp, sht_thread_t, timeout);
		thread->delay-=delay2;
	}

	sht_exit_critical(level);
	return;
}

void timeout_queue_del(sht_thread_t *new)
{
	if (sht_list_empty(&new->timeout))
		return;
	sht_list_del(&new->timeout);
	return;
}

void timeout_delay_deal()
{
	sht_list_t *tmp, *tmp1, *head;
	sht_thread_t  *thread;

	head = &timeout_queue;
	if(sht_list_empty(head))
	{
	  	return;
	}

	thread=list_entry(head->next,sht_thread_t,timeout);
	if (thread->delay>0)
		thread->delay--;
	for(tmp=head->next;tmp!=head;)
	{
		thread=list_entry(tmp,sht_thread_t,timeout);
		if(thread->delay>0)
		    break;

		tmp1=tmp->next;
		sht_list_del(&thread->timeout);

		tmp=tmp1;
		/*thread->state*/
		sht_rdy_thread(thread);
	}
}


