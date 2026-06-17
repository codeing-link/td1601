

/******************************************************************************
 * @file     example_rtc.c
 * @brief    the main function for the RTC driver
 * @version  V1.0
 * @date     26. March 2020
 ******************************************************************************/
#include <stdio.h>
#include <soc.h>
#include <drv/rtc.h>
#include <csi_config.h>
#include <board_config.h>
#include <board_init.h>

#define RTC_TIME_OSC_RATIO                 (2U)

#define RTC_TIMEOUT0_SECS                  (5U)
#define RTC_TIMEOUT1_SECS                  (10U)
#define RTC_TIMEOUT2_SECS                  (15U)

#define RTC_TIME_CURRENT_YEAR              (120)
#define RTC_TIME_CURRENT_MON               (0)
#define RTC_TIME_CURRENT_DAY               (5)
#define RTC_TIME_CURRENT_HOUR              (23)
#define RTC_TIME_CURRENT_MIN               (59)
#define RTC_TIME_CURRENT_SEC               (55)

#define RTC_TIME_ALARM_YEAR                (120)
#define RTC_TIME_ALARM_MON                 (0)
#define RTC_TIME_ALARM_DAY                 (6)
#define RTC_TIME_ALARM_HOUR                (0)
#define RTC_TIME_ALARM_MIN                 (0)
#define RTC_TIME_ALARM_SEC                 (RTC_TIMEOUT1_SECS)


static volatile uint8_t cb_rtc_flag;
static csi_rtc_t g_rtc_handle;

extern void mdelay(uint32_t ms);


void rtc_event_cb_fun(csi_rtc_t *rtc_handle, void *arg)
{
    cb_rtc_flag = 1U;
    printf("enter rtc cb\n");
}

static int32_t test_rtc_really_time(csi_rtc_t *rtc_handle)
{
    csi_rtc_time_t last_time, initime;
    int32_t ret = 0;
    int32_t state;
    int32_t secs = 0;

    initime.tm_year    = RTC_TIME_CURRENT_YEAR;
    initime.tm_mon     = RTC_TIME_CURRENT_MON ;
    initime.tm_mday    = RTC_TIME_CURRENT_DAY ;
    initime.tm_hour    = RTC_TIME_CURRENT_HOUR;
    initime.tm_min     = RTC_TIME_CURRENT_MIN ;
    initime.tm_sec     = RTC_TIME_CURRENT_SEC ;

    state = csi_rtc_set_time(&g_rtc_handle, &initime);

    if (state < 0) {
        printf("csi_rtc_set_time error\n");
        ret = -1;
    }

    mdelay(RTC_TIMEOUT0_SECS * 1000U);

    state = csi_rtc_get_time(&g_rtc_handle, &last_time);

    if (state < 0) {
        printf("csi_rtc_get_time error\n");
        ret = -1;
    }

    if (initime.tm_sec != last_time.tm_sec) {
        secs += (last_time.tm_sec - initime.tm_sec);
    }

    if (initime.tm_min != last_time.tm_min) {
        secs += (last_time.tm_min - initime.tm_min) * 60;
    }

    if (initime.tm_hour != last_time.tm_hour) {
        secs += (last_time.tm_hour - initime.tm_hour) * 60 * 60;
    }

    if (initime.tm_mday != last_time.tm_mday) {
        secs += (last_time.tm_mday - initime.tm_mday) * 60 * 60 * 24;
    }

    if ((secs <= (RTC_TIMEOUT0_SECS + 5)) && (secs >= (RTC_TIMEOUT0_SECS - 5))) {
        printf("The time is %d-%d-%d %d:%d:%d\n", last_time.tm_year + 1900, last_time.tm_mon + 1,
               last_time.tm_mday, last_time.tm_hour, last_time.tm_min, last_time.tm_sec);
    } else {
        printf("get rtc timer error\n");
        ret = -1;
    }

    printf("----------------\n");
    return ret;
}

static int32_t test_rtc_alarm_time(csi_rtc_t *rtc_handle)
{
    int32_t ret = 0;
    int32_t state;
    csi_rtc_time_t alarm_time, last_time;

    alarm_time.tm_year    = RTC_TIME_ALARM_YEAR;
    alarm_time.tm_mon     = RTC_TIME_ALARM_MON ;
    alarm_time.tm_mday    = RTC_TIME_ALARM_DAY ;
    alarm_time.tm_hour    = RTC_TIME_ALARM_HOUR;
    alarm_time.tm_min     = RTC_TIME_ALARM_MIN ;
    alarm_time.tm_sec     = RTC_TIME_ALARM_SEC ;

    uint32_t secs = 0U;

    cb_rtc_flag = 0U;

    state = csi_rtc_set_alarm(&g_rtc_handle, &alarm_time, rtc_event_cb_fun, NULL);

    if (state < 0) {
        printf("csi_rtc_set_alarm error\n");
        ret = -1;
    }

    printf("test rtc alarm time %ds\n", RTC_TIMEOUT1_SECS);

    mdelay((RTC_TIMEOUT1_SECS * RTC_TIME_OSC_RATIO) * 1000U);

    if (cb_rtc_flag == 1U) {

        state = csi_rtc_get_time(&g_rtc_handle, &last_time);

        if (state < 0) {
            printf("csi_rtc_get_time error\n");
            ret = -1;
        }

        if (alarm_time.tm_sec != last_time.tm_sec) {
            secs += (last_time.tm_sec - alarm_time.tm_sec);
        }

        if (alarm_time.tm_min != last_time.tm_min) {
            secs += (last_time.tm_min - alarm_time.tm_min) * 60;
        }

        if (alarm_time.tm_hour != last_time.tm_hour) {
            secs += (last_time.tm_hour - alarm_time.tm_hour) * 60 * 60;
        }

        if (alarm_time.tm_mday != last_time.tm_mday) {
            secs += (last_time.tm_mday - alarm_time.tm_mday) * 60 * 60 * 24;
        }

        printf("The time is %d-%d-%d %d:%d:%d\n", last_time.tm_year + 1900, last_time.tm_mon + 1,
               last_time.tm_mday, last_time.tm_hour, last_time.tm_min, last_time.tm_sec);
        state = csi_rtc_cancel_alarm(rtc_handle);

        if (state < 0) {
            printf("csi_rtc_enable_alarm error\n");
            ret = -1;
        }

        printf("----------------\n");
    } else {
        ret = -1;
    }

    return ret;
}

static int32_t test_rtc(void)
{
    int32_t ret = 0;
    int32_t state;

    printf("--------------------------------\n");
    printf("start test_rtc\n");
    state = csi_rtc_init(&g_rtc_handle, 0U);

    if (state < 0) {
        printf("csi_rtc_init error\n");
        ret = -1;
    }

    state = test_rtc_really_time(&g_rtc_handle);

    if (state < 0) {
        printf("test_rtc_realy_time error\n");
        ret = -1;
    }

    state = test_rtc_alarm_time(&g_rtc_handle);

    if (state < 0) {
        printf("test_rtc_alarm_time error\n");
        ret = -1;
    }

    csi_rtc_uninit(&g_rtc_handle);

    return ret;
}
int example_rtc(void)
{
    int ret = 0;
    int32_t state;

    state = test_rtc();

    if (state < 0) {
        printf("test rtc failed\n");
        ret = -1;
    } else {
        printf("test rtc successfully\n");
    }
    CSI_EXAMPLE_RESULT(state);

    return ret;
}

int main(void)
{
    board_init();

    return example_rtc();
}
