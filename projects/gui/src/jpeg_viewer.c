/******************************************************************************
 * @file     jpeg_viewer.c
 * @brief    从 LittleFS 文件流式 JPEG 解码并显示到 ST77916 LCD
 ******************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include "jpeg_viewer.h"
#include "lfs_port.h"
#include "ST77916.h"
#include "TJpgDec/tjpgd.h"

#define JPEG_VIEWER_POOL_SIZE   3500U

static uint8_t s_jpeg_pool[JPEG_VIEWER_POOL_SIZE];

typedef struct {
    lfs_file_t *file;
} jpeg_viewer_io_t;

static size_t jpeg_viewer_input(JDEC *jd, uint8_t *buf, size_t ndata)
{
    jpeg_viewer_io_t *io = (jpeg_viewer_io_t *)jd->device;

    if (buf != NULL) {
        lfs_ssize_t n = lfs_file_read(lfs_port_get(), io->file, buf,
                                      (lfs_size_t)ndata);
        return (n > 0) ? (size_t)n : 0U;
    }

    lfs_file_seek(lfs_port_get(), io->file, (lfs_soff_t)ndata, LFS_SEEK_CUR);
    return ndata;
}

static int jpeg_viewer_output(JDEC *jd, void *bitmap, JRECT *rect)
{
    (void)jd;
    LCD_addWindow(rect->left, rect->top, rect->right, rect->bottom,
                  (uint16_t *)bitmap);
    return 1;
}

int jpeg_viewer_show_file(const char *path)
{
    lfs_t *lfs = lfs_port_get();
    lfs_file_t file;
    JDEC jdec;
    jpeg_viewer_io_t io;

    if ((lfs == NULL) || (path == NULL)) {
        return -1;
    }

    int err = lfs_file_open(lfs, &file, path, LFS_O_RDONLY);
    if (err != LFS_ERR_OK) {
        printf("[jpeg_viewer] open '%s' failed (%d)\r\n", path, err);
        return -1;
    }

    io.file = &file;
    JRESULT jres = jd_prepare(&jdec, jpeg_viewer_input, s_jpeg_pool,
                              JPEG_VIEWER_POOL_SIZE, &io);
    if (jres != JDR_OK) {
        printf("[jpeg_viewer] jd_prepare failed (%d)\r\n", (int)jres);
        lfs_file_close(lfs, &file);
        return -1;
    }

    printf("[jpeg_viewer] image: %ux%u\r\n",
           (unsigned)jdec.width, (unsigned)jdec.height);

    jres = jd_decomp(&jdec, jpeg_viewer_output, 0);
    if (jres != JDR_OK) {
        printf("[jpeg_viewer] jd_decomp failed (%d)\r\n", (int)jres);
        lfs_file_close(lfs, &file);
        return -1;
    }

    lfs_file_close(lfs, &file);
    printf("[jpeg_viewer] display done\r\n");
    return 0;
}
