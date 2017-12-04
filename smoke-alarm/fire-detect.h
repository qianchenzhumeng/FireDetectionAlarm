/**
 * \file
 *         Fire detect
 * \author
 *         Mr.Y <qianchenzhumeng@live.cn>
 */

#ifndef __FIRE_DETECT_H
#define __FIRE_DETECT_H

#include "contiki.h"

extern process_event_t event_self_alarm;
extern process_event_t event_self_alarm_release;

PROCESS_NAME(fire_detect_process);

#endif /* __FIRE_DETECT_H */