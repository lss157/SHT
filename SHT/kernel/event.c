/**
 * @file event.h
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，事件资源相关
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
#include "mem.h"
#include "message.h"
#include "thread.h"
#include "list.h"
#include <stdio.h>

sht_pool_ctrl_t sht_evt_pool_ctrl;
void sht_evt_sys_init()
{
	sht_evt_pool_init();
}

void sht_evt_pool_init()
{
	sht_evt_pool_ctrl.type = SHT_RES_EVENT;
	sht_evt_pool_ctrl.size = sizeof(sht_evt_t);
	sht_evt_pool_ctrl.num_per_pool = 8;
	sht_evt_pool_ctrl.num = 0;
	sht_evt_pool_ctrl.max_pools = 4;
	sht_pool_ctrl_init(&sht_evt_pool_ctrl);
#ifdef CFG_MSG
	sht_msg_sys_init();
#endif
}

sht_evt_t *sht_alloc_evt()
{
	return (sht_evt_t *)sht_get_res(&sht_evt_pool_ctrl);
}

void sht_evt_init(sht_evt_t *evt)
{
	sht_init_list(&evt->wait_queue);
}

_Bool sht_evt_queue_empty(sht_evt_t *evt)
{
	return sht_list_empty(&evt->wait_queue);
}

sht_thread_t *sht_evt_high_thread(sht_evt_t *evt)
{
	sht_list_t *head;
	sht_thread_t *thread;
	head = &evt->wait_queue;
	if (sht_list_empty(head))
		return NULL;
	thread = list_entry(head->next, sht_thread_t, waiting);
	return thread;
}

void sht_evt_queue_add(sht_evt_t *evt, sht_thread_t *new)
{
	sht_list_t *head, *tmp;
	sht_thread_t *thread;
	new->evt = evt;
	head = &evt->wait_queue;
	for (tmp = head->next; tmp != head; tmp = tmp->next)
	{
		thread = list_entry(tmp, sht_thread_t, waiting);
		/*如果线程资源已经不在使用，即release状态则释放*/
		if (thread->prio > new->prio)
			break;

		if (tmp == tmp->next)
			break;
	}
	sht_list_add(&new->waiting, tmp->prev);
}

void sht_evt_queue_del(sht_thread_t *thread)
{
	sht_list_del(&thread->waiting);
	thread->evt = NULL;
}
