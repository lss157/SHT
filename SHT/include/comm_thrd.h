/**
 * @file comm_thrd.h
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，普通线程头文件
 * @version 1.0
 * @date 2022-07-23
 * @copyright Copyright (c) 2023
 * @revisionHistory 
 *  <table> 
 *   <tr><th> 版本 <th>作者 <th>日期 <th>修改内容 
 *   <tr><td> 0.1 <td>jivin <td>2010-03-08 <td>Created 
 *   <tr><td> 1.0 <td>王彬浩 <td> 2022-07-23 <td>Standardized 
 *  </table>
 */
#ifndef COMM_THRD_H
#define COMM_THRD_H

#include "thread.h"

/**
 * @brief 普通线程调度相关的数据
 * 
 */
typedef struct{
	unsigned char prio; 		///<优先级
	unsigned char prio_type; 	///<优先级类型，有SHT_BASE_PRIO和SHT_ABSOLUTE_PRIO，后者表示优先级设定是多少就是多少，前者则需要根据系统需要在创建线程时进行调整
}sht_comm_policy_data_t;

/**
 * @brief 初始化普通线程的一些数据
 * 
 * @param thread 
 * @param route 
 * @param args 
 * @param data 
 * @return int 
 */
int comm_policy_thread_init(sht_thread_t *thread,void (*route)(void *args),void *args,void *data);

/**
 * @brief 注册普通机制
 * @note 调用时机为系统初始化阶段
 */
void comm_policy_init();
#endif
