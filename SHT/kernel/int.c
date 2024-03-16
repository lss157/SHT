/**
 * @file int.c
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，中断相关函数
 * @version 1.0
 * @date 2022-07-24
 * @copyright Copyright (c) 2023
 * @revisionHistory
 *  <table>
 *   <tr><th> 版本 <th>作者 <th>日期 <th>修改内容
 *   <tr><td> 0.1 <td>jivin <td>2010-03-08 <td>Created
 *   <tr><td> 1.0 <td>王彬浩 <td> 2022-07-24 <td>Standardized
 *  </table>
 */

#include "hal.h"
#include "lsched.h"
#include "int.h"
#include <stdio.h>

void sht_intr_sys_init()
{

}

int sht_intr_attach(int vector,void (*isr)(int)){

}

int sht_intr_detach(int vector){

}

int sht_intr_unmask(int vector){

}

int sht_intr_mask(int vector){
}

void sht_default_isr(int vector){
	sht_print("in sht_default_isr");
}

/**
 * @brief 中断退出函数
 * 
 */
void sht_intr_exit()
{
	if (!sht_need_sched)
		return;

	if (sht_intr_nesting)
		return;

	if (sched_lock)
		return;

	if (!sht_start_sched)
		return;

	/*如果需要调度，则调用此函数*/
	sht_sched();
}


/**
 * @brief 设置中断类型，SHT_COMM_INTR、SHT_EXPERT_INTR或SHT_RT_INTR
 * 
 * @param vector 
 * @param type 
 */
void sht_intr_set_type(int vector, unsigned char type)
{

}

/**
 * @brief 异常错误输出，SHT除了中断异常以外的异常发生后，都会通过EXP_HANDLER进入这个函数打印异常信息
 * 
 * @param lr 链接寄存器，保存异常发生时的pc
 * @param stack 栈指针，保存发生异常时线程的栈
 */
void sht_fault_entry(int lr, int *stack)
{
	sht_intr_disable();
	if (!sht_start_sched)
		while (1)
			;
	sht_print("Exception occur\n");
	sht_print("******************\n");
	sht_print("Thread name:%s\n", sht_cur_thread->name);
	sht_print("Thread prio:%d\n", sht_cur_thread->prio);
	sht_print("Thread stack_size:%d\n", sht_cur_thread->stack_size);
	sht_print("Thread stack_buttom:0x%x\n", sht_cur_thread->stack_buttom);
	sht_print("Thread stack:0x%x\n", sht_cur_thread->stack);
	sht_print("Pc:0x%x\n", lr);
	sht_print("Stack:0x%x\n", stack);
	sht_print("******************\n");
	while (1)
		;
}