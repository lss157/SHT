/**
 * @file timer.h
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，定时器相关头文件
 * @version 1.0
 * @date 2022-07-20
 * @copyright Copyright (c) 2023
 * @revisionHistory 
 *  <table> 
 *   <tr><th> 版本 <th>作者 <th>日期 <th>修改内容 
 *   <tr><td> 0.1 <td>jivin <td>2010-03-08 <td>Created 
 *   <tr><td> 1.0 <td>王彬浩 <td> 2022-07-20 <td>Standardized 
 *  </table>
 */

#ifndef SHT_TIMER_H
#define SHT_TIMER_H

#include "autocfg.h"
#include "core.h"
#include "thread.h"
#include "stm32wlxx_hal.h"


#define TIME_TO_TICKS(time) (time)//>sht中sht_ticks_init()设置了systick每1ms产生一次中断，所以这里直接返回time（周期线程周期以毫秒单位）
extern sht_list_t time_delay_queue;
extern sht_list_t timeout_queue;

void sht_time_sys_init();
void sht_time_init(void);
void sht_ticks_init(void);
void sht_ticks_entry(int vector);
void time_delay_deal(void);

/**
 * @brief 将线程挂到延时队列上
 * 
 */
void sht_delayqueue_add(sht_list_t*, sht_thread_t*);

/**
 * @brief 超时链表处理函数
 * 
 */
void timeout_delay_deal(void);

/**
 * @brief 将线程挂到超时队列上
 * 
 */
void timeout_queue_add(sht_thread_t*);

/**
 * @brief 将线程从超时队列删除
 * 
 */
void timeout_queue_del(sht_thread_t*);

/***************ticks相关API****************/

/**
 * @brief 设置SHT心跳tick的值
 * 
 * @param time tick新值
 */
void sht_set_ticks(unsigned int time);

/**
 * @brief 得到tick的值
 * 
 * @return tick的值
 */
unsigned int sht_get_ticks(void);

#endif

