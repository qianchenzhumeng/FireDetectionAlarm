/**
 * \file
 *         
 * \author
 *         Mr.Y <qianchenzhumeng@live.cn>
 */

#include "contiki.h"
#include "fire-detect.h"
#include "main-process.h"
#include "dev/temperature-sensor.h"
#include "dev/smoke-sensor.h"

//#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

process_event_t event_self_alarm;
process_event_t event_self_alarm_release;

PROCESS(fire_detect_process, "fire detect process");
//AUTOSTART_PROCESSES(&fire_detect_process);

/*---------------------------------------------------------------------------*/
uint8_t fire(void)
{
    uint8_t r = 0;
    int smoke;
    float temperature;
    smoke = smoke_sensor.value(0);
    temperature = temperature_sensor.value(0) / 10.0;
    PRINTF("smoke: %d, temperature: %.1f\n", smoke, temperature);
    // TO-DO
    return r;
}

PROCESS_THREAD(fire_detect_process, ev, data)
{
    static struct etimer period_timer;
    static uint8_t fire_flag = 0, counter = 0;
    
    PROCESS_BEGIN();
    
    SENSORS_ACTIVATE(smoke_sensor);
    SENSORS_ACTIVATE(temperature_sensor);
    
    //等待主线程就绪
    PROCESS_WAIT_EVENT_UNTIL(ev == event_main_process_ready);
    etimer_set(&period_timer, CLOCK_SECOND);
    
    while(1) {
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && data == &period_timer);
        PRINTF("fire detect...\n");
        if(fire()) {
            if(!fire_flag) {
                if(counter < 3) {
                    counter++;
                }
                counter++;
                switch(counter) {
                    case 1:
                        break;
                    case 2:
                        break;
                    case 3:
                        fire_flag = 1;
                        event_self_alarm = process_alloc_event();
                        process_post(&main_process, event_self_alarm, NULL);
                        break;
                    default: break;
                }
            }
            etimer_set(&period_timer, CLOCK_SECOND*4);
        } else {
            if(fire_flag) {
                event_self_alarm_release = process_alloc_event();
                process_post(&main_process, event_self_alarm_release, NULL);
                fire_flag = 0;
                counter = 0;
            }
            etimer_set(&period_timer, CLOCK_SECOND*8);
        } 
    }
    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
