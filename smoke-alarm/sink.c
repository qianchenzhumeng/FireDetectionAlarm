/**
 * \file
 *         
 * \author
 *         Mr.Y <qianchenzhumeng@live.cn>
 */

#include "contiki.h"
#include "net/rime/mesh.h"

#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "dev/serial-line.h"
#if CONTIKI_TARGET_SKY
#include "dev/uart1.h"
#endif
#include <string.h>

//#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static struct mesh_conn mesh;
/*---------------------------------------------------------------------------*/
PROCESS(process, "Alarm message transmision");
AUTOSTART_PROCESSES(&process);
/*---------------------------------------------------------------------------*/

static void mesh_recv(struct mesh_conn *c, const linkaddr_t *from, uint8_t hops)
{
	PRINTF("Data received from %d.%d: %.*s (%d)\n",
		from->u8[0], from->u8[1],
		packetbuf_datalen(), (char *)packetbuf_dataptr(), packetbuf_datalen());
}

/*---------------------------------------------------------------------------*/

static const struct mesh_callbacks mesh_call = {mesh_recv, NULL, NULL};

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(process, ev, data)
{
    linkaddr_t sink;

	PROCESS_BEGIN();
    
    sink.u8[0] = 0;
    sink.u8[1] = 0;
    linkaddr_set_node_addr(&sink);

	mesh_open(&mesh, 138, &mesh_call);
	SENSORS_ACTIVATE(button_sensor);

	while(1) {

		PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);
		
		PRINTF("Button clicked\n");
	}

    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
