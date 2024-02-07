#include "acoral.h"

void test_func(void *args)
{
	acoral_print("test\r\n");
}

void user_main(void)
{
	

	acoral_period_policy_data_t* test_thread_data;
	test_thread_data = acoral_malloc(sizeof(acoral_period_policy_data_t));
	test_thread_data->prio = 10;
	test_thread_data->prio_type = ACORAL_HARD_PRIO;
	test_thread_data->time = 2000; //ms为单位
	acoral_create_thread(test_func, 512, NULL, "test_thread", NULL, ACORAL_SCHED_POLICY_PERIOD, test_thread_data); //超声波测距 记录当前线程的ID

}

