/**
 * @file thread.h
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，线程优先级、控制块定义，线程管理函数声明
 * @version 1.1
 * @date 2023-04-19
 * @copyright Copyright (c) 2023
 * @revisionHistory 
 *  <table> 
 *   <tr><th> 版本 <th>作者 <th>日期 <th>修改内容 
 *   <tr><td> 0.1 <td>pegasus <td>2010-07-19 <td>增加timeout链表，用来处理超时，挂g_timeout_queue
 *   <tr><td> 1.0 <td>王彬浩 <td> 2022-06-24 <td>Standardized 
 * 	 <tr><td> 1.1 <td>王彬浩 <td> 2023-04-19 <td>use enum 
 *  </table>
 */

#ifndef SHT_THREAD_H
#define SHT_THREAD_H
#include "autocfg.h"
#include "list.h"
#include "mem.h"
#include "event.h"

#define SHT_MAX_PRIO_NUM ((CFG_MAX_THREAD + 1) & 0xff) ///<41。总共有40个线程，就有0~40共41个优先级
#define SHT_MINI_PRIO CFG_MAX_THREAD ///<SHT最低优先级40

extern sht_list_t sht_threads_queue;

typedef enum{
	SHT_INIT_PRIO,	///<init线程独有的0优先级
	SHT_MAX_PRIO,	///<SHT系统中允许的最高优先级
	SHT_HARD_RT_PRIO_MAX,	///<硬实时任务最高优先级
	SHT_HARD_RT_PRIO_MIN = SHT_HARD_RT_PRIO_MAX+CFG_HARD_RT_PRIO_NUM,	///<硬实时任务最低优先级
	SHT_NONHARD_RT_PRIO_MAX,	///<非硬实时任务最高优先级

	SHT_DAEMON_PRIO = SHT_MINI_PRIO-2,	///<daemon回收线程专用优先级
	SHT_NONHARD_RT_PRIO_MIN,	///<非硬实时任务最低优先级
	SHT_IDLE_PRIO	///<idle线程专用优先级，也是系统最低优先级SHT_MINI_PRIO
}shtPrioEnum;

typedef enum{
	SHT_NONHARD_PRIO, ///<非硬实时任务的优先级，会将tcb中的prio加上SHT_NONHARD_RT_PRIO_MAX
	SHT_HARD_PRIO	 ///<硬实时任务优先级，会将tcb中的prio加上SHT_HARD_RT_PRIO_MAX
}shtPrioTypeEnum;

typedef enum{
	SHT_THREAD_STATE_READY = 1,
	SHT_THREAD_STATE_SUSPEND = 1<<1,
	SHT_THREAD_STATE_RUNNING = 1<<2,
	SHT_THREAD_STATE_EXIT = 1<<3,
	SHT_THREAD_STATE_RELEASE = 1<<4,
	SHT_THREAD_STATE_DELAY = 1<<5,
	SHT_THREAD_STATE_MOVE = 1<<6
}shtThreadStateEnum;

typedef enum{
    SHT_ERR_THREAD,
    SHT_ERR_THREAD_DELAY,
    SHT_ERR_THREAD_NO_STACK  ///<线程栈指针为空
}shtThreadErrorEnum;

/**
 * 
 *  @struct sht_thread_t
 *  @brief 线程控制块TCB
 * 
 * 
 */
typedef struct{//SPG加注释
  	sht_res_t res;	///<资源id，线程创建后作为线程id
	unsigned char state;
	unsigned char prio;
	unsigned char policy;
	sht_list_t ready;	///<用于挂载到全局就绪队列
	sht_list_t timeout;	///<用于挂载到超时阻塞队列
	sht_list_t waiting;	///<用于挂载到延时队列
	sht_list_t global_list;	///<用于挂载到全局线程列表
	sht_evt_t* evt;		///<用于指向线程占用的事件（信号量、互斥量等），方便线程退出时释放事件
	unsigned int *stack;	///<指示线程的堆栈，线程被抢占切换时会保存cpu堆栈寄存器sp的值
	unsigned int *stack_buttom;	///<栈底
	unsigned int stack_size;	///<堆栈大小
	int delay;				///<指定线程延时时间，单位是Ticks，调用sht_delay_self时会把时间参数转换成Ticks后赋值给delay，剩余等待时间
	char *name;				///<线程名字
	int console_id;			///<线程控制台ID号
	void*	private_data;	///<长久备用数据指针
	void*	data;			///<临时备用数据指针
}sht_thread_t;

void sht_release_thread(sht_res_t *thread);
void sht_suspend_thread(sht_thread_t *thread);
void sht_resume_thread(sht_thread_t *thread);
void sht_kill_thread(sht_thread_t *thread);
unsigned int sht_thread_init(sht_thread_t *thread,void (*route)(void *args),void (*exit)(void),void *args);
sht_thread_t *sht_alloc_thread(void);
void sht_thread_pool_init(void);
void sht_thread_sys_init(void);
void sht_unrdy_thread(sht_thread_t *thread);
void sht_rdy_thread(sht_thread_t *thread);
void sht_thread_move2_tail_by_id(int thread_id);
void sht_thread_move2_tail(sht_thread_t *thread);
void sht_thread_change_prio(sht_thread_t* thread, unsigned int prio);

/***************线程控制API****************/

/**
 * @brief 创建一个线程
 * 
 * @param route 线程函数
 * @param stack_size 线程栈大小
 * @param args 线程函数参数
 * @param name 线程名字
 * @param stack 线程栈指针
 * @param sched_policy 线程调度策略
 * @param data 线程策略数据
 * @return int 返回线程id
 */
int sht_create_thread(void (*route)(void *args),unsigned int stack_size,void *args,char *name,void *stack,unsigned int sched_policy,void *data);



/**
 * @brief 挂起当前线程
 * 
 */
void sht_suspend_self(void);

/**
 * @brief 挂起某个线程
 * 
 * @param thread_id 要挂起的线程id
 */
void sht_suspend_thread_by_id(unsigned int thread_id);

/**
 * @brief 唤醒某个线程
 * 
 * @param thread_id 要唤醒的线程id
 */
void sht_resume_thread_by_id(unsigned int thread_id);

/**
 * @brief 将当前线程延时
 * 
 * @param time 延时时间（毫秒）
 */
void sht_delay_self(unsigned int time);

/**
 * @brief 干掉某个线程
 * 
 * @param id 要干掉的线程id
 */
void sht_kill_thread_by_id(int id);

/**
 * @brief 结束当前线程
 * 
 */
void sht_thread_exit(void);

/**
 * @brief 改变当前线程优先级
 * 
 * @param prio 目标优先级
 */
void sht_change_prio_self(unsigned int prio);

/**
 * @brief 改变某个线程优先级
 * 
 * @param thread_id 线程id
 * @param prio 目标优先级
 */
void sht_thread_change_prio_by_id(unsigned int thread_id, unsigned int prio);

#endif

