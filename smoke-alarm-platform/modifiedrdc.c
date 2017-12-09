#include "contiki-conf.h"
#include "dev/radio.h"
#include "net/netstack.h"
#include "sys/rtimer.h"
#include "sys/pt.h"
#include "net/rime/rime.h"
#include "net/mac/mac-sequence.h"
#include "modifiedrdc.h"

#define STROBE_TIME             2228  /* 2228 / 32768 ~= 68 ms */
#define INTER_PACKET_INTERVAL   65   /* 65 / 32768 ~= 2 ms */

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINTDEBUG(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#define PRINTDEBUG(...)
#endif

static volatile uint8_t modifiedrdc_is_on = 0;
static volatile uint8_t radio_is_on = 0;
static volatile uint8_t we_are_sending = 0;

static void
init(void)
{
  radio_is_on = 0;
  modifiedrdc_is_on = 1;
  PRINTF("ModifiedRDC: init\n");
}
/*---------------------------------------------------------------------------*/
static int
send_one_packet(mac_callback_t sent, void *ptr)
{
  rtimer_clock_t t0, wt;
  int strobes, transmit_len, ret, is_broadcast;
  uint8_t collisions;

  packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &linkaddr_node_addr);

  if(NETSTACK_FRAMER.create() < 0) {
    /* Failed to allocate space for headers */
    PRINTF("ModifiedRDC: framer failed\n");
    return MAC_TX_ERR_FATAL;
  }

  if(packetbuf_holds_broadcast()) {
    is_broadcast = 1;
    PRINTDEBUG("ModifiedRDC: send broadcast\n");
  }

  transmit_len = packetbuf_totlen();

  if(NETSTACK_RADIO.receiving_packet() || NETSTACK_RADIO.pending_packet()) {
    PRINTF("ModifiedRDC: collision receiving %d, pending %d\n",
           NETSTACK_RADIO.receiving_packet(), NETSTACK_RADIO.pending_packet());
    return MAC_TX_COLLISION;
  }

  t0 = RTIMER_NOW();
  for(strobes = 0; RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + STROBE_TIME); strobes++) {
    NETSTACK_RADIO.prepare(packetbuf_hdrptr(), transmit_len);
    ret = NETSTACK_RADIO.transmit(transmit_len);
    
    if(ret == RADIO_TX_COLLISION) {
      collisions++;
      PRINTF("ModifiedRDC: collisions while sending\n");
    }
  }

  PRINTF("ModifiedRDC: send (strobes=%u, len=%u, %s), done\n", strobes,
         packetbuf_totlen(),
         collisions ? "collision" : "no collision");

  ret = MAC_TX_OK;
  mac_call_sent_callback(sent, ptr, ret, 1);
  return ret;
}
/*---------------------------------------------------------------------------*/
static void
send_packet(mac_callback_t sent, void *ptr)
{
  send_one_packet(sent, ptr);
}
/*---------------------------------------------------------------------------*/
static void
send_list(mac_callback_t sent, void *ptr, struct rdc_buf_list *buf_list)
{
  while(buf_list != NULL) {
    /* We backup the next pointer, as it may be nullified by
     * mac_call_sent_callback() */
    struct rdc_buf_list *next = buf_list->next;
    int last_sent_ok;

    queuebuf_to_packetbuf(buf_list->buf);
    last_sent_ok = send_one_packet(sent, ptr);

    /* If packet transmission was not successful, we should back off and let
     * upper layers retransmit, rather than potentially sending out-of-order
     * packet fragments. */
    if(!last_sent_ok) {
      return;
    }
    buf_list = next;
  }
}
/*---------------------------------------------------------------------------*/
static void
input_packet(void)
{
#if NULLRDC_SEND_802154_ACK
  int original_datalen;
  uint8_t *original_dataptr;

  original_datalen = packetbuf_datalen();
  original_dataptr = packetbuf_dataptr();
#endif

#if NULLRDC_802154_AUTOACK
  if(packetbuf_datalen() == ACK_LEN) {
    /* Ignore ack packets */
    PRINTF("ModifiedRDC: ignored ack\n"); 
  } else
#endif /* NULLRDC_802154_AUTOACK */
  if(NETSTACK_FRAMER.parse() < 0) {
    PRINTF("ModifiedRDC: failed to parse %u\n", packetbuf_datalen());
#if NULLRDC_ADDRESS_FILTER
  } else if(!linkaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER),
                                         &linkaddr_node_addr) &&
            !packetbuf_holds_broadcast()) {
    PRINTF("ModifiedRDC: not for us\n");
#endif /* NULLRDC_ADDRESS_FILTER */
  } else {
    int duplicate = 0;

//#if NULLRDC_802154_AUTOACK || NULLRDC_802154_AUTOACK_HW
#if RDC_WITH_DUPLICATE_DETECTION
    /* Check for duplicate packet. */
    duplicate = mac_sequence_is_duplicate();
    if(duplicate) {
      /* Drop the packet. */
      PRINTF("ModifiedRDC: drop duplicate link layer packet %u\n",
             packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO));
    } else {
      mac_sequence_register_seqno();
    }
#endif /* RDC_WITH_DUPLICATE_DETECTION */
//#endif /* NULLRDC_802154_AUTOACK */

#if NULLRDC_SEND_802154_ACK
    {
      frame802154_t info154;
      frame802154_parse(original_dataptr, original_datalen, &info154);
      if(info154.fcf.frame_type == FRAME802154_DATAFRAME &&
         info154.fcf.ack_required != 0 &&
         linkaddr_cmp((linkaddr_t *)&info154.dest_addr,
                      &linkaddr_node_addr)) {
        uint8_t ackdata[ACK_LEN] = {0, 0, 0};

        ackdata[0] = FRAME802154_ACKFRAME;
        ackdata[1] = 0;
        ackdata[2] = info154.seq;
        NETSTACK_RADIO.send(ackdata, ACK_LEN);
      }
    }
#endif /* NULLRDC_SEND_ACK */
    if(!duplicate) {
      NETSTACK_MAC.input();
    }
  }
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  PRINTF("ModifiedRDC: on\n");
  
  if(modifiedrdc_is_on == 0) {
    modifiedrdc_is_on = 1;
    radio_is_on = 1;
    NETSTACK_RADIO.on();
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
off(int keep_radio_on)
{
  PRINTF("ModifiedRDC: off\n");
  
  modifiedrdc_is_on = 0;
  radio_is_on = 0;
  
  return NETSTACK_RADIO.off();
}
/*---------------------------------------------------------------------------*/
static unsigned short
channel_check_interval(void)
{
  PRINTF("ModifiedRDC: channel_check_interval\n");
  return 0;
}
/*---------------------------------------------------------------------------*/
const struct rdc_driver modifiedrdc_driver = {
  "ModifiedRDC",
  init,
  send_packet,
  send_list,
  input_packet,
  on,
  off,
  channel_check_interval,
};
