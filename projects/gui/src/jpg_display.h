#ifndef _JPG_DISPLAY_H_
#define _JPG_DISPLAY_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  完整流程：挂载 LFS → 按需写入 jpg → 流式解码 → 全屏显示
 *
 * 策略：
 *   - 首次上电（LFS 无有效文件系统）：格式化 + 写入
 *   - 文件已存在且大小匹配：直接读取解码，不重写
 *   - 文件不存在或大小不符：删除旧文件 + 重新写入
 */
void jpg_display_run(void);

#ifdef __cplusplus
}
#endif

#endif /* _JPG_DISPLAY_H_ */
