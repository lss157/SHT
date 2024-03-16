/**
 * @file mutex.c
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，互斥量机制
 * @version 1.0
 * @date 2023-05-08
 * @copyright Copyright (c) 2023
 * @revisionHistory
 *  <table>
 *   <tr><th> 版本 <th>作者 <th>日期 <th>修改内容
 *   <tr><td> 0.1 <td>jivin <td> 2010-03-08 <td>Created
 *   <tr><td> 1.0 <td>王彬浩 <td> 2023-05-08 <td>Standardized
 *  </table>
 */

#include "event.h"
#include "hal.h"
#include "thread.h"
#include "lsched.h"
#include "int.h"
#include "timer.h"
#include "mutex.h"
#include <stdio.h>

extern void sht_evt_queue_del(sht_thread_t *thread);
extern void sht_evt_queue_add(sht_evt_t *evt, sht_thread_t *new);

sht_thread_t *sht_evt_high_thread(sht_evt_t *evt);

shtMutexRetVal sht_mutex_init(sht_evt_t *evt, unsigned char prio)
{
	if ((sht_evt_t *)0 == evt)
	{
		return MUTEX_ERR_NULL;
	}
	evt->count = (prio << 16) | MUTEX_AVAI;
	evt->type = SHT_EVENT_MUTEX;
	evt->data = NULL;
	sht_evt_init(evt);
	return MUTEX_SUCCED;
}

sht_evt_t *sht_mutex_create(unsigned char prio, unsigned int *err) // SPG这个和上面那个保留一个
{
	sht_evt_t *evt;

	/* 是否大于最小优先线，和是否该优先级已占用*/
	evt = sht_alloc_evt(); /* alloc a RAM for the event*/
	if (NULL == evt)
	{
		*err = MUTEX_ERR_NULL;
		return NULL;
	}
	evt->count = (prio << 16) | MUTEX_AVAI | MUTEX_U_MASK;
	evt->type = SHT_EVENT_MUTEX;
	evt->data = NULL;
	sht_evt_init(evt);
	return evt;
}

shtMutexRetVal sht_mutex_del(sht_evt_t *evt, unsigned int opt)
{
	/* 参数检测 */
	if (NULL == evt)
	{
		return MUTEX_ERR_NULL; /*error*/
	}

	if (SHT_EVENT_MUTEX != evt->type)
	{
		return MUTEX_ERR_TYPE; /*error*/
	}

	/* 是否有任务等待*/
	long level = sht_enter_critical();
	if (sht_evt_queue_empty(evt))
	{
		/*无等待任务删除*/
		sht_exit_critical(level);
		return MUTEX_SUCCED;
	}
	else
	{
		/*有等待任务*/
		sht_exit_critical(level);
		return MUTEX_ERR_TASK_EXIST;
	}
}

shtMutexRetVal sht_mutex_trypend(sht_evt_t *evt)
{
	sht_thread_t *cur;

	if (sht_intr_nesting > 0)
		return MUTEX_ERR_INTR;

	cur = sht_cur_thread;

	long level = sht_enter_critical();
	if (NULL == evt)
	{
		sht_exit_critical(level);
		return MUTEX_ERR_NULL;
	}

	if ((unsigned char)(evt->count & MUTEX_L_MASK) == MUTEX_AVAI)
	{
		/* 申请成功*/
		evt->count &= MUTEX_U_MASK;
		evt->count |= cur->prio;
		evt->data = (void *)cur;
		sht_exit_critical(level);
		return MUTEX_SUCCED;
	}

	sht_exit_critical(level);
	return MUTEX_ERR_TIMEOUT;
}

shtMutexRetVal sht_mutex_pend(sht_evt_t *evt, unsigned int timeout)
{
	unsigned char highPrio;
	sht_thread_t *thread;
	sht_thread_t *cur;

	if (sht_intr_nesting > 0)
		return MUTEX_ERR_INTR;

	cur = sht_cur_thread;

	long level = sht_enter_critical();
	if (NULL == evt)
	{
		sht_exit_critical(level);
		return MUTEX_ERR_NULL;
	}

	if ((unsigned char)(evt->count & MUTEX_L_MASK) == MUTEX_AVAI)
	{
		/* 申请成功*/
		evt->count &= MUTEX_U_MASK;
		evt->count |= cur->prio;
		evt->data = (void *)cur;
		sht_exit_critical(level);
		return MUTEX_SUCCED;
	}

	/* 互斥量已被占有*/
	highPrio = (unsigned char)(evt->count >> 8);
	thread = (sht_thread_t *)evt->data;

	/*有可能优先级反转，继承最高优先级*/
	if (thread->prio > cur->prio)
	{
		if (cur->prio < highPrio)
		{
			highPrio = cur->prio;
			evt->count &= ~MUTEX_U_MASK;
			evt->count |= highPrio << 8;
		}
		sht_thread_change_prio(thread, highPrio);
	}
	/*不需要或不能提高优先级*/
	sht_unrdy_thread(cur);
	sht_evt_queue_add(evt, cur);
	if (timeout > 0)
	{
		/*加载到超时队列*/
		cur->delay = TIME_TO_TICKS(timeout);
		timeout_queue_add(cur);
	}
	sht_exit_critical(level);
	sht_sched();
	level = sht_enter_critical();
	if (evt->data != cur && timeout > 0 && cur->delay <= 0)
	{
		sht_print("Time Out Return\n");
		sht_evt_queue_del(cur);
		sht_exit_critical(level);
		return MUTEX_ERR_TIMEOUT;
	}

	//---------------
	// modify by pegasus 0804: timeout_queue_del [+]
	timeout_queue_del(cur);

	if (evt->data != cur)
	{
		sht_print("Err Ready Return\n");
		sht_evt_queue_del(cur);
		sht_exit_critical(level);
		return MUTEX_ERR_RDY;
	}

	return MUTEX_SUCCED;
}

shtMutexRetVal sht_mutex_pend2(sht_evt_t *evt, unsigned int timeout)
{
	sht_thread_t *cur;

	if (sht_intr_nesting > 0)
		return MUTEX_ERR_INTR;

	cur = sht_cur_thread;

	long level = sht_enter_critical();
	if (NULL == evt)
	{
		sht_exit_critical(level);
		return MUTEX_ERR_NULL;
	}

	if ((unsigned char)(evt->count & MUTEX_L_MASK) == MUTEX_AVAI)
	{
		/* 申请成功*/
		evt->count &= MUTEX_U_MASK;
		evt->count |= cur->prio;
		evt->data = (void *)cur;

		/*提升至天花板优先级*/
		cur->prio = (evt->count & MUTEX_CEILING_MASK) >> 16;
		sht_exit_critical(level);
		return MUTEX_SUCCED;
	}

	/* 互斥量已被占有*/
	sht_unrdy_thread(cur);
	sht_evt_queue_add(evt, cur);
	if (timeout > 0)
	{
		/*加载到超时队列*/
		cur->delay = TIME_TO_TICKS(timeout);
		timeout_queue_add(cur);
	}
	sht_exit_critical(level);

	/*触发调度*/
	sht_sched();

	level = sht_enter_critical();

	/*超时时间内未获得互斥量*/
	if (evt->data != cur && timeout > 0 && cur->delay <= 0)
	{
		sht_print("Time Out Return\n");
		sht_evt_queue_del(cur);
		sht_exit_critical(level);
		return MUTEX_ERR_TIMEOUT;
	}

	/*超时时间内获得了互斥量*/
	timeout_queue_del(cur);

	if (evt->data != cur)
	{
		sht_print("Err Ready Return\n");
		sht_evt_queue_del(cur);
		sht_exit_critical(level);
		return MUTEX_ERR_RDY;
	}

	return MUTEX_SUCCED;
}

shtMutexRetVal sht_mutex_post(sht_evt_t *evt)
{
	unsigned char ownerPrio;
	unsigned char highPrio;
	sht_thread_t *thread;
	sht_thread_t *cur;

	long level = sht_enter_critical();

	if (NULL == evt)
	{
		sht_print("mutex NULL\n");
		sht_exit_critical(level);
		return MUTEX_ERR_NULL; /*error*/
	}

	highPrio = (unsigned char)(evt->count >> 8);
	ownerPrio = (unsigned char)(evt->count & MUTEX_L_MASK);
	cur = sht_cur_thread;
	if (highPrio != 0 && cur->prio != highPrio && cur->prio != ownerPrio)
	{
		sht_print("mutex prio err\n");
		sht_exit_critical(level);
		return MUTEX_ERR_UNDEF;
	}
	cur->evt = NULL;
	if (cur->prio != ownerPrio)
	{
		/* 提升过优先级，进行优先级复原*/
		sht_change_prio_self(ownerPrio);
	}

	thread = sht_evt_high_thread(evt);
	if (thread == NULL)
	{
		evt->count |= MUTEX_AVAI;
		evt->data = NULL;
		sht_exit_critical(level);
		return MUTEX_SUCCED;
	}
	timeout_queue_del(thread);
	sht_evt_queue_del(thread);
	evt->count &= MUTEX_U_MASK;
	evt->count |= thread->prio;
	evt->data = thread;
	sht_rdy_thread(thread);
	sht_exit_critical(level);
	sht_sched();
	return MUTEX_SUCCED;
}
