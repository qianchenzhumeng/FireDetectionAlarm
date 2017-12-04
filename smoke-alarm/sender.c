/**
 * \file
 *         
 * \author
 *         Mr.Y <qianchenzhumeng@live.cn>
 */

#include "contiki.h"
#include "net/rime/rime.h"
#include "net/linkaddr.h"
#include "net/rime/mesh.h"

#include "dev/button-sensor.h"
#include "dev/buzzer-sensor.h"
#include "dev/leds.h"
#include "dev/serial-line.h"
#if CONTIKI_TARGET_SKY
#include "dev/uart1.h"
#endif
#include "main-process.h"
#include "get-address.h"
#include "fire-detect.h"
#include "sys/ctimer.h"

//#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define MESSAGE "Smoke alarm!"

static linkaddr_t sink_addr;
static struct mesh_conn mesh;
static struct broadcast_conn broadcast;
process_event_t event_main_process_ready;
process_event_t event_other_alarm;
process_event_t event_other_alarm_release;

static uint8_t self_alarm_flag = 0, other_alarm_flag = 0;
static struct ctimer other_alarm_t;

static struct ctimer test_t;

/*---------------------------------------------------------------------------*/
PROCESS(main_process, "Alarm message transmision");
AUTOSTART_PROCESSES(&main_process, &get_address_process, &fire_detect_process);
/*---------------------------------------------------------------------------*/

static void mesh_sent(struct mesh_conn *c)
{
	PRINTF("mesh packet sent\n");
}

static void mesh_timeout(struct mesh_conn *c)
{
	PRINTF("mesh packet timeout\n");
}

static void mesh_recv(struct mesh_conn *c, const linkaddr_t *from, uint8_t hops)
{
	PRINTF("Data received from %d.%d: %.*s (%d)\n",
		from->u8[0], from->u8[1],
		packetbuf_datalen(), (char *)packetbuf_dataptr(), packetbuf_datalen());

}

static void other_alarm_timeout_handler(void *ptr) {
    other_alarm_flag = 1;
    event_other_alarm_release = process_alloc_event();
    process_post(&main_process, event_other_alarm_release, NULL);
}

static void test_over_handler(void *ptr) {
    PRINTF("test is over\n");
    self_alarm_flag = 0;
    leds_off(LEDS_RED);
    SENSORS_DEACTIVATE(buzzer_sensor);
}

static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
    static uint8_t type;
    static uint8_t *msg;
    msg = (uint8_t *)packetbuf_dataptr();
    
    PRINTF("broadcast message received from %d.%d: %d/%d\n",
		from->u8[0], from->u8[1],
        *((uint8_t *)packetbuf_dataptr()), *((uint8_t *)packetbuf_dataptr()+1));
    type = *msg;
    switch(type) {
        case OTHER_ALARM:
            if(!other_alarm_flag) {
            other_alarm_flag = 1;
            event_other_alarm = process_alloc_event();
            process_post(&main_process, event_other_alarm, NULL);
            ctimer_set(&other_alarm_t, CLOCK_SECOND * 8, other_alarm_timeout_handler, NULL);
            } else {
                ctimer_restart(&other_alarm_t);
            }
            break;
        case OTHER_ALARM_RELEASE:
            other_alarm_flag = 0;
            event_other_alarm_release = process_alloc_event();
            process_post(&main_process, event_other_alarm_release, NULL);
            ctimer_stop(&other_alarm_t);
            break;
        default: break;
    }
}

/*---------------------------------------------------------------------------*/

static const struct mesh_callbacks mesh_call = {mesh_recv, mesh_sent, mesh_timeout};
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(main_process, ev, data)
{
    alarm_msg_t msg = {0};
    static uint8_t msg_id = 0;
    PROCESS_EXITHANDLER(mesh_close(&mesh); broadcast_close(&broadcast);)

    PROCESS_BEGIN();
    PRINTF("I'm a sender!\n");
    mesh_open(&mesh, 138, &mesh_call);
    broadcast_open(&broadcast, 129, &broadcast_call);
    SENSORS_ACTIVATE(button_sensor);
    sink_addr.u8[0] = 0;
    sink_addr.u8[1] = 0;

    PROCESS_WAIT_EVENT_UNTIL(ev == event_addr_set_success);
    
    event_main_process_ready = process_alloc_event();
    process_post(&fire_detect_process, event_main_process_ready, NULL);
    
    while(1) {
        PROCESS_WAIT_EVENT();
        if(ev == sensors_event && data == &button_sensor) {
            PRINTF("Button clicked\n");
            self_alarm_flag = 1;
            leds_on(LEDS_RED);
            msg.type = OTHER_ALARM;
            msg.id = msg_id;
            msg_id++;
            packetbuf_copyfrom(&msg, sizeof(msg));
            broadcast_send(&broadcast);
            SENSORS_ACTIVATE(buzzer_sensor);
            packetbuf_copyfrom(&msg, sizeof(msg));
            //mesh_send(&mesh, &sink_addr);
            ctimer_set(&test_t, CLOCK_SECOND * 8, test_over_handler, NULL);
        }
        if(ev == event_self_alarm) {
            PRINTF("fire alarm from self!\n");
            self_alarm_flag = 1;
            leds_on(LEDS_RED);
            SENSORS_ACTIVATE(buzzer_sensor);
            msg.type = OTHER_ALARM;
            msg.id = msg_id;
            msg_id++;
            packetbuf_copyfrom(&msg, sizeof(msg));
            broadcast_send(&broadcast);
            packetbuf_copyfrom(&msg, sizeof(msg));
            //mesh_send(&mesh, &sink_addr);
            // TO-DO
        }
        if(ev == event_self_alarm_release) {
            self_alarm_flag = 0;
            leds_off(LEDS_RED);
            if(!self_alarm_flag) {
                SENSORS_DEACTIVATE(buzzer_sensor);
            }
            // TO-DO
        }
        if(ev == event_other_alarm) {
            PRINTF("fire alarm from other node!\n");
            other_alarm_flag = 1;
            leds_on(LEDS_YELLOW);
            SENSORS_ACTIVATE(buzzer_sensor);
            // TO-DO
        }
        if(ev == event_other_alarm_release) {
            PRINTF("release the fire alarm of other node!\n");
            other_alarm_flag = 0;
            leds_off(LEDS_YELLOW);
            if(!self_alarm_flag) {
                SENSORS_DEACTIVATE(buzzer_sensor);
            }
            // TO-DO
        }
    }

    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
