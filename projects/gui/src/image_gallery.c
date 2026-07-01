/******************************************************************************
 * @file     image_gallery.c
 * @brief    LittleFS 图片库：扫描 JPG、显示当前图片、左右滑切换
 ******************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "image_gallery.h"
#include "jpeg_viewer.h"
#include "ST77916.h"
#include "lfs_port.h"
#include "littlefs/lfs.h"

#define IMAGE_GALLERY_MAX_PATH_LEN    66U

typedef struct {
    char paths[IMAGE_GALLERY_MAX_IMAGES][IMAGE_GALLERY_MAX_PATH_LEN];
    uint32_t count;
    uint32_t current;
} image_gallery_ctx_t;

static image_gallery_ctx_t s_gallery;

static void gallery_clear_screen(void)
{
    static uint16_t black_row[EXAMPLE_LCD_WIDTH];

    LCD_setWindow(0, 0, EXAMPLE_LCD_WIDTH - 1U, EXAMPLE_LCD_HEIGHT - 1U);
    for (uint16_t y = 0; y < EXAMPLE_LCD_HEIGHT; y++) {
        LCD_writeRowData(black_row, EXAMPLE_LCD_WIDTH);
    }
}

static int gallery_char_lower(int ch)
{
    if (ch >= 'A' && ch <= 'Z') {
        return ch + ('a' - 'A');
    }
    return ch;
}

static int gallery_is_jpg_name(const char *name)
{
    uint32_t len;

    if (name == NULL) {
        return 0;
    }

    len = (uint32_t)strlen(name);
    if (len < 5U) {
        return 0;
    }

    if (gallery_char_lower(name[len - 4U]) == '.' &&
        gallery_char_lower(name[len - 3U]) == 'j' &&
        gallery_char_lower(name[len - 2U]) == 'p' &&
        gallery_char_lower(name[len - 1U]) == 'g') {
        return 1;
    }

    if (len >= 6U &&
        gallery_char_lower(name[len - 5U]) == '.' &&
        gallery_char_lower(name[len - 4U]) == 'j' &&
        gallery_char_lower(name[len - 3U]) == 'p' &&
        gallery_char_lower(name[len - 2U]) == 'e' &&
        gallery_char_lower(name[len - 1U]) == 'g') {
        return 1;
    }

    return 0;
}

static int gallery_find_path(const char *path)
{
    for (uint32_t i = 0; i < s_gallery.count; i++) {
        if (strcmp(s_gallery.paths[i], path) == 0) {
            return (int)i;
        }
    }
    return -1;
}

static void gallery_insert_sorted(const char *path)
{
    uint32_t pos;

    if ((path == NULL) || (s_gallery.count >= IMAGE_GALLERY_MAX_IMAGES)) {
        return;
    }

    pos = s_gallery.count;
    while (pos > 0U && strcmp(path, s_gallery.paths[pos - 1U]) < 0) {
        memcpy(s_gallery.paths[pos], s_gallery.paths[pos - 1U],
               IMAGE_GALLERY_MAX_PATH_LEN);
        pos--;
    }

    strncpy(s_gallery.paths[pos], path, IMAGE_GALLERY_MAX_PATH_LEN - 1U);
    s_gallery.paths[pos][IMAGE_GALLERY_MAX_PATH_LEN - 1U] = '\0';
    s_gallery.count++;
}

static int gallery_load_saved_path(char *path, uint32_t path_size)
{
    lfs_t *lfs = lfs_port_get();
    lfs_file_t file;
    lfs_ssize_t n;

    if ((lfs == NULL) || (path == NULL) || (path_size == 0U)) {
        return -1;
    }

    int err = lfs_file_open(lfs, &file, IMAGE_GALLERY_CURRENT_PATH, LFS_O_RDONLY);
    if (err != LFS_ERR_OK) {
        return -1;
    }

    n = lfs_file_read(lfs, &file, path, path_size - 1U);
    lfs_file_close(lfs, &file);
    if (n <= 0) {
        return -1;
    }

    path[n] = '\0';
    for (int32_t i = 0; i < n; i++) {
        if (path[i] == '\r' || path[i] == '\n') {
            path[i] = '\0';
            break;
        }
    }

    return 0;
}

static void gallery_save_current_path(void)
{
    lfs_t *lfs = lfs_port_get();
    lfs_file_t file;
    const char *path;

    if ((lfs == NULL) || (s_gallery.count == 0U)) {
        return;
    }

    path = s_gallery.paths[s_gallery.current];
    int err = lfs_file_open(lfs, &file, IMAGE_GALLERY_CURRENT_PATH,
                            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
    if (err != LFS_ERR_OK) {
        return;
    }

    (void)lfs_file_write(lfs, &file, path, (lfs_size_t)strlen(path));
    lfs_file_close(lfs, &file);
}

static int gallery_display_current(void)
{
    int ret;

    if (s_gallery.count == 0U) {
        return -1;
    }

    ret = jpeg_viewer_show_file(s_gallery.paths[s_gallery.current]);
    if (ret == 0) {
        gallery_save_current_path();
    }

    return ret;
}

int image_gallery_refresh(void)
{
    lfs_t *lfs = lfs_port_get();
    lfs_dir_t dir;
    struct lfs_info info;
    char path[IMAGE_GALLERY_MAX_PATH_LEN];

    if (lfs == NULL) {
        return -1;
    }

    memset(&s_gallery, 0, sizeof(s_gallery));

    int err = lfs_dir_open(lfs, &dir, "/");
    if (err != LFS_ERR_OK) {
        return -1;
    }

    while (lfs_dir_read(lfs, &dir, &info) > 0) {
        if (info.type != LFS_TYPE_REG) {
            continue;
        }

        if (!gallery_is_jpg_name(info.name)) {
            continue;
        }

        if ((strlen(info.name) + 1U) >= sizeof(path)) {
            continue;
        }

        path[0] = '/';
        strcpy(&path[1], info.name);
        gallery_insert_sorted(path);
    }

    lfs_dir_close(lfs, &dir);
    return 0;
}

int image_gallery_init(void)
{
    return image_gallery_refresh();
}

int image_gallery_show_path(const char *path)
{
    int index;

    if ((path == NULL) || (image_gallery_refresh() != 0)) {
        return -1;
    }

    index = gallery_find_path(path);
    if (index < 0) {
        return -1;
    }

    s_gallery.current = (uint32_t)index;
    return gallery_display_current();
}

int image_gallery_show_saved_or_first(void)
{
    char saved[IMAGE_GALLERY_MAX_PATH_LEN];
    int index = -1;

    if (image_gallery_refresh() != 0) {
        return -1;
    }

    if (s_gallery.count == 0U) {
        printf("[gallery] no jpg files\r\n");
        gallery_clear_screen();
        return 0;
    }

    if (gallery_load_saved_path(saved, sizeof(saved)) == 0) {
        index = gallery_find_path(saved);
    }

    s_gallery.current = (index >= 0) ? (uint32_t)index : 0U;
    return gallery_display_current();
}

int image_gallery_next(void)
{
    uint32_t old_index = s_gallery.current;

    if (s_gallery.count <= 1U) {
        return 0;
    }

    s_gallery.current = (s_gallery.current + 1U) % s_gallery.count;
    if (gallery_display_current() != 0) {
        s_gallery.current = old_index;
        return -1;
    }

    return 0;
}

int image_gallery_prev(void)
{
    uint32_t old_index = s_gallery.current;

    if (s_gallery.count <= 1U) {
        return 0;
    }

    if (s_gallery.current == 0U) {
        s_gallery.current = s_gallery.count - 1U;
    } else {
        s_gallery.current--;
    }

    if (gallery_display_current() != 0) {
        s_gallery.current = old_index;
        return -1;
    }

    return 0;
}

void image_gallery_handle_touch(const cst816_data_t *touch)
{
    if (touch == NULL) {
        return;
    }

    if (touch->gesture == CST816_GESTURE_LEFT) {
        (void)image_gallery_next();
    } else if (touch->gesture == CST816_GESTURE_RIGHT) {
        (void)image_gallery_prev();
    }
}

uint32_t image_gallery_count(void)
{
    return s_gallery.count;
}

const char *image_gallery_current_path(void)
{
    if (s_gallery.count == 0U) {
        return NULL;
    }

    return s_gallery.paths[s_gallery.current];
}
