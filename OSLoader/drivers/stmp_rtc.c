


#include "regsrtc.h"

#include "rtc_up.h"


void portRTC_init()
{
    while(HW_RTC_STAT.B.NEW_REGS);

    HW_RTC_PERSISTENT0.B.XTAL32KHZ_PWRUP = 1;

    while(HW_RTC_STAT.B.NEW_REGS);

    HW_RTC_PERSISTENT0.B.CLOCKSOURCE = 1;

    while(HW_RTC_STAT.B.NEW_REGS);

    HW_RTC_PERSISTENT0.B.AUTO_RESTART = 0;

    while(HW_RTC_STAT.B.NEW_REGS);

    HW_RTC_PERSISTENT0.B.DISABLE_PSWITCH = 1;

    while(HW_RTC_STAT.B.NEW_REGS);



}

// 2026-09-11：本机 RTC 秒计数实测为 10Hz（日志对照：RTC 每分钟 COUNT+6000@1kHz tick，
// 即每秒 +10；上游 125 同此行为，手册无分频寄存器可调）——软件 /10 适配为真实秒
uint32_t rtc_get_seconds()
{
    return HW_RTC_SECONDS.B.COUNT / 10;
}

void rtc_set_seconds(uint32_t s)
{
    
    while(HW_RTC_STAT.B.NEW_REGS);
    HW_RTC_SECONDS_WR(s * 10);
    while(HW_RTC_STAT.B.NEW_REGS);
}