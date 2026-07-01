/******************************************************************************
 * @file     lfs_test.c
 * @brief    LittleFS 测试程序
 *
 *  测试流程：
 *   1. 挂载 LittleFS（首次使用自动格式化）
 *   2. 在内存中生成 360×360 RGB565 彩条图片（8 色竖条）
 *   3. 分块写入 LittleFS 文件 "/img/rainbow.bin"
 *   4. 从 LittleFS 读回，逐行调用 LCD_addWindow 显示到屏幕
 *   5. 打印测试结果
 *
 *  分辨率：360×360，色彩：RGB565（2字节/像素），共 259200 字节
 *  由于 RAM 有限（64KB SRAM），采用逐行（360×2=720字节）读写，不一次性缓冲整帧
 *
 * @version  V1.0
 * @date     2026-06-24
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "lfs_port.h"
#include "ST77916.h"

/* ---- 图片参数 ---------------------------------------------------------- */
/* 存储尺寸：180×90，RGB565，共 32400 字节，远小于 128KB 分区 */
#define IMG_WIDTH       180
#define IMG_HEIGHT      90
#define IMG_BPP         2                    /* RGB565：2 bytes/pixel */
#define IMG_ROW_BYTES   (IMG_WIDTH * IMG_BPP)/* 360 bytes/行 */

/* 在 360×360 屏幕上居中显示：左上角 (90, 135) */
#define DISP_X0         ((EXAMPLE_LCD_WIDTH  - IMG_WIDTH)  / 2)   /* 90 */
#define DISP_Y0         ((EXAMPLE_LCD_HEIGHT - IMG_HEIGHT) / 2)   /* 135 */

/* 文件路径 */
#define LFS_DIR_NAME    "/img"
#define LFS_FILE_NAME   "/img/rainbow.bin"

/* ---- 8 色彩条（RGB565，大端字节序以匹配 ST77916 传输） ---------------- */
/* RGB565 大端：[R4:R3:R2:R1:R0:G5:G4:G3] [G2:G1:G0:B4:B3:B2:B1:B0] */
#define RGB565(r, g, b) \
    ((uint16_t)(((r) & 0x1F) << 11) | (((g) & 0x3F) << 5) | ((b) & 0x1F))

/* 8 种颜色：红、橙、黄、绿、青、蓝、紫、白 */
static const uint16_t s_bar_colors[8] = {
    RGB565(31,  0,  0),   /* 红 */
    RGB565(31, 20,  0),   /* 橙 */
    RGB565(31, 63,  0),   /* 黄 */
    RGB565( 0, 63,  0),   /* 绿 */
    RGB565( 0, 63, 31),   /* 青 */
    RGB565( 0,  0, 31),   /* 蓝 */
    RGB565(15,  0, 31),   /* 紫 */
    RGB565(31, 63, 31),   /* 白 */
};

/* 每条彩条高度（90 / 8 = 11 像素，最后一条补余数） */
#define BAR_HEIGHT_PX   (IMG_HEIGHT / 8)

/* ---- 行缓冲（720字节），在 BSS 段，不占堆栈 --------------------------- */
static uint16_t s_row_buf[IMG_WIDTH];

/* =========================================================================
 * 内部辅助：生成第 row 行的 RGB565 横条数据到 s_row_buf
 *   横条：颜色由行号决定，整行同色
 * ====================================================================== */
static void gen_bar_row(int row)
{
    int bar_idx = row / BAR_HEIGHT_PX;
    if (bar_idx >= 8) bar_idx = 7;
    uint16_t color = s_bar_colors[bar_idx];
    for (int x = 0; x < IMG_WIDTH; x++) {
        s_row_buf[x] = color;
    }
}

/* =========================================================================
 * lfs_test_write_image()
 *   生成彩条图片并写入 LittleFS，返回 0 成功，-1 失败
 * ====================================================================== */
static int lfs_test_write_image(void)
{
    lfs_t *lfs = lfs_port_get();
    if (!lfs) {
        printf("[lfs_test] LittleFS not mounted\r\n");
        return -1;
    }

    /* 创建目录（已存在时忽略错误） */
    int err = lfs_mkdir(lfs, LFS_DIR_NAME);
    if (err != LFS_ERR_OK && err != LFS_ERR_EXIST) {
        printf("[lfs_test] mkdir '%s' failed (%d)\r\n", LFS_DIR_NAME, err);
        return -1;
    }

    /* 若文件已存在，先删除以回收空间，避免 No more free space */
    struct lfs_info fi;
    if (lfs_stat(lfs, LFS_FILE_NAME, &fi) == LFS_ERR_OK) {
        printf("[lfs_test] removing old '%s' (%u bytes)...\r\n",
               LFS_FILE_NAME, (unsigned)fi.size);
        lfs_remove(lfs, LFS_FILE_NAME);
    }

    /* 打开/创建文件 */
    lfs_file_t file;
    err = lfs_file_open(lfs, &file, LFS_FILE_NAME,
                        LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
    if (err != LFS_ERR_OK) {
        printf("[lfs_test] open '%s' failed (%d)\r\n", LFS_FILE_NAME, err);
        return -1;
    }

    printf("[lfs_test] Writing %dx%d rainbow bars to '%s'...\r\n",
           IMG_WIDTH, IMG_HEIGHT, LFS_FILE_NAME);

    /* 逐行写入，每次 360 字节 */
    for (int row = 0; row < IMG_HEIGHT; row++) {
        gen_bar_row(row);
        lfs_ssize_t written = lfs_file_write(lfs, &file,
                                              s_row_buf, IMG_ROW_BYTES);
        if (written != (lfs_ssize_t)IMG_ROW_BYTES) {
            printf("[lfs_test] write row %d failed (%d)\r\n",
                   row, (int)written);
            lfs_file_close(lfs, &file);
            return -1;
        }
        /* 每 16 行打印一次进度 */
        if ((row % 16) == 0 || row == IMG_HEIGHT - 1) {
            printf("[lfs_test]   row %d/%d written\r\n", row, IMG_HEIGHT);
        }
    }

    lfs_file_close(lfs, &file);

    /* 验证文件大小 */
    struct lfs_info info;
    lfs_stat(lfs, LFS_FILE_NAME, &info);
    printf("[lfs_test] Write done. File size: %u bytes (expected %u)\r\n",
           (unsigned)info.size,
           (unsigned)(IMG_WIDTH * IMG_HEIGHT * IMG_BPP));

    return (info.size == (lfs_size_t)(IMG_WIDTH * IMG_HEIGHT * IMG_BPP)) ? 0 : -1;
}

/* =========================================================================
 * lfs_test_read_and_display()
 *   从 LittleFS 读取彩条图片并通过 ST77916 显示，返回 0 成功，-1 失败
 * ====================================================================== */
static int lfs_test_read_and_display(void)
{
    lfs_t *lfs = lfs_port_get();
    if (!lfs) {
        printf("[lfs_test] LittleFS not mounted\r\n");
        return -1;
    }

    lfs_file_t file;
    int err = lfs_file_open(lfs, &file, LFS_FILE_NAME, LFS_O_RDONLY);
    if (err != LFS_ERR_OK) {
        printf("[lfs_test] open '%s' for read failed (%d)\r\n",
               LFS_FILE_NAME, err);
        return -1;
    }

    printf("[lfs_test] Reading and displaying '%s' at (%d,%d)...\r\n",
           LFS_FILE_NAME, DISP_X0, DISP_Y0);

    /* 逐行读取并显示：每行设 CASET+RASET（y 递增）后发 RAMWR+像素数据 */
    for (int row = 0; row < IMG_HEIGHT; row++) {
        lfs_ssize_t n = lfs_file_read(lfs, &file, s_row_buf, IMG_ROW_BYTES);
        if (n != (lfs_ssize_t)IMG_ROW_BYTES) {
            printf("[lfs_test] read row %d failed (%d)\r\n", row, (int)n);
            lfs_file_close(lfs, &file);
            return -1;
        }
        uint16_t y = (uint16_t)(DISP_Y0 + row);
        LCD_addWindow((uint16_t)DISP_X0, y,
                      (uint16_t)(DISP_X0 + IMG_WIDTH - 1), y,
                      s_row_buf);
    }

    lfs_file_close(lfs, &file);
    printf("[lfs_test] Display done.\r\n");
    return 0;
}

/* =========================================================================
 * lfs_test_run()  — 对外入口，main() 调用
 * ====================================================================== */
void lfs_test_run(void)
{
    printf("\r\n======== LittleFS Test ========\r\n");

    /* 1. 挂载文件系统 */
    if (lfs_port_mount() != 0) {
        printf("[lfs_test] FAIL: mount\r\n");
        return;
    }

    /* 2. 打印分区信息 */
    lfs_t *lfs = lfs_port_get();
    lfs_ssize_t used  = lfs_fs_size(lfs);
    printf("[lfs_test] Used blocks: %d / %d  (~%uKB free)\r\n",
           (int)used,
           LFS_BLOCK_COUNT,
           (unsigned)((LFS_BLOCK_COUNT - (int)used) * LFS_SECTOR_SIZE / 1024));

    /* 3. 写入彩条图片 */
    printf("\r\n--- Step 1: Write rainbow image ---\r\n");
    if (lfs_test_write_image() != 0) {
        printf("[lfs_test] FAIL: write image\r\n");
        goto done;
    }
    printf("[lfs_test] PASS: write image\r\n");

    /* 4. 读取并显示 */
    printf("\r\n--- Step 2: Read & Display ---\r\n");
    if (lfs_test_read_and_display() != 0) {
        printf("[lfs_test] FAIL: read & display\r\n");
        goto done;
    }
    printf("[lfs_test] PASS: read & display\r\n");

    /* 5. 验证：重新读一行，与现场生成对比 */
    printf("\r\n--- Step 3: Data verify ---\r\n");
    {
        lfs_file_t file;
        int err = lfs_file_open(lfs, &file, LFS_FILE_NAME, LFS_O_RDONLY);
        if (err == LFS_ERR_OK) {
            static uint16_t ref_row[IMG_WIDTH];
            lfs_ssize_t n = lfs_file_read(lfs, &file, s_row_buf, IMG_ROW_BYTES);
            lfs_file_close(lfs, &file);
            gen_bar_row(0);
            memcpy(ref_row, s_row_buf, IMG_ROW_BYTES);

            /* 重新读文件首行与参考行对比 */
            err = lfs_file_open(lfs, &file, LFS_FILE_NAME, LFS_O_RDONLY);
            if (err == LFS_ERR_OK) {
                lfs_file_read(lfs, &file, s_row_buf, IMG_ROW_BYTES);
                lfs_file_close(lfs, &file);
                (void)n;
                if (memcmp(s_row_buf, ref_row, IMG_ROW_BYTES) == 0) {
                    printf("[lfs_test] PASS: data verify\r\n");
                } else {
                    printf("[lfs_test] FAIL: data mismatch\r\n");
                }
            }
        }
    }

done:
    lfs_port_unmount();
    printf("======== LittleFS Test Done ========\r\n\r\n");
}
