#ifndef LPM_H_
#define LPM_H_

typedef enum
{
    STOP_MODE_WITH_RTC,
    LOW_POWER_SLEEP_MODE
}LPM_MODE;

void lpm_enter(LPM_MODE lpm_mode);
#endif  /* LPM_H_ */
