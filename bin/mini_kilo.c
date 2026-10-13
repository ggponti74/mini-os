#include "mini_kilo.h"
#include "display.h"
#include "fs/vfs.h"
#include "memory.h"
#include "string.h"

#define EDITOR_TAB_STOP 4

typedef struct erow {
    int size;
    char *chars;
} erow;

static struct {
    int cx, cy;
    int rowoff;
    int numrows;
    erow *row;
    const char *filename;
} E;

static void editor_insert_row(int at, const char *s, int len) {
    if (at > E.numrows) return;

    E.row = kmalloc(sizeof(erow) * (E.numrows + 1));
    for (int i = 0; i < at; i++) E.row[i] = E.row[i];
    for (int i = E.numrows; i > at; i--) E.row[i] = E.row[i - 1];

    E.row[at].size = len;
    E.row[at].chars = kmalloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';

    E.numrows++;
}

static void editor_open(const char *filename) {
    E.filename = filename;
    E.cx = E.cy = E.rowoff = 0;
    E.numrows = 0;

    vfs_node_t *file = vfs_lookup(fs_root, filename);
    if (!file) return;

    char *buf = kmalloc(file->length + 1);
    vfs_read(file, 0, file->length, (uint8_t *)buf);
    buf[file->length] = '\0';

    char *start = buf;
    for (uint32_t i = 0; i < file->length; i++) {
        if (buf[i] == '\n') {
            editor_insert_row(E.numrows, start, buf[i] - start);
            start = buf + i + 1;
        }
    }

    if (start < buf + file->length)
        editor_insert_row(E.numrows, start, (buf + file->length) - start);

    kfree(buf);
}

static void editor_draw_rows(void) {
    clear_screen();

    for (int y = 0; y < 25; y++) {
        int filerow = E.rowoff + y;
        if (filerow >= E.numrows) {
            kprint_color("~\n", COLOR_DEFAULT);
        } else {
            kprint_color(E.row[filerow].chars, COLOR_DEFAULT);
            kputchar_color('\n', COLOR_DEFAULT);
        }
    }
}

void editor_refresh_screen(void) {
    editor_draw_rows();

    // Move cursor
    display_move_cursor(E.cx, E.cy);
}

static void editor_insert_char(char c) {
    if (E.cy >= E.numrows) return;

    erow *row = &E.row[E.cy];

    row->chars = krealloc(row->chars, row->size + 2);
    memmove(&row->chars[E.cx + 1], &row->chars[E.cx], row->size - E.cx + 1);

    row->chars[E.cx] = c;
    row->size++;
    E.cx++;
}

static void editor_process_key(int key) {
    switch (key) {
        case KEY_LEFT:
            if (E.cx > 0) E.cx--;
            break;

        case KEY_RIGHT:
            if (E.cy < E.numrows && E.cx < E.row[E.cy].size)
                E.cx++;
            break;

        case KEY_UP:
            if (E.cy > 0) E.cy--;
            break;

        case KEY_DOWN:
            if (E.cy < E.numrows - 1) E.cy++;
            break;

        default:
            if (key >= 32 && key <= 126)
                editor_insert_char((char)key);
            break;
    }
}
