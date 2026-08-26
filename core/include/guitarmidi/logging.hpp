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
class LoggingAPI{
    public:
    virtual void info(std::string info_message)=0;
    virtual void error(std::string error_message)=0;
    virtual void warn(std::string warn_message)=0;
};
class LoggerSingleton{
    private:
    LoggingAPI* m_logger=0;
    public:
    void info(std::string info_message){
        if(m_logger)
            m_logger->info(info_message);
    }
    void error(std::string error_message){
        if(m_logger)
            m_logger->error(error_message);
    }
    void warn(std::string warn_message){
        if(m_logger)
            m_logger->warn(warn_message);
    }

};

extern LoggerSingleton g_logger;
// call this function to start a nanosecond-resolution timer
struct timespec timer_start();

// call this function to end a timer, returning nanoseconds elapsed as a long
long timer_end(struct timespec start_time);