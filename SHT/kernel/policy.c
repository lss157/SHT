/**
 * @file policy.c
 * @author 王彬浩 (SPGGOGOGO@outlook.com)
 * @brief kernel层，调度策略
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
#include "hal.h"
#include "thread.h"
#include "policy.h"
#include "int.h"
#include <stdio.h>
#include "comm_thrd.h"
#include "period_thrd.h"

sht_list_t policy_list;

sht_sched_policy_t *sht_get_policy_ctrl(unsigned char type){
	sht_list_t   *tmp,*head;
	sht_sched_policy_t  *policy_ctrl;
	head=&policy_list;
	tmp=head;
	for(tmp=head->next;tmp!=head;tmp=tmp->next){
		policy_ctrl=list_entry(tmp,sht_sched_policy_t,list);
		if(policy_ctrl->type==type)
			return policy_ctrl;
	}
	return NULL;
}

int sht_policy_thread_init(unsigned int policy,sht_thread_t *thread,void (*route)(void *args),void *args,void *data){
	sht_sched_policy_t   *policy_ctrl;
	policy_ctrl=sht_get_policy_ctrl(policy);	
	if(policy_ctrl==NULL||policy_ctrl->policy_thread_init==NULL){
		long level = sht_enter_critical();
		sht_release_res((sht_res_t *)thread);
		sht_exit_critical(level);
		sht_print("No thread policy support:%d\n",thread->policy);
		return -1;
	}
	return policy_ctrl->policy_thread_init(thread,route,args,data);
}

void sht_register_sched_policy(sht_sched_policy_t *policy){
	sht_list_add2_tail(&policy->list,&policy_list);
}

void sht_policy_delay_deal(){
	sht_list_t   *tmp,*head;
	sht_sched_policy_t  *policy_ctrl;
	head=&policy_list;
	tmp=head;
	for(tmp=head->next;tmp!=head;tmp=tmp->next){
		policy_ctrl=list_entry(tmp,sht_sched_policy_t,list);
		if(policy_ctrl->delay_deal!=NULL)
			policy_ctrl->delay_deal();
	}
}

void sht_policy_thread_release(sht_thread_t *thread){
	sht_sched_policy_t   *policy_ctrl;
	policy_ctrl=sht_get_policy_ctrl(thread->policy);
	if(policy_ctrl->policy_thread_release!=NULL)
		policy_ctrl->policy_thread_release(thread);
}


void sht_sched_policy_init(){
	sht_init_list(&policy_list);
	comm_policy_init();

#ifdef CFG_THRD_PERIOD
	period_policy_init();
#endif
}


