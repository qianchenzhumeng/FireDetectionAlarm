/*---------------------------------------------------------------------------*/
#ifndef __CONTIKI_CONF_H__
#define __CONTIKI_CONF_H__
/*---------------------------------------------------------------------------*/
#include "platform-conf.h"
/*---------------------------------------------------------------------------*/

/* Code Shortcuts */
/*
 * When set, this directive also configures the following bypasses:
 *   - process_post_synch() in tcpip_input() (we call packet_input())
 *   - process_post_synch() in tcpip_uipcall (we call the relevant pthread)
 *   - mac_call_sent_callback() is replaced with sent() in various places
 *
 * These are good things to do, they reduce stack usage and prevent crashes
 */
#define NETSTACK_CONF_SHORTCUTS   1

/* Network Stack */
#ifndef NETSTACK_CONF_NETWORK
#if NETSTACK_CONF_WITH_IPV6
#define NETSTACK_CONF_NETWORK sicslowpan_driver
#else
#define NETSTACK_CONF_NETWORK rime_driver
#endif /* NETSTACK_CONF_WITH_IPV6 */
#endif /* NETSTACK_CONF_NETWORK */

#ifndef NETSTACK_CONF_MAC
#define NETSTACK_CONF_MAC     csma_driver
#endif

#ifndef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC     contikimac_driver
#define NULLRDC_802154_AUTOACK 1
#define NULLRDC_802154_AUTOACK_HW 1
#endif

#ifndef NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE
#define NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE 8
#endif

#ifndef NETSTACK_CONF_FRAMER
#define NETSTACK_CONF_FRAMER  framer_802154
#endif

/* radio driver blocks until ACK is received */
#define NULLRDC_CONF_ACK_WAIT_TIME          (0)
#define CONTIKIMAC_CONF_BROADCAST_RATE_LIMIT 0
#define IEEE802154_CONF_PANID       0xABCD

//#define AODV_COMPLIANCE

/*---------------------------------------------------------------------------*/
/* include the project config */
/* PROJECT_CONF_H might be defined in the project Makefile */
#ifdef PROJECT_CONF_H
#include PROJECT_CONF_H
#endif /* PROJECT_CONF_H */
/*---------------------------------------------------------------------------*/
#endif /* CONTIKI_CONF_H */
/*---------------------------------------------------------------------------*/
