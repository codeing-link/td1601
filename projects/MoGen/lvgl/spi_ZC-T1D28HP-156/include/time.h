#ifndef _TIME_H_
#define _TIME_H_

#include <stdint.h>
#include <stddef.h>

#define CLK_TCK           (1000000)
#define CLOCKS_PER_SEC    (1000000)

#define NSEC_PER_SEC        1000000000
#define USEC_PER_SEC        1000000
#define NSEC_PER_USEC       1000
#define USEC_PER_MSEC       1000
#define MSEC_PER_SEC        1000
#define NSEC_PER_MSEC       1000000
#define TICK2MSEC(tick)     ((tick)* (1000 / CLOCKS_PER_SEC))

#define CLOCK_REALTIME      (clockid_t)0
#define CLOCK_MONOTONIC     (clockid_t)1
#define TIMER_ABSTIME       (clockid_t)1

#define localtime_r(c,r)   gmtime_r(c,r)

#if !defined(__time_t_defined) && !defined(_TIME_T_DECLARED)
typedef long  time_t;
#define __time_t_defined
#define _TIME_T_DECLARED
#endif

#if !defined(__clock_t_defined) && !defined(_CLOCK_T_DECLARED)
typedef unsigned long clock_t;
#define __clock_t_defined
#define _CLOCK_T_DECLARED
#endif

#if !defined(__clockid_t_defined) && !defined(_CLOCKID_T_DECLARED)
typedef uint8_t   clockid_t;
#define __clockid_t_defined
#define _CLOCKID_T_DECLARED
#endif

#ifndef _SYS__TIMESPEC_H_
#define _SYS__TIMESPEC_H_
struct timespec {
    time_t tv_sec;
    long   tv_nsec;
};
#endif

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

#ifdef __cplusplus
extern "C" {
#endif

clock_t clock(void);
double difftime(time_t t1, time_t t0);
time_t time(time_t *t);
char *asctime(const struct tm *tp);
char *ctime(const time_t *t);
struct tm *gmtime(const time_t *t);
struct tm *localtime(const time_t *t);
size_t strftime(char *s, size_t max, const char *fmt, const struct tm *tp);
time_t mktime(struct tm *tp);
struct tm *gmtime_r(const time_t *timep, struct tm *result);

int clock_timer_init(void);
int clock_timer_uninit(void);
int clock_timer_start(void);
int clock_timer_stop(void);
int clock_gettime(clockid_t clockid, struct timespec *tp);

#ifdef __cplusplus
}
#endif

#endif /* _TIME_H_ */
