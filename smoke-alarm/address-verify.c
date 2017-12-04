/**
 * \file
 *         Address verify protocol
 * \author
 *         Mr.Y <qianchenzhumeng@live.cn>
 */

/**
 * \addtogroup addressverify
 * @{
 */

//#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#include "contiki.h"

#include "net/rime/rime.h"
#include "address-verify.h"

#include <stddef.h> /* For offsetof */

struct add_verify_msg {
  linkaddr_t dest;
  linkaddr_t test;  //试验地址
  uint8_t areq_id;
  uint8_t pad;
};

struct arep_hdr {
  uint8_t areq_id;
  uint8_t hops;
  linkaddr_t dest;
  linkaddr_t originator;
  linkaddr_t test;  //试验地址
};

/*---------------------------------------------------------------------------*/
static char arep_pending;		/* A address reply for a request is pending. */
static linkaddr_t areq_verify_address;
/*---------------------------------------------------------------------------*/

static void areq_timeout_handler(void *ptr);
static void arep_timeout_handler(void *ptr);

static void send_areq(struct address_verify_conn *c, const linkaddr_t *dest)
{
    linkaddr_t dest_copy;
    struct add_verify_msg *msg;

    linkaddr_copy(&dest_copy, dest);
    dest = &dest_copy;

    packetbuf_clear();
    msg = packetbuf_dataptr();
    packetbuf_set_datalen(sizeof(struct add_verify_msg));

    msg->pad = 0;
    msg->areq_id = c->areq_id;
    linkaddr_copy(&msg->dest, dest);
    linkaddr_copy(&msg->test, dest);

    netflood_send(&c->areqconn, c->areq_id);
    c->areq_id++;
}

/*---------------------------------------------------------------------------*/
static void send_arep(struct address_verify_conn *c, const linkaddr_t *dest, const linkaddr_t *test)
{
    struct arep_hdr *arepmsg;
    linkaddr_t saved_dest;
  
    linkaddr_copy(&saved_dest, dest);

    packetbuf_clear();
    dest = &saved_dest;
    arepmsg = packetbuf_dataptr();
    packetbuf_set_datalen(sizeof(struct arep_hdr));
    arepmsg->hops = 0;
    arepmsg->areq_id = c->areq_id;
    linkaddr_copy(&arepmsg->dest, dest);
    linkaddr_copy(&arepmsg->originator, &linkaddr_node_addr);
    linkaddr_copy(&arepmsg->test, test);
    
    netflood_send(&c->arepconn, c->areq_id);
}

static void insert_route(const linkaddr_t *originator, const linkaddr_t *last_hop,
	     uint8_t hops)
{
    PRINTF("%d.%d: Inserting %d.%d into routing table, next hop %d.%d, hop count %d\n",
        linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1],
        originator->u8[0], originator->u8[1],
        last_hop->u8[0], last_hop->u8[1],
        hops);
  
    route_add(originator, last_hop, hops, 0);
}

static int arep_packet_received(struct netflood_conn *nf, const linkaddr_t *from,
		     const linkaddr_t *originator, uint8_t seqno, uint8_t hops)
{
    struct arep_hdr *msg = packetbuf_dataptr();
    struct route_entry *rt;
    struct address_verify_conn *c = (struct address_verify_conn *)
        ((char *)nf - offsetof(struct address_verify_conn, arepconn));
    static int r = 0;
    PRINTF("%d.%d: arep_packet_received from %d.%d towards %d.%d for test %d.%d len %d\n",
        linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1],
        from->u8[0],from->u8[1],
        msg->dest.u8[0],msg->dest.u8[1],
        msg->test.u8[0], msg->test.u8[1],
        packetbuf_datalen());

    PRINTF("from %d.%d hops %d rssi %d lqi %d\n",
        from->u8[0], from->u8[1],
        hops,
        packetbuf_attr(PACKETBUF_ATTR_RSSI),
        packetbuf_attr(PACKETBUF_ATTR_LINK_QUALITY));

    insert_route(&msg->originator, from, hops);

    if(linkaddr_cmp(&areq_verify_address, &msg->test)) { //确保arep和areq一致
        if(linkaddr_cmp(&msg->dest, &linkaddr_node_addr)) {
            PRINTF("arep for us!\n");
            arep_pending = 0;
            ctimer_stop(&c->verify_t);
            if(c->cb->new_route != NULL) {
                c->cb->new_route(c, &msg->originator);
            }
            r = 0;
        } else {
            rt = route_lookup(&msg->test);
            if(rt != NULL) {
                route_remove(rt);   //收到回复，删除试验地址
                PRINTF("route_remove: %d.%d\n", rt->dest.u8[0], rt->dest.u8[1]);
            }
            r =  1;
        }
    }
    ctimer_set(&c->arep_t, PACKET_TIMEOUT, arep_timeout_handler, c);
    return r;
}

static int areq_packet_received(struct netflood_conn *nf, const linkaddr_t *from,
		     const linkaddr_t *originator, uint8_t seqno, uint8_t hops)
{
    struct add_verify_msg *msg = packetbuf_dataptr();
    struct address_verify_conn *c = (struct address_verify_conn *)
        ((char *)nf - offsetof(struct address_verify_conn, areqconn));
    struct route_entry *rt;
    static int r = 0;
    PRINTF("%d.%d: areq_packet_received: address request from %d.%d for %d.%d areq_id %d last %d.%d/%d\n",
	   linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1],
	   originator->u8[0], originator->u8[1],
       msg->dest.u8[0], msg->dest.u8[1],
       msg->areq_id,
       c->last_test.u8[0], c->last_test.u8[1],
       c->last_areq_id);
    
    if(!(linkaddr_cmp(&c->last_test, &msg->test) && c->last_areq_id == msg->areq_id)) {
        //linkaddr_copy(&areq_verify_address, &msg->test);
        linkaddr_copy(&c->last_test, &msg->test);
        c->last_areq_id = msg->areq_id;
        
        if(linkaddr_cmp(&msg->test, &linkaddr_node_addr)) {
            PRINTF("%d.%d: route_packet_received: address request for our address\n",
                linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);
            PRINTF("from %d.%d hops %d rssi %d lqi %d\n",
                from->u8[0], from->u8[1],
                hops,
                packetbuf_attr(PACKETBUF_ATTR_RSSI),
                packetbuf_attr(PACKETBUF_ATTR_LINK_QUALITY));
        
            /* Send address reply back to source. */
            send_arep(c, originator, &msg->test);
            r = 0; /* Don't continue to flood the areq packet. */
        } else {
            rt = route_lookup(&msg->test);
            if(rt != NULL) {
                send_arep(c, originator, &msg->test);
                r =  0; /* Don't continue to flood the areq packet. */
            } else {
                PRINTF("Inserting test address in to routing table:\n\t");
                if(hops) {
                    insert_route(&msg->test, from ,hops);   //暂时把试验地址加入路由表
                } else {
                    insert_route(&msg->test, &msg->test ,hops);   //暂时把试验地址加入路由表
                }
                PRINTF("from %d.%d hops %d rssi %d lqi %d\n",
                    from->u8[0], from->u8[1],
                    hops,
                    packetbuf_attr(PACKETBUF_ATTR_RSSI),
                    packetbuf_attr(PACKETBUF_ATTR_LINK_QUALITY));
                r = 1;
            }
        }
    }
    ctimer_set(&c->areq_t, PACKET_TIMEOUT, areq_timeout_handler, c);
    return r; /* Don't forward packet. */
}

static void netflood_sent(struct netflood_conn *c)
{
    // 更新洪泛连接储存的包特征
    // 最近一次包发送方的地址设置为自己的地址
    c->last_originator.u8[0] = linkaddr_node_addr.u8[0];
    c->last_originator.u8[1] = linkaddr_node_addr.u8[1];
}

static const struct netflood_callbacks arep_callbacks = {arep_packet_received, NULL, NULL};
static const struct netflood_callbacks areq_callbacks = {areq_packet_received, netflood_sent, NULL};

void address_verify_explicit_open(struct address_verify_conn *c,
			     clock_time_t time,
			     uint16_t areq_channel,
			     uint16_t arep_channel,
			     const struct address_verify_callbacks *callbacks)
{
    netflood_open(&c->areqconn, time, areq_channel, &areq_callbacks);
    netflood_open(&c->arepconn, time, arep_channel, &arep_callbacks);
    c->cb = callbacks;
}

void address_verify_open(struct address_verify_conn *c,
		     clock_time_t time,
		     uint16_t channels,
		     const struct address_verify_callbacks *callbacks)
{
    address_verify_explicit_open(c, time, channels + 0, channels + 1, callbacks);
}

void address_verify_close(struct address_verify_conn *c)
{
    netflood_close(&c->arepconn);
    netflood_close(&c->areqconn);
    ctimer_stop(&c->verify_t);
    ctimer_stop(&c->arep_t);
    ctimer_stop(&c->areq_t);
}
/*---------------------------------------------------------------------------*/
static void verify_timeout_handler(void *ptr)
{
    struct address_verify_conn *c = ptr;
    PRINTF("address_verify: timeout\n");
    arep_pending = 0;
    if(c->cb->timedout) {
        c->cb->timedout(c);
    }
}

static void areq_timeout_handler(void *ptr)
{
    struct address_verify_conn *c = ptr;
    linkaddr_copy(&c->areqconn.last_originator, &linkaddr_node_addr);
    c->areqconn.last_originator_seqno = 0;
    PRINTF("areqconn: Reset the last_originator and last_originator_seqno.\n");
}

static void arep_timeout_handler(void *ptr)
{
    struct address_verify_conn *c = ptr;
    linkaddr_copy(&c->arepconn.last_originator, &linkaddr_node_addr);
    c->arepconn.last_originator_seqno = 0;
    PRINTF("arepconn: Reset the last_originator and last_originator_seqno.\n");
}
/*---------------------------------------------------------------------------*/
int address_verify_verifier(struct address_verify_conn *c, const linkaddr_t *test,
			 clock_time_t timeout)
{
    if(arep_pending) {
        PRINTF("address_verify_send: ignoring request because of pending response\n");
        return 0;
    }

    PRINTF("address_verify_send: sending address request\n");
    ctimer_set(&c->verify_t, timeout, verify_timeout_handler, c);
    arep_pending = 1;
    linkaddr_copy(&areq_verify_address, test);
    send_areq(c, test);
    return 1;
}
/*---------------------------------------------------------------------------*/
/** @} */
