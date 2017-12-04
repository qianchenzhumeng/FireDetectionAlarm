/**
 * \file
 *         Set the Rime address.
 * \author
 *         Mr.Y <qianchenzhumeng@live.cn>
 */

 #if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#include "contiki.h"
#include "get-address.h"
#include "address-verify.h"
#include "main-process.h"

static struct address_verify_conn address_verify;

process_event_t event_addr_set_success;
process_event_t event_addr_verify_success;
process_event_t event_addr_verify_fail;

/*---------------------------------------------------------------------------*/
PROCESS(get_address_process, "set rime address process");
//AUTOSTART_PROCESSES(&get_address_process);

/*---------------------------------------------------------------------------*/
static void found_address(struct address_verify_conn *rdc, const linkaddr_t *dest)
{
    event_addr_verify_fail = process_alloc_event();
    process_post(&get_address_process, event_addr_verify_fail, NULL);
}

/*---------------------------------------------------------------------------*/
static void route_timed_out(struct address_verify_conn *rdc)
{
    PRINTF("get address timeout.\n");
    event_addr_verify_success = process_alloc_event();
    process_post(&get_address_process, event_addr_verify_success, NULL);
}

/*---------------------------------------------------------------------------*/
static const struct address_verify_callbacks address_verify_callbacks =
    { found_address, route_timed_out };

PROCESS_THREAD(get_address_process, ev, data)
{
    static uint8_t addr_u8_0, addr_u8_1;
    static linkaddr_t dest, temp_addr;
    uint16_t channels = 132;
    
    PROCESS_BEGIN();

    /* 设置临时地址 */
    temp_addr.u8[0] = 255;
    temp_addr.u8[1] = 255;
     linkaddr_set_node_addr(&temp_addr);
    
    route_init();
    address_verify_open(&address_verify,
                         CLOCK_SECOND * 2,
                         channels,
                         &address_verify_callbacks);
  
    for(addr_u8_0 = 1; addr_u8_0 < 254; addr_u8_0++)
    {
        dest.u8[0] = addr_u8_0;
        dest.u8[1] = addr_u8_1;
        PRINTF("get_address_process: target address %d.%d\n", addr_u8_0, addr_u8_1);
        address_verify_verifier(&address_verify, &dest, PACKET_TIMEOUT);
        PROCESS_WAIT_EVENT_UNTIL(ev == event_addr_verify_success ||
                                     ev == event_addr_verify_fail);
        if(ev == event_addr_verify_success)
        {
            linkaddr_set_node_addr(&dest);
            // 更新洪泛连接储存的包特征
            // 最近一次包发送方的地址设置为自己的地址
            linkaddr_copy(&address_verify.areqconn.last_originator, &linkaddr_node_addr);
            // 最近一次洪泛包的序号设置为0
            address_verify.areqconn.last_originator_seqno = 0;
            PRINTF("set_rime_address: address was set as: %d.%d\n", linkaddr_node_addr.u8[0],linkaddr_node_addr.u8[1]);
            break;
        }
    }
   
    event_addr_set_success = process_alloc_event();
    process_post(&main_process, event_addr_set_success, NULL);

    while(1){
        PROCESS_YIELD();
    }
    PROCESS_END();
}

/** @} */
