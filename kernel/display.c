// kernel/display.c
#include <stdint.h>
#include "display.h"
#include "rtc.h"
#include "shell.h"

#define MAIN_SCREEN_HEIGHT 24  // Reserve Row 24 for the Status Line

// Color attributes
#define COLOR_STATUSBAR 0x1F // Bright white text on blue background

static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;

void clear_screen(void) {
    uint16_t blank = ' ' | (COLOR_DEFAULT << 8);
    // Clear rows 0 to 23
    for (int i = 0; i < VGA_WIDTH * MAIN_SCREEN_HEIGHT; i++) {
        VGA_MEMORY[i] = blank;
    }
    cursor_x = 0;
    cursor_y = 0;
}

void kputchar_color(const char c, uint8_t color) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            int index = cursor_y * VGA_WIDTH + cursor_x;
            VGA_MEMORY[index] = (uint16_t)' ' | ((uint16_t)color << 8);
        }
    } else {
        int index = cursor_y * VGA_WIDTH + cursor_x;
        VGA_MEMORY[index] = (uint16_t)c | ((uint16_t)color << 8);
        cursor_x++;
        if (cursor_x >= VGA_WIDTH) {
            cursor_x = 0;
            cursor_y++;
        }
    }

    // Scroll/reset before reaching the status bar row
    if (cursor_y >= MAIN_SCREEN_HEIGHT) {
        clear_screen();
    }
}

void kputchar(char c) {
    kputchar_color(c, COLOR_DEFAULT);
}

void kprint_color(const char *str, uint8_t color) {
    for (int i = 0; str[i] != '\0'; i++) {
        kputchar_color(str[i], color);
    }
}

void update_status_bar(void) {
    rtc_time_t rtc;
    rtc_read_datetime(&rtc);

    int tz = shell_get_timezone();
    if (tz != 0) {
        uint32_t epoch = rtc_to_epoch(&rtc);
        int32_t local_epoch = (int32_t)epoch + (tz * 3600);
        if (local_epoch >= 0) {
            epoch_to_rtc((uint32_t)local_epoch, &rtc);
        }
    }

    char dt[20];
    uint32_t year = rtc.year;
    dt[0] = '0' + ((year / 1000) % 10);
    dt[1] = '0' + ((year / 100) % 10);
    dt[2] = '0' + ((year / 10) % 10);
    dt[3] = '0' + (year % 10);
    dt[4] = '-';
    dt[5] = '0' + (rtc.month / 10);
    dt[6] = '0' + (rtc.month % 10);
    dt[7] = '-';
    dt[8] = '0' + (rtc.day / 10);
    dt[9] = '0' + (rtc.day % 10);
    dt[10] = ' ';
    dt[11] = '0' + (rtc.hour / 10);
    dt[12] = '0' + (rtc.hour % 10);
    dt[13] = ':';
    dt[14] = '0' + (rtc.minute / 10);
    dt[15] = '0' + (rtc.minute % 10);
    dt[16] = ':';
    dt[17] = '0' + (rtc.second / 10);
    dt[18] = '0' + (rtc.second % 10);
    dt[19] = '\0';

    draw_status_bar(dt, 1);
}

// Helper to write formatted text to a specific VGA line
void draw_status_bar(const char* datetime_str, int sound_enabled) {
    uint16_t *status_row = (uint16_t*)(0xB8000 + (24 * 80 * 2));
    
    // 1. Fill entire row 24 with background color
    uint16_t blank = ' ' | (COLOR_STATUSBAR << 8);
    for (int col = 0; col < VGA_WIDTH; col++) {
        status_row[col] = blank;
    }

    // 2. Sound Symbol: ASCII 14 ('♪') or 'S' when ON, 'M' or 'x' when MUTED/OFF
    char sound_icon = sound_enabled ? 14 : 'x'; 

    // Construct format: " [System Ready]                     2026-10-04 10:58 | Sound: [♪] "
    // Left-hand side label
    const char *label = " mini-os";
    for (int i = 0; label[i] != '\0'; i++) {
        status_row[i] = (uint16_t)label[i] | (COLOR_STATUSBAR << 8);
    }

    // Right-hand side status: Date, Time, Sound Indicator
    // Calculate start position to right-align
    // Example string length: "YYYY-MM-DD HH:MM:SS | Sound: [♪]" (~32 chars)
    int pos = 50; 
    
    // Render Date and Time String
    for (int i = 0; datetime_str[i] != '\0' && pos < 72; i++, pos++) {
        status_row[pos] = (uint16_t)datetime_str[i] | (COLOR_STATUSBAR << 8);
    }

    // Divider
    status_row[pos++] = ' ' | (COLOR_STATUSBAR << 8);
    status_row[pos++] = '|' | (COLOR_STATUSBAR << 8);
    status_row[pos++] = ' ' | (COLOR_STATUSBAR << 8);

    // Render Sound Indicator
    const char *snd_prefix = "SND:[";
    for (int i = 0; snd_prefix[i] != '\0'; i++, pos++) {
        status_row[pos] = (uint16_t)snd_prefix[i] | (COLOR_STATUSBAR << 8);
    }
    
    // Icon character
    status_row[pos++] = (uint16_t)sound_icon | (COLOR_STATUSBAR << 8);
    status_row[pos++] = ']' | (COLOR_STATUSBAR << 8);
}