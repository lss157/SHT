/**
 * @file sched.c
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，SHT调度相关函数
 * @version 1.0
 * @date 2022-07-28
 * @copyright Copyright (c) 2023
 * @revisionHistory
 *  <table>
 *   <tr><th> 版本 <th>作者 <th>日期 <th>修改内容
 *   <tr><td> 0.1 <td>jivin <td>2010-03-08 <td>Created
 *   <tr><td> 1.0 <td>王彬浩 <td> 2022-07-28 <td>Standardized
 *  </table>
 */

#include "hal.h"
#include "thread.h"
#include "int.h"
#include "lsched.h"
#include "list.h"
#include "bitops.h"
#include <stdbool.h>
#include "stm32wlxx_hal.h"

unsigned char sht_need_sched; ///< SHT是否需要调度标志，仅当SHT就绪队列sht_ready_queues有线程加入或被取下时，该标志被置为true；仅当SHT在调度线程时，该标志位被置为false
unsigned char sched_lock = 1;	 ///< SHT初始化完成之前，调度都是被上锁的，即不允许调度。
sht_thread_t *sht_cur_thread;		///<sht当前运行的线程
sht_thread_t *sht_ready_thread;	///<下一个将被调度运行的线程

sht_rdy_queue_t sht_ready_queues; ///<SHT就绪队列
/* 之前是static sht_rdy_queue_t sht_ready_queues[HAL_MAX_CPU],*/
/*改成static sht_rdy_queue_t* sht_ready_queues 后，由于static存在，会把这个指针变量初始化为0，*/
/*后面初始化就绪队列的操作就会修改0地址的异常向量表，导致时钟中断被打开后，无法正常进入中断服务*/
/*很恐怖的bug！！谨此记录*/

void sht_prio_queue_add(sht_rdy_queue_t *array, unsigned char prio, sht_list_t *list)
{
	sht_list_t *queue;
	sht_list_t *head;
	array->num++;
	queue = array->queue + prio;
	head = queue;
	sht_list_add2_tail(list, head);
	sht_set_bit(prio, array->bitmap);
}

void sht_prio_queue_del(sht_rdy_queue_t *array, unsigned char prio, sht_list_t *list)
{
	sht_list_t *queue;
	sht_list_t *head;
	queue = array->queue + prio;
	head = queue;
	array->num--;
	sht_list_del(list);
	if (sht_list_empty(head))
		sht_clear_bit(prio, array->bitmap);
}

unsigned int sht_get_highprio(sht_rdy_queue_t *array)
{
	return sht_find_first_bit(array->bitmap, PRIO_BITMAP_SIZE);
}

void sht_prio_queue_init(sht_rdy_queue_t *array)
{
	unsigned char i;
	sht_list_t *queue;
	sht_list_t *head;
	array->num = 0;
	for (i = 0; i < PRIO_BITMAP_SIZE; i++)
		array->bitmap[i] = 0;
	for (i = 0; i < SHT_MAX_PRIO_NUM; i++)
	{
		queue = array->queue + i;
		head = queue;
		sht_init_list(head);
	}
}

void sht_sched_init()
{
	sched_lock = 0;
	sht_set_need_sched(false);
}


void sht_set_running_thread(sht_thread_t *thread)
{
	sht_cur_thread->state &= ~SHT_THREAD_STATE_RUNNING;
	thread->state |= SHT_THREAD_STATE_RUNNING;
	sht_cur_thread = thread;
}


void sht_thread_runqueue_init() 
{
	sht_rdy_queue_t *rdy_queue;
	/*初始化每个核上的优先级队列*/
	rdy_queue = &sht_ready_queues;
	sht_prio_queue_init(rdy_queue);
}

void sht_rdyqueue_add(sht_thread_t *thread)
{
	sht_rdy_queue_t *rdy_queue;
	rdy_queue = &sht_ready_queues;
	sht_prio_queue_add(rdy_queue, thread->prio, &thread->ready);
	thread->state &= ~SHT_THREAD_STATE_SUSPEND;
	thread->state |= SHT_THREAD_STATE_READY;
	sht_set_need_sched(true);
}

void sht_rdyqueue_del(sht_thread_t *thread)
{
	sht_rdy_queue_t *rdy_queue;
	rdy_queue = &sht_ready_queues;
	sht_prio_queue_del(rdy_queue, thread->prio, &thread->ready);
	thread->state &= ~SHT_THREAD_STATE_READY;
	thread->state &= ~SHT_THREAD_STATE_RUNNING;
	thread->state |= SHT_THREAD_STATE_SUSPEND;
	/*设置线程所在的核可调度*/
	sht_set_need_sched(true);
}

void sht_sched()
{
	/*如果不需要调度，则返回*/
	if (!sht_need_sched)
		return;

	if (sht_intr_nesting)
		return;

	if (sched_lock)
		return;
	/*如果还没有开始调度，则返回*/
	if (!sht_start_sched)
		return;

	/*这个函数进行简单处理后会直接或间接调用sht_real_sched,或者sht_real_intr_sched*/
	sht_real_sched();
	
	return;
}
void sht_real_sched()
{
	sht_thread_t *prev;
	sht_thread_t *next;
	sht_set_need_sched(false);
	prev = sht_cur_thread;
	/*选择最高优先级线程*/
	sht_select_thread();
	next = sht_ready_thread;
	if (prev != next)
	{
		sht_set_running_thread(next);
		if (prev->state == SHT_THREAD_STATE_EXIT)
		{
			prev->state = SHT_THREAD_STATE_RELEASE;
			hw_context_switch_to(&next->stack);
			return;
		}

		/*线程切换*/
		hw_context_switch(&prev->stack, &next->stack);
	}
}

void sht_real_intr_sched()
{
	sht_thread_t *prev;
	sht_thread_t *next;
	sht_set_need_sched(false);
	prev = sht_cur_thread;
	/*选择最高优先级线程*/
	sht_select_thread();
	next = sht_ready_thread;
	if (prev != next)
	{
		sht_set_running_thread(next);
		if (prev->state == SHT_THREAD_STATE_EXIT)
		{
			prev->state = SHT_THREAD_STATE_RELEASE;
			
			hw_context_switch_to(&next->stack);

			return;
		}

		/*
			系统分析(线程调度)
		*/
		#ifdef ANALYZE2
				sht_print("*****************analyze begin******************\r\n");
				sht_print("%s -> %s\r\n",prev->name,next->name);
				sht_print("prev thread state:%d\r\n",prev->state);
				sht_print("next thread state:%d\r\n",next->state);
				sht_print("sched start Systick:%d\r\n", SysTick->VAL);
		#endif
		
		/*线程切换*/
		hw_context_switch_interrupt(&prev->stack, &next->stack);

		#ifdef ANALYZE2
				sht_print("sched end Systick:%d\r\n",SysTick->VAL);
				sht_print("*****************analyze end********************\r\n\r\n");
		#endif
	}
}

void sht_select_thread()
{
	unsigned int index;
	sht_rdy_queue_t *rdy_queue;
	sht_list_t *head;
	sht_thread_t *thread;
	sht_list_t *queue;
	rdy_queue = &sht_ready_queues;
	/*找出就绪队列中优先级最高的线程的优先级*/
	index = sht_get_highprio(rdy_queue);
	queue = rdy_queue->queue + index;
	head = queue;
	thread = list_entry(head->next, sht_thread_t, ready);
	sht_set_ready_thread(thread);
}
