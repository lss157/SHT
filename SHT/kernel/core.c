/**
 * @mainpage SHT-I源码文档
 * @section 简介
 * 珊瑚SHT单核版本源码文档，对源码中的文件、函数、变量等进行了详细说明，适合初学者阅读，用以加深对SHT-I的理解。\n
 * 配合《珊瑚-I 内核手册》食用更香哦。
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @version 1.0
 * @date 2022-06-24
 * @copyright Copyright (c) 2023
 * @revisionHistory
 *  <table>
 *   <tr><th> 版本 <th>作者 <th>日期 <th>修改内容
 *   <tr><td> 0.1 <td>jivin <td>2010-03-08 <td>Created
 *   <tr><td> 1.0 <td>王彬浩 <td> 2022-06-24 <td>Standardized
 *  </table>
 */

/**
 * @file core.c
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，SHT内核初始化文件，紧接start.S
 * @version 1.0
 * @date 2022-07-04
 * @copyright Copyright (c) 2023
 * @revisionHistory
 *  <table>
 *   <tr><th> 版本 <th>作者 <th>日期 <th>修改内容
 *   <tr><td> 0.1 <td>jivin <td>2010-03-08 <td>Created
 *   <tr><td> 1.0 <td>王彬浩 <td> 2022-07-04 <td>Standardized
 *  </table>
 */

#include "kernel.h"
#include "hal.h"
#include "lib.h"
#include <stdio.h>
#include <stdbool.h>
#include "sys_app.h"

sht_list_t sht_res_release_queue; ///< 将被daem线程回收的线程队列
volatile unsigned int sht_start_sched = false;
int daemon_id, idle_id, init_id;

void idle(void *args)
{
	//__ASM volatile ("MSR primask, %0" : : "r" (0) : "memory");
	
	for (;;)
	{
	}
}

void daem(void *args)
{
	sht_thread_t *thread;
	sht_list_t *head, *tmp, *tmp1;
	head = &sht_res_release_queue;
	while (1)
	{
		for (tmp = head->next; tmp != head;)
		{
			tmp1 = tmp->next;
			long level = sht_enter_critical();
			thread = list_entry(tmp, sht_thread_t, waiting);
			/*如果线程资源已经不在使用，即release状态则释放*/
			sht_list_del(tmp);
			sht_exit_critical(level);
			tmp = tmp1;
			if (thread->state == SHT_THREAD_STATE_RELEASE)
			{

				sht_release_thread((sht_res_t *)thread);
			}
			else
			{
				long level = sht_enter_critical();
				tmp1 = head->prev;
				sht_list_add2_tail(&thread->waiting, head); /**/
				sht_exit_critical(level);
			}
		}
		sht_suspend_self();
	}
}

void init(void *args)
{
    APP_LOG(TS_OFF, VLEVEL_M, "in init\r\n");
	sht_comm_policy_data_t *data;
    data = sht_malloc(sizeof(sht_comm_policy_data_t));
	sht_ticks_init();
	/*ticks中断初始化函数*/
	sht_start_sched = true;

	/*创建后台服务进程*/
	sht_init_list(&sht_res_release_queue);
	data->prio = SHT_DAEMON_PRIO;
	data->prio_type = SHT_HARD_PRIO;
	daemon_id = sht_create_thread(daem, DAEM_STACK_SIZE, NULL, "daemon", NULL, SHT_SCHED_POLICY_COMM, data);
	if (daemon_id == -1)
		while (1);
			/*应用级相关服务初始化,应用级不要使用延时函数，没有效果的*/
#ifdef CFG_SHELL
	//sht_shell_init();
#endif
	user_main();//SPG先不要？
    APP_LOG(TS_OFF, VLEVEL_M, "init done\r\n");
}

void sht_start()
{
    APP_LOG(TS_OFF, VLEVEL_M, "before module init\r\n");
	/*内核模块初始化*/
	sht_module_init();
    APP_LOG(TS_OFF, VLEVEL_M, "after module init\r\n");
	/*主cpu开始函数*/
	sht_core_cpu_start();
}

void sht_core_cpu_start()
{
	sht_comm_policy_data_t* data1;
	data1 = sht_malloc(sizeof(sht_comm_policy_data_t));
	/*创建空闲线程*/
	sht_start_sched = false;
	data1->prio = SHT_IDLE_PRIO;
	data1->prio_type = SHT_HARD_PRIO;
	idle_id = sht_create_thread(idle, IDLE_STACK_SIZE, NULL, "idle", NULL, SHT_SCHED_POLICY_COMM, data1);
	if (idle_id == -1)
	{
		while (1)
		{
		}
	}
	/*创建初始化线程,这个调用层次比较多，需要多谢堆栈*/
	sht_comm_policy_data_t* data2;
	data2 = sht_malloc(sizeof(sht_comm_policy_data_t));
	data2->prio = SHT_INIT_PRIO;
	data2->prio_type = SHT_HARD_PRIO;
	/*动态堆栈*/
	init_id = sht_create_thread(init, 1516, "in init", "init", NULL, SHT_SCHED_POLICY_COMM, data2);
	if (init_id == -1)
	{
		while (1)
		{
		}
	}
	sht_start_os();
}

void sht_start_os()
{
	sht_sched_init();
	sht_select_thread();
	// sht_set_running_thread(sht_ready_thread); // SPG空指针
	sht_cur_thread = sht_ready_thread;
	hw_context_switch_to(&sht_cur_thread->stack);
}

void sht_module_init()
{
	/*中断系统初始化*/
	sht_intr_sys_init();
    APP_LOG(TS_OFF, VLEVEL_M, "intr_sys_init done\r\n");

	/*内存管理系统初始化*/
	sht_mem_sys_init();
    APP_LOG(TS_OFF, VLEVEL_M, "mem_sys_init done\r\n");

	/*线程管理系统初始化*/
	sht_thread_sys_init();
    APP_LOG(TS_OFF, VLEVEL_M, "thread_sys_init done\r\n");

	/*时钟管理系统初始化*/
	sht_time_sys_init();
    APP_LOG(TS_OFF, VLEVEL_M, "time_sys_init done\r\n");

	/*事件管理系统初始化*/
	sht_evt_sys_init();
    APP_LOG(TS_OFF, VLEVEL_M, "evt_sys_init done\r\n");
#ifdef CFG_DRIVER
	/*驱动管理系统初始化*/
	// sht_drv_sys_init();
#endif
}
