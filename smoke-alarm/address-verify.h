/**
 * \file
 *         Header file for the address verify protocol
 * \author
 *         Mr.Y <qianchenzhumeng@live.cn>
 */

/**
 * The ibc module uses 2 channels; one for the flooded address request
 * packets and one for the flooded address replies.
 *
 */
#ifndef __ADDRESS_VERIFY_H
#define __ADDRESS_VERIFY_H

#include "net/rime/unicast.h"
#include "net/rime/netflood.h"
#include "sys/ctimer.h"

#define PACKET_TIMEOUT (CLOCK_SECOND * 10)

struct address_verify_conn;

struct address_verify_callbacks {
    void (* new_route)(struct address_verify_conn *c, const linkaddr_t *to);
    void (* timedout)(struct address_verify_conn *c);
};

struct address_verify_conn {
    struct netflood_conn areqconn;
    struct netflood_conn arepconn;
    linkaddr_t last_test;
    uint16_t last_areq_id;
    uint16_t areq_id;
    struct ctimer verify_t;
    struct ctimer areq_t;  //
    struct ctimer arep_t;
    const struct address_verify_callbacks *cb;
};

void address_verify_open(struct address_verify_conn *c, clock_time_t time,
			  uint16_t channels,
			  const struct address_verify_callbacks *callbacks);
void address_verify_explicit_open(struct address_verify_conn *c, clock_time_t time,
				   uint16_t areq_channel,
				   uint16_t arep_channel,
				   const struct address_verify_callbacks *callbacks);
int address_verify_verifier(struct address_verify_conn *c, const linkaddr_t *test,
			     clock_time_t timeout);

void address_verify_close(struct address_verify_conn *c);

#endif /* __ADDRESS_VERIFY_H */
/** @} */
/** @} */
