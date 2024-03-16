#include "sys_app.h"
#include "sht.h"
#include "subghz_phy_app.h"
#include "dht11.h"

void user_main(void)
{
    /* LoRa线程 */
	sht_period_policy_data_t* lora_thread_data;
	lora_thread_data = sht_malloc(sizeof(sht_period_policy_data_t));
	lora_thread_data->prio = 10;
	lora_thread_data->prio_type = SHT_HARD_PRIO;
	lora_thread_data->time = 4000; //ms为单位
	sht_create_thread(PingPong_Process, 512, NULL, "lora_thread", NULL, SHT_SCHED_POLICY_PERIOD, lora_thread_data); 

    /* 温湿度线程 */
//    sht_period_policy_data_t* tmp_humi_thread_data;
//	tmp_humi_thread_data = sht_malloc(sizeof(sht_period_policy_data_t));
//	tmp_humi_thread_data->prio = 10;
//	tmp_humi_thread_data->prio_type = SHT_HARD_PRIO;
//	tmp_humi_thread_data->time = 5000;
//	sht_create_thread(get_temp_humi_thread,512,NULL,"tmp_humi_thread",NULL,SHT_SCHED_POLICY_PERIOD,tmp_humi_thread_data);
}

