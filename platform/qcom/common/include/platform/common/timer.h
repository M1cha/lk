#ifndef __PLATFORM_COMMON_TIMER_H_
#define __PLATFORM_COMMON_TIMER_H_

void qcom_timer_early_init(addr_t control_base);
void qcom_timer_init(uint32_t freq);

#endif
