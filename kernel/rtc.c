#include "rtc.h"

static int rtc_is_leap_year(uint32_t year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

int rtc_get_epoch(uint32_t *epoch) {
    if (!epoch) return -1;

    rtc_time_t time;
    rtc_read_datetime(&time);

    if (time.month < 1 || time.month > 12 || time.day < 1 ||
        time.hour > 23 || time.minute > 59 || time.second > 59) {
        return -1;
    }

    static const uint8_t days_in_month[12] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    uint8_t max_day = days_in_month[time.month - 1];
    if (time.month == 2 && rtc_is_leap_year(time.year)) max_day = 29;
    if (time.day > max_day || time.year < 1970) return -1;

    *epoch = rtc_to_epoch(&time);
    return 0;
}
