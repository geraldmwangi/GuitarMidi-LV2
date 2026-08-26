#pragma once

#include <lv2/core/lv2.h>

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
#include <lv2/log/logger.h>
#include <guitarmidi/logging.hpp>


class LV2Logger: public LoggingAPI{
    private:
    LV2_Log_Logger m_logger;
    public: 
    LV2Logger(LV2_URID_Map *,LV2_Log_Log *);
        virtual void info(std::string info_message,...);
    virtual void error(std::string error_message,...);
    virtual void warn(std::string warn_message,...);
};