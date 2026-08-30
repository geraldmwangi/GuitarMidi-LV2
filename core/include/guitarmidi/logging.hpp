#pragma once
/* GuitarMidi-LV2 Library
 * Copyright (C) 2022 Gerald Mwangi
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include <time.h>
#include <guitarmidi/config.hpp>
#include <string>
#include <stdarg.h>
class LoggingAPI
{
public:
    virtual void info(std::string info_message, va_list args) = 0;
    virtual void error(std::string error_message, va_list args) = 0;
    virtual void warn(std::string warn_message, va_list args) = 0;
};
class LoggerSingleton
{
private:
    LoggingAPI *m_logger = nullptr;

public:
    LoggerSingleton(LoggingAPI* logger=nullptr){
        m_logger=logger;
    }
    ~LoggerSingleton(){
        if(m_logger)
            delete m_logger;
    }
    void info(std::string info_message, ...)
    {
        va_list args;
        va_start(args, info_message);
        if (m_logger)
            m_logger->info(info_message, args);
        va_end(args);
    }
    void error(std::string error_message, ...)
    {
        va_list args;
        va_start(args, error_message);
        if (m_logger)
            m_logger->error(error_message, args);
        va_end(args);
    }
    void warn(std::string warn_message, ...)
    {
        va_list args;
        va_start(args, warn_message);
        if (m_logger)
            m_logger->warn(warn_message, args);
        va_end(args);
    }

    void setLogger(LoggingAPI* logger){
        if(m_logger!=nullptr)   
            delete m_logger;
        m_logger=logger;
    }
};

extern LoggerSingleton g_logger;
// call this function to start a nanosecond-resolution timer
struct timespec timer_start();

// call this function to end a timer, returning nanoseconds elapsed as a long
long timer_end(struct timespec start_time);