/**
 * @file lsched.h
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，SHT调度机制头文件
 * @note 之所以要叫lsched.h，而不叫sched.h，是因为linux有一个头文件叫sched.h，怕重名会有问题
 * @version 1.1
 * @date 2023-04-20
 * @copyright Copyright (c) 2023
 * @revisionHistory 
 *  <table> 
 *   <tr><th> 版本 <th>作者 <th>日期 <th>修改内容 
 *   <tr><td> 0.1 <td>jivin <td>2010-03-08 <td>Created 
 *   <tr><td> 1.0 <td>王彬浩 <td> 2022-07-08 <td>Standardized 
 *   <tr><td> 1.1 <td>王彬浩 <td> 2023-04-20 <td>optimized 
 *  </table>
 */
#ifndef SCHED_H
#define SCHED_H

#include "thread.h"

extern unsigned char sht_need_sched; 
extern unsigned char sched_lock;
extern sht_thread_t *sht_cur_thread,*sht_ready_thread;

///就绪队列中的优先级位图的大小，目前等于2，算法就是优先级数目除以32向上取整
#define PRIO_BITMAP_SIZE ((SHT_MAX_PRIO_NUM+31)/32) 
#define sht_set_need_sched(val) (sht_need_sched=(val))
#define sht_set_ready_thread(thread) (sht_ready_thread=(thread))

/**
 * @brief SHT就绪队列
 * 
 */
typedef struct{
	unsigned int num;							///<就绪的线程数
	unsigned int bitmap[PRIO_BITMAP_SIZE];		///<优先级位图，每一位对应一个优先级，为1表示这个优先级有就绪线程
	sht_list_t queue[SHT_MAX_PRIO_NUM];	///<每一个优先级都有独立的队列
}sht_rdy_queue_t;

void sht_sched_init(void);
void sht_set_running_thread(sht_thread_t *thread);
void sht_thread_runqueue_init(void);

/**
 * @brief 将某个线程挂载到就绪队列上
 * 
 * @param new 将被挂载的线程
 */
void sht_rdyqueue_add(sht_thread_t *new);

/**
 * @brief 从就绪队列上删除一个线程
 * 
 * @param old 要删除的线程
 */
void sht_rdyqueue_del(sht_thread_t *old);


/**
 * @brief 从就绪队列中选出优先级最高的线程，设为sht_ready_thread
 * 
 */
void sht_select_thread(void);
void sht_sched(void);
void sht_real_sched();
void sht_real_intr_sched();
#endif
