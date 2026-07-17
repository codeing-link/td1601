#ifndef GPIO_TOGGLE_H
#define GPIO_TOGGLE_H

#ifdef __cplusplus
extern "C" {
#endif

/* 初始化 PA24 为低电平 GPIO 输出。 */
int gpio_toggle_init(void);

/* 非阻塞轮询函数；每隔 1000 ms 翻转一次 PA24。 */
void gpio_toggle(void);

#ifdef __cplusplus
}
#endif

#endif
