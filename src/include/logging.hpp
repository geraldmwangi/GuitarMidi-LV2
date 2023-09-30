#pragma once
#include <lv2/log/logger.h>
#include <time.h>
extern LV2_Log_Logger g_logger;

// call this function to start a nanosecond-resolution timer
struct timespec timer_start();

// call this function to end a timer, returning nanoseconds elapsed as a long
long timer_end(struct timespec start_time);