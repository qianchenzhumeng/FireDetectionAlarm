/**
 * \file
 *         Fire detect
 * \author
 *         Mr.Y <qianchenzhumeng@live.cn>
 */

#ifndef __MAIN_PROCESS_H
#define __MAIN_PROCESS_H

#include "contiki.h"

typedef enum {OTHER_ALARM, OTHER_ALARM_RELEASE} alarm_msg_type;

typedef struct alarm_msg {
    alarm_msg_type type;
    uint8_t id;
}alarm_msg_t;

extern process_event_t event_main_process_ready;

PROCESS_NAME(main_process);

#endif /* __FIRE_DETECT_H */