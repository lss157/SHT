/**
 * @file int.h
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，SHT中断相关头文件
 * @version 1.0
 * @date 2022-07-08
 * @copyright Copyright (c) 2023
 * @revisionHistory 
 *  <table> 
 *   <tr><th> 版本 <th>作者 <th>日期 <th>修改内容 
 *   <tr><td> 0.1 <td>jivin <td>2010-03-08 <td>Created 
 *   <tr><td> 1.0 <td>王彬浩 <td> 2022-07-08 <td>Standardized 
 *  </table>
 */
#ifndef SHT_INT_H
#define SHT_INT_H

/**
 * @brief 中断结构体
 * 
 */
typedef struct {
	unsigned char  type;				///<上面三种中断类型
	void (*isr)(int);		///<中断服务程序
	void (*enter)(int);	///<中断服务程序执行之前执行的操作，aCroal中为hal_intr_ack函数
	void (*exit)(int);	///<中断服务程序执行完成后的操作，比如置中断结束，目前SHT中没有这个操作
	void (*mask)(int);	///<除能中断操作
	void (*unmask)(int);	///<使能中断操作
}sht_intr_ctr_t;

/* 将中断使能/除能加上中断嵌套 */
// long sht_intr_disable(void);
// void sht_intr_enable(long level);

// long sht_enter_critical(void);
// void sht_exit_critical(long level);

/**
 * @brief SHT全局中断打开
 * 
 */
#define sht_intr_enable(level) hw_interrupt_enable(level)

/**
 * @brief SHT全局中断关闭
 * 
 */
#define sht_intr_disable() hw_interrupt_disable()

/**
 * @brief SHT进入临界区（本质就是关中断）
 * v2.0加入中断嵌套功能
 */
#define sht_enter_critical() HAL_ENTER_CRITICAL()

/**
 * @brief SHT退出临界区（本质就是开中断）
 * v2.0加入中断嵌套功能
 */
#define sht_exit_critical(level) HAL_EXIT_CRITICAL(level)

#define sht_intr_nesting interrupt_get_nest()
#define sht_intr_nesting_inc() HAL_INTR_NESTING_INC()
#define sht_intr_nesting_dec() HAL_INTR_NESTING_DEC()


void sht_default_isr(int vector);
void sht_intr_sys_init();

/***************中断相关API****************/

/**
 * @brief 给某个plic中断绑定中断服务函数
 * 
 * @param vector 中断号
 * @param isr 中断服务函数
 * @return int 0 success
 */
int sht_intr_attach(int vector,void (*isr)(int));

/**
 * @brief 给某个plic中断解绑中断服务函数，并换成SHT默认的中断服务函数sht_default_isr
 * 
 * @param vector 中断号
 * @return int 0 success
 */
int sht_intr_detach(int vector);

/**
 * @brief 使能某个中断
 * 
 * @param vector 中断号
 * @return int 返回0成功，其它失败
 */
int sht_intr_unmask(int vector);

/**
 * @brief 除能某个中断
 * 
 * @param vector 中断号
 * @return int 返回0成功，其它失败
 */
int sht_intr_mask(int vector);



#endif
