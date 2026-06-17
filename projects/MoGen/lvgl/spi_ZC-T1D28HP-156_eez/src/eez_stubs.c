#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <drv/tick.h>

clock_t clock(void) { return (clock_t)csi_tick_get_ms(); }
double difftime(time_t t1, time_t t0) { return (double)(t1 - t0); }
time_t time(time_t *t) { time_t v = (time_t)(csi_tick_get_ms() / 1000); if (t) *t = v; return v; }
char *asctime(const struct tm *tp) { (void)tp; return "Thu Jan  1 00:00:00 1970\n"; }
char *ctime(const time_t *t) { (void)t; return "Thu Jan  1 00:00:00 1970\n"; }
struct tm *gmtime(const time_t *t) { static struct tm z; (void)t; return &z; }
struct tm *localtime(const time_t *t) { static struct tm z; (void)t; return &z; }
size_t strftime(char *s, size_t max, const char *fmt, const struct tm *tp) { (void)s; (void)max; (void)fmt; (void)tp; return 0; }
time_t mktime(struct tm *tp) { (void)tp; return 0; }

uint32_t osKernelGetTickCount(void) {
    return (uint32_t)csi_tick_get_ms();
}

uint32_t millis(void) {
    return (uint32_t)csi_tick_get_ms();
}

void __aeabi_assert(const char *expr, const char *file, int line) {
    (void)expr; (void)file; (void)line;
    while (1);
}

void *__dso_handle = 0;

int __aeabi_atexit(void *object, void (*destructor)(void *), void *dso_handle) {
    (void)object; (void)destructor; (void)dso_handle;
    return 0;
}

void _atexit_init(void) {}
void _atexit_mutex(void) {}
