/**
 * @file sem.h
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，内核信号量相关函数
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

#include "event.h"
#include "thread.h"
#include "lsched.h"
#include "hal.h"
#include "int.h"
#include "timer.h"
#include "sem.h"
#include <stdio.h>

extern void sht_evt_queue_del(sht_thread_t *thread);
extern void sht_evt_queue_add(sht_evt_t *evt, sht_thread_t *new);
sht_thread_t *sht_evt_high_thread(sht_evt_t *evt);
shtSemRetValEnum sht_sem_init(sht_evt_t *evt, unsigned int semNum)
{
	if (NULL == evt)
	{
		return SEM_ERR_NULL;
	}
	semNum = 1 - semNum; /* 拥有多个资源，0,一个  -1 两个， -2 三个 ....*/
	evt->count = semNum;
	evt->type = SHT_EVENT_SEM;
	evt->data = NULL;
	sht_evt_init(evt);
	return SEM_SUCCED;
}

sht_evt_t *sht_sem_create(unsigned int semNum)
{
	sht_evt_t *evt;
	evt = sht_alloc_evt();
	if (NULL == evt)
	{
		return NULL;
	}
	semNum = 1 - semNum; /* 拥有多个资源，0,一个  -1 两个， -2 三个 ....*/
	evt->count = semNum;
	evt->type = SHT_EVENT_SEM;
	evt->data = NULL;
	sht_evt_init(evt);
	return evt;
}

shtSemRetValEnum sht_sem_del(sht_evt_t *evt)
{
	sht_thread_t *thread;
	if (sht_intr_nesting)
	{
		return SEM_ERR_INTR;
	}
	/* 参数检测*/
	if (NULL == evt)
		return SEM_ERR_NULL; /* error*/
	if (evt->type != SHT_EVENT_SEM)
		return SEM_ERR_TYPE; /* error*/

	long level = sht_enter_critical();
	thread = sht_evt_high_thread(evt);
	if (thread == NULL)
	{
		/*队列上无等待任务*/
		sht_exit_critical(level);
		evt = NULL;
		return SEM_ERR_UNDEF;
	}
	else
	{
		/*有等待任务*/
		sht_exit_critical(level);
		return SEM_ERR_TASK_EXIST; /*error*/
	}
}

shtSemRetValEnum sht_sem_trypend(sht_evt_t *evt)
{
	if (sht_intr_nesting)
	{
		return SEM_ERR_INTR;
	}

	/* 参数检测 */
	if (NULL == evt)
	{
		return SEM_ERR_NULL; /*error*/
	}
	if (SHT_EVENT_SEM != evt->type)
	{
		return SEM_ERR_TYPE; /*error*/
	}

	/* 计算信号量处理*/
	long level = sht_enter_critical();
	if ((char)evt->count <= SEM_RES_AVAI)
	{ /* available*/
		evt->count++;
		sht_exit_critical(level);
		return SEM_SUCCED;
	}
	sht_exit_critical(level);
	return SEM_ERR_TIMEOUT;
}

shtSemRetValEnum sht_sem_pend(sht_evt_t *evt, unsigned int timeout)
{
	sht_thread_t *cur = sht_cur_thread;

	if (sht_intr_nesting)
	{
		return SEM_ERR_INTR;
	}

	/* 参数检测 */
	if (NULL == evt)
	{
		return SEM_ERR_NULL; /*error*/
	}
	if (SHT_EVENT_SEM != evt->type)
	{
		return SEM_ERR_TYPE; /*error*/
	}

	/* 计算信号量处理*/
	long level = sht_enter_critical();
	if ((char)evt->count <= SEM_RES_AVAI)
	{ /* available*/
		evt->count++;
		sht_exit_critical(level);
		return SEM_SUCCED;
	}

	evt->count++;
	sht_unrdy_thread(cur);
	if (timeout > 0)
	{
		cur->delay = TIME_TO_TICKS(timeout);
		timeout_queue_add(cur);
	}
	sht_evt_queue_add(evt, cur);
	sht_exit_critical(level);

	sht_sched();

	level = sht_enter_critical();
	if (timeout > 0 && cur->delay <= 0)
	{
		//--------------
		// modify by pegasus 0804: count-- [+]
		evt->count--;
		sht_evt_queue_del(cur);
		sht_exit_critical(level);
		return SEM_ERR_TIMEOUT;
	}

	//-------------------
	// modify by pegasus 0804: timeout_queue_del [+]
	timeout_queue_del(cur);
	sht_exit_critical(level);
	return SEM_SUCCED;
}

shtSemRetValEnum sht_sem_post(sht_evt_t *evt)
{
	sht_thread_t *thread;

	/* 参数检测*/
	if (NULL == evt)
	{
		return SEM_ERR_NULL; /* error*/
	}
	if (SHT_EVENT_SEM != evt->type)
	{
		return SEM_ERR_TYPE;
	}

	long level = sht_enter_critical();

	/* 计算信号量的释放*/
	if ((char)evt->count <= SEM_RES_NOAVAI)
	{ /* no waiting thread*/
		evt->count--;
		sht_exit_critical(level);
		return SEM_SUCCED;
	}
	/* 有等待线程*/
	evt->count--;
	thread = sht_evt_high_thread(evt);
	if (thread == NULL)
	{
		/*应该有等待线程却没有找到*/
		sht_print("Err Sem post\n");
		sht_exit_critical(level);
		return SEM_ERR_UNDEF;
	}
	timeout_queue_del(thread);
	/*释放等待任务*/
	sht_evt_queue_del(thread);
	sht_rdy_thread(thread);
	sht_exit_critical(level);
	sht_sched();
	return SEM_SUCCED;
}

int sht_sem_getnum(sht_evt_t *evt)
{
	int t;
	if (NULL == evt)
		return SEM_ERR_NULL;

	long level = sht_enter_critical();
	t = 1 - (int)evt->count;
	sht_exit_critical(level);
	return t;
}
