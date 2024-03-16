/**
 * @file mutex.h
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，互斥量头文件
 * @version 1.1
 * @date 2023-04-20
 * @copyright Copyright (c) 2023
 * @revisionHistory
 *  <table>
 *   <tr><th> 版本 <th>作者 <th>日期 <th>修改内容
 *   <tr><td> 0.1 <td>jivin <td>2010-03-08 <td>Created
 *   <tr><td> 1.0 <td>王彬浩 <td> 2022-07-28 <td>Standardized
 *   <tr><td> 1.1 <td>王彬浩 <td> 2023-04-20 <td>optimized
 *  </table>
 */

#ifndef _SHT_MUTEX_H
#define _SHT_MUTEX_H

#include "event.h"

#define MUTEX_AVAI 0x00FF

#define MUTEX_L_MASK 0x00FF
#define MUTEX_U_MASK 0xFF00
#define MUTEX_CEILING_MASK 0xFF0000

typedef enum
{
    MUTEX_SUCCED,
    MUTEX_THREAD_SUSPEND,
    MUTEX_ERR_NULL,
    MUTEX_ERR_TYPE,
    MUTEX_ERR_TASK_EXIST,
    MUTEX_ERR_INTR,
    MUTEX_ERR_UNDEF,
    MUTEX_ERR_TIMEOUT,
    MUTEX_ERR_RDY
} shtMutexRetVal;

/***************互斥量相关API****************/

/**
 * @brief 初始化互斥量
 *
 * @param evt 互斥量指针
 * @param prio 优先级天花板//SPG设置限制和默认值
 * @return shtMutexRetVal
 */
shtMutexRetVal sht_mutex_init(sht_evt_t *evt, unsigned char prio);

/**
 * @brief 创建并初始化互斥量
 *
 * @param prio 优先级天花板
 * @param err 当创建失败时，err被置为 MUTEX_ERR_NULL
 * @return sht_evt_t* 返回互斥量指针
 */
sht_evt_t *sht_mutex_create(unsigned char prio, unsigned int *err);

/**
 * @brief 删除互斥量//SPG没释放内存
 *
 * @param evt 互斥量指针
 * @return shtMutexRetVal
 */
shtMutexRetVal sht_mutex_del(sht_evt_t *evt, unsigned int opt);

/**
 * @brief 获取互斥量（非阻塞式）
 *
 * @param evt 互斥量指针
 * @return shtMutexRetVal
 */
shtMutexRetVal sht_mutex_trypend(sht_evt_t *evt);

/**
 * @brief 获取互斥量（优先级继承的优先级反转解决）
 *
 * @param evt 互斥量指针
 * @param timeout 申请超时时间（0代表不设置超时时间）
 * @return shtMutexRetVal
 */
shtMutexRetVal sht_mutex_pend(sht_evt_t *evt, unsigned int timeout);

/**
 * @brief 获取互斥量（优先级天花板的优先级反转解决）
 *
 * @param evt 互斥量指针
 * @param timeout timeout 申请超时时间（0代表不设置超时时间）
 * @return shtMutexRetVal
 */
shtMutexRetVal sht_mutex_pend2(sht_evt_t *evt, unsigned int timeout);

/**
 * @brief 释放互斥量
 *
 * @param evt 互斥量指针
 * @return shtMutexRetVal
 */
shtMutexRetVal sht_mutex_post(sht_evt_t *evt);

#endif
