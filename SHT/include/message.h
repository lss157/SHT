/**
 * @file message.h
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，消息队列头文件
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

#ifndef SHT_MESSAGE_H
#define SHT_MESSAGE_H

#include "list.h"
#include "mem.h"
#include "event.h"
#include "thread.h"

#define SHT_MESSAGE_MAX_COUNT 10

typedef enum{
	MST_DEL_UNFORCE,
	MST_DEL_FORCE
}shtMsgctrDeleteFlag;


typedef enum{
    MST_ERR_NULL,
    MST_ERR_INTR,
    MST_ERR_TIMEOUT,
    MST_ERR_UNDEF,
    MSG_ERR_COUNT,
    MSGCTR_SUCCED,
    MSG_ERR_NULL
}shtMessgaeErrorEnum;

/**
 * @brief 消息容器结构体
 *
 */
typedef struct
{
	sht_res_t res; 			///<消息容器也是资源
	char *name;					///<消息容器名字
	sht_list_t msgctr_list; 	///<全局消息列表
	unsigned int count; 		///<消息数量
	unsigned int wait_thread_num; ///<等待线程数
	sht_list_t waiting; 		///<等待线程指针链
	sht_list_t msglist; 		///<消息链指针，用于挂载消息
}sht_msgctr_t;

/**
 * @brief 消息结构体
 *
 */
typedef struct
{
	sht_res_t res; 		///<消息也是一种资源
	sht_list_t msglist; 	///<消息链指针，用于挂载到消息容器
	unsigned int id; 		///<消息标识	
	unsigned int count; 		///<消息被接收次数，每被接收一次减一,直到0为止	
	unsigned int ttl; 		///<消息最大生命周期  ticks计数
	void *data; 			///<消息内容指针
} sht_msg_t;

void sht_msg_sys_init(void);
sht_msgctr_t *sht_alloc_msgctr(void);
sht_msg_t *sht_alloc_msg(void);

/**
 * @brief 唤醒最高优先等待线程
 * 
 * @param head 消息容器waiting成员
 */
void wake_up_thread(sht_list_t *head);

/***************消息相关API****************/

/**
 * @brief 创建消息容器
 * 
 * @return sht_msgctr_t* 消息容器指针
 */
sht_msgctr_t *sht_msgctr_create();

/**
 * @brief 创建消息
 * 
 * @param count 消息被接收次数，每被接收一次减一,直到0为止	
 * @param id 消息id
 * @param nTtl 消息最大生命周期  ticks计数
 * @param dat 消息内容指针
 * @return sht_msg_t* 消息指针
 */
sht_msg_t *sht_msg_create(unsigned int count, unsigned int id, unsigned int nTtl, void *dat);

/**
 * @brief 发送消息
 * 
 * @param msgctr 目标消息容器指针
 * @param msg 待发送消息指针
 * @return unsigned int 
 */
unsigned int sht_msg_send(sht_msgctr_t *msgctr, sht_msg_t *msg);

/**
 * @brief 接收消息
 * 
 * @param msgctr 源消息容器
 * @param id 消息id
 * @param timeout 超时时间，如果目前消息容器中没有目标id的消息，则等待超时时间，还没有等到消息再返回
 * @param err 错误号
 * @return void* 消息内容指针或NULL
 */
void *sht_msg_recv(sht_msgctr_t *msgctr, unsigned int id, unsigned int timeout, unsigned int *err);

/**
 * @brief 删除消息容器
 * 
 * @param pmsgctr 消息容器指针
 * @param flag 取值shtMsgctrDeleteFlag，unforce代表如果消息的count还不为0，则不会删除
 * @return shtMessgaeErrorEnum
 */
unsigned int sht_msgctr_del(sht_msgctr_t *pmsgctr, unsigned int flag);

/**
 * @brief 删除消息
 * 
 * @param pmsg 消息指针
 * @return unsigned int 0成功
 */
unsigned int sht_msg_del(sht_msg_t *pmsg);

/**
 * @brief 打印消息容器上全部消息，不会消耗消息的count
 * 
 * @param msgctr 消息容器指针
 */
void sht_print_all_msg(sht_msgctr_t *msgctr);

#endif
