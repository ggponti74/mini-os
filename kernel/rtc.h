#ifndef RTC_H
#define RTC_H

#include <stdint.h>
#include "io.h"

#define CMOS_ADDRESS     0x70
#define CMOS_DATA        0x71

#define RTC_REG_SECONDS  0x00
#define RTC_REG_MINUTES  0x02
#define RTC_REG_HOURS    0x04
#define RTC_REG_DAY      0x07
#define RTC_REG_MONTH    0x08
#define RTC_REG_YEAR     0x09
#define RTC_REG_CENTURY  0x32
#define RTC_STATUS_A     0x0A
#define RTC_STATUS_B     0x0B

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint32_t year;
} rtc_time_t;

static inline int rtc_is_updating(void) {
    outb(CMOS_ADDRESS, RTC_STATUS_A);
    return (inb(CMOS_DATA) & 0x80);
}

static inline uint8_t rtc_read_register(uint8_t reg) {
    outb(CMOS_ADDRESS, reg);
    return inb(CMOS_DATA);
}

static inline void rtc_read_datetime(rtc_time_t *time) {
    rtc_time_t last;

    // Wait until RTC update cycle is complete
    int timeout = 100000;
    while (rtc_is_updating() && --timeout > 0);

    time->second = rtc_read_register(RTC_REG_SECONDS);
    time->minute = rtc_read_register(RTC_REG_MINUTES);
    time->hour   = rtc_read_register(RTC_REG_HOURS);
    time->day    = rtc_read_register(RTC_REG_DAY);
    time->month  = rtc_read_register(RTC_REG_MONTH);
    time->year   = rtc_read_register(RTC_REG_YEAR);

    // Read until two consecutive readings match to guard against rollovers mid-read
    do {
        last = *time;
        timeout = 100000;
        while (rtc_is_updating() && --timeout > 0);

        time->second = rtc_read_register(RTC_REG_SECONDS);
        time->minute = rtc_read_register(RTC_REG_MINUTES);
        time->hour   = rtc_read_register(RTC_REG_HOURS);
        time->day    = rtc_read_register(RTC_REG_DAY);
        time->month  = rtc_read_register(RTC_REG_MONTH);
        time->year   = rtc_read_register(RTC_REG_YEAR);
    } while ((last.second != time->second) || (last.minute != time->minute) ||
             (last.hour   != time->hour)   || (last.day    != time->day)    ||
             (last.month  != time->month)  || (last.year   != time->year));

    uint8_t reg_b = rtc_read_register(RTC_STATUS_B);

    // Convert BCD to binary if Bit 2 of Status Register B is clear
    if (!(reg_b & 0x04)) {
        time->second = ((time->second / 16) * 10) + (time->second & 0x0F);
        time->minute = ((time->minute / 16) * 10) + (time->minute & 0x0F);
        time->hour   = (((time->hour & 0x70) / 16) * 10) + (time->hour & 0x0F) | (time->hour & 0x80);
        time->day    = ((time->day / 16) * 10) + (time->day & 0x0F);
        time->month  = ((time->month / 16) * 10) + (time->month & 0x0F);
        time->year   = ((time->year / 16) * 10) + (time->year & 0x0F);
    }

    // Convert 12-hour clock format to 24-hour clock format if necessary
    if (!(reg_b & 0x02)) {
        if (time->hour & 0x80) {
            time->hour = ((time->hour & 0x7F) % 12) + 12;
        } else {
            time->hour = time->hour % 12;
        }
    }

    time->year += 2000;
}

static inline uint32_t rtc_to_epoch(const rtc_time_t *t) {
    uint32_t year = t->year;
    uint32_t month = t->month;
    uint32_t day = t->day;

    static const uint16_t days_before_month[13] = {
        0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };

    uint32_t leap_days = 0;
    for (uint32_t y = 1970; y < year; y++) {
        if ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)) {
            leap_days++;
        }
    }

    uint32_t days = (year - 1970) * 365 + leap_days + days_before_month[month] + (day - 1);
    if (month > 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) {
        days++;
    }

    return days * 86400 + (uint32_t)t->hour * 3600 + (uint32_t)t->minute * 60 + t->second;
}

static inline void epoch_to_rtc(uint32_t epoch, rtc_time_t *t) {
    t->second = epoch % 60;
    epoch /= 60;
    t->minute = epoch % 60;
    epoch /= 60;
    t->hour = epoch % 24;
    epoch /= 24;

    uint32_t days = epoch;
    uint32_t year = 1970;
    while (1) {
        int leap = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
        uint32_t days_in_year = leap ? 366 : 365;
        if (days < days_in_year) {
            break;
        }
        days -= days_in_year;
        year++;
    }
    t->year = year;

    int leap = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
    static const uint8_t days_in_month[12] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    t->month = 12;
    for (int m = 0; m < 12; m++) {
        uint8_t dim = (m == 1 && leap) ? 29 : days_in_month[m];
        if (days < dim) {
            t->month = m + 1;
            break;
        }
        days -= dim;
    }
    t->day = days + 1;
}

#endif // RTC_H