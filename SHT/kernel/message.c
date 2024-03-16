/**
 * @file message.c
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，消息机制
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
#include "message.h"
#include <stdio.h>

sht_pool_ctrl_t sht_msgctr_pool_ctrl;
sht_pool_ctrl_t sht_msg_pool_ctrl;
sht_list_t g_msgctr_header; ///< 全局-用来串系统中所有的sht_msgctr_t块，在中断函数中处理ttl和timeout，create sht_msgctr_t 时加到该链表中

void sht_msg_sys_init()
{
	/*初始化全局事件列表头*/
	sht_init_list(&(g_msgctr_header));
	sht_msgctr_pool_ctrl.type = SHT_RES_MST;
	sht_msgctr_pool_ctrl.size = sizeof(sht_msgctr_t);
	sht_msgctr_pool_ctrl.num_per_pool = 10;
	sht_msgctr_pool_ctrl.max_pools = 4;

	sht_msg_pool_ctrl.type = SHT_RES_MSG;
	sht_msg_pool_ctrl.size = sizeof(sht_msg_t);
	sht_msg_pool_ctrl.num_per_pool = 10;
	sht_msg_pool_ctrl.max_pools = 4;

	sht_pool_ctrl_init(&sht_msgctr_pool_ctrl);
	sht_pool_ctrl_init(&sht_msg_pool_ctrl);
}

sht_msgctr_t *sht_alloc_msgctr()
{
	return (sht_msgctr_t *)sht_get_res(&sht_msgctr_pool_ctrl);
}

sht_msg_t *sht_alloc_msg()
{
	return (sht_msg_t *)sht_get_res(&sht_msg_pool_ctrl);
}

void sht_msgctr_queue_add(sht_msgctr_t *msgctr,
							 sht_thread_t *thread)
{ /*需按优先级排序*/
	/*sht_list_add2_tail (&thread->waiting, &msgctr->waiting);*/
	sht_list_t *p, *q;
	sht_thread_t *ptd;

	p = &msgctr->waiting;
	q = p->next;
	for (; p != q; q = q->next)
	{
		ptd = list_entry(q, sht_thread_t, waiting);
		if (ptd->prio > thread->prio)
			break;
	}
	sht_list_add(&thread->waiting, q->prev);
}

sht_msgctr_t *sht_msgctr_create()
{
	sht_msgctr_t *msgctr;

	msgctr = sht_alloc_msgctr();

	if (msgctr == NULL)
		return NULL;

	msgctr->name = NULL;
	msgctr->count = 0;
	msgctr->wait_thread_num = 0;

	sht_init_list(&msgctr->msgctr_list);
	sht_init_list(&msgctr->msglist);
	sht_init_list(&msgctr->waiting);

	sht_list_add2_tail(&msgctr->msgctr_list, &(g_msgctr_header));
	return msgctr;
}

sht_msg_t *sht_msg_create(
	unsigned int count, unsigned int id,
	unsigned int nTtl /* = 0*/, void *dat /*= NULL*/)
{
	sht_msg_t *msg;

	msg = sht_alloc_msg();

	if (msg == NULL)
		return NULL;

	msg->id = id;	 /*消息标识*/
	msg->count = count;		 /*消息被接收次数*/
	msg->ttl = nTtl; /*消息生存周期*/
	msg->data = dat; /*消息指针*/
	sht_init_list(&msg->msglist);
	return msg;
}

unsigned int sht_msg_send(sht_msgctr_t *msgctr, sht_msg_t *msg)
{
	/*	if (sht_intr_nesting > 0)
			return MST_ERR_INTR;
	*/
	long level = sht_enter_critical();

	if (NULL == msgctr)
	{
		sht_exit_critical(level);
		return MST_ERR_NULL;
	}

	if (NULL == msg)
	{
		sht_exit_critical(level);
		return MSG_ERR_NULL;
	}

	/*----------------*/
	/*   消息数限制*/
	/*----------------*/
	if (SHT_MESSAGE_MAX_COUNT <= msgctr->count)
	{
		sht_exit_critical(level);
		return MSG_ERR_COUNT;
	}

	/*----------------*/
	/*   增加消息*/
	/*----------------*/
	msgctr->count++;
	msg->ttl += sht_get_ticks();
	sht_list_add2_tail(&msg->msglist, &msgctr->msglist);

	/*----------------*/
	/*   唤醒等待*/
	/*----------------*/
	if (msgctr->wait_thread_num > 0)
	{
		/* 此处将最高优先级唤醒*/
		wake_up_thread(&msgctr->waiting);
		msgctr->wait_thread_num--;
	}
	sht_exit_critical(level);
	sht_sched();
	return MSGCTR_SUCCED;
}

void *sht_msg_recv(sht_msgctr_t *msgctr,
					  unsigned int id,
					  unsigned int timeout,
					  unsigned int *err)
{
	void *dat;
	sht_list_t *p, *q;
	sht_msg_t *pmsg;
	sht_thread_t *cur;

	if (sht_intr_nesting > 0)
	{
		*err = MST_ERR_INTR;
		return NULL;
	}
	if (NULL == msgctr)
	{
		*err = MST_ERR_NULL;
		return NULL;
	}

	cur = sht_cur_thread;

	long level = sht_enter_critical();
	if (timeout > 0)
	{
		cur->delay = TIME_TO_TICKS(timeout);
		timeout_queue_add(cur);
	}
	while (1)
	{
		p = &msgctr->msglist;
		q = p->next;
		for (; p != q; q = q->next)
		{
			pmsg = list_entry(q, sht_msg_t, msglist);
			if ((pmsg->id == id) && (pmsg->count > 0))
			{
				/*-----------------*/
				/* 有接收消息*/
				/*-----------------*/
				pmsg->count--;
				/*-----------------*/
				/* 延时列表删除*/
				/*-----------------*/
				timeout_queue_del(cur);
				dat = pmsg->data;
				sht_list_del(q);
				sht_release_res((sht_res_t *)pmsg);
				msgctr->count--;
				sht_exit_critical(level);
				return dat;
			}
		}

		/*-----------------*/
		/*  没有接收消息*/
		/*-----------------*/
		msgctr->wait_thread_num++;
		sht_msgctr_queue_add(msgctr, cur);
		sht_unrdy_thread(cur);
		sht_exit_critical(level);
		sht_sched();
		/*-----------------*/
		/*  看有没有超时*/
		/*-----------------*/
		long level = sht_enter_critical();

		if (timeout > 0 && (int)cur->delay <= 0)
			break;
	}

	/*---------------*/
	/*  超时退出*/
	/*---------------*/
	//	timeout_queue_del(cur);
	if (msgctr->wait_thread_num > 0)
		msgctr->wait_thread_num--;
	sht_list_del(&cur->waiting);
	sht_exit_critical(level);
	*err = MST_ERR_TIMEOUT;
	return NULL;
}

unsigned int sht_msgctr_del(sht_msgctr_t *pmsgctr, unsigned int flag)
{
	sht_list_t *p, *q;
	sht_thread_t *thread;
	sht_msg_t *pmsg;

	if (NULL == pmsgctr)
		return MST_ERR_NULL;
	if (flag == MST_DEL_UNFORCE)
	{
		if ((pmsgctr->count > 0) || (pmsgctr->wait_thread_num > 0))
			return MST_ERR_UNDEF;
		else
			sht_release_res((sht_res_t *)pmsgctr);
	}
	else
	{
		// 释放等待进程
		if (pmsgctr->wait_thread_num > 0)
		{
			p = &pmsgctr->waiting;
			q = p->next;
			for (; q != p; q = q->next)
			{
				thread = list_entry(q, sht_thread_t, waiting);
				// sht_list_del  (&thread->waiting);
				sht_rdy_thread(thread);
			}
		}

		// 释放消息结构
		if (pmsgctr->count > 0)
		{
			p = &pmsgctr->msglist;
			q = p->next;
			for (; p != q; q = p->next)
			{
				pmsg = list_entry(q, sht_msg_t, msglist);
				sht_list_del(q);
				sht_release_res((sht_res_t *)pmsg);
			}
		}

		// 释放资源
		sht_release_res((sht_res_t *)pmsgctr);
	}
	return MSGCTR_SUCCED;
}

unsigned int sht_msg_del(sht_msg_t *pmsg)
{
	if (NULL != pmsg)
		sht_release_res((sht_res_t *)pmsg);
	return 0;
}

void wake_up_thread(sht_list_t *head)
{
	sht_list_t *p, *q;
	sht_thread_t *thread;

	p = head;
	q = p->next;
	thread = list_entry(q, sht_thread_t, waiting);
	sht_list_del(&thread->waiting);
	sht_rdy_thread(thread);
}

void sht_print_all_msg(sht_msgctr_t *msgctr)
{
	sht_list_t *p, *q;
	sht_msg_t *pmsg;

	p = &msgctr->msglist;
	q = p->next;
	for (; p != q; q = q->next)
	{
		pmsg = list_entry(q, sht_msg_t, msglist);
		sht_print("\nid = %d", pmsg->id);
	}
}