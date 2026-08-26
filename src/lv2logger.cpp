#include <lv2logger.hpp>

LV2Logger::LV2Logger(LV2_URID_Map *map, LV2_Log_Log *log)
{
    lv2_log_logger_init(&m_logger, map, log);
}

void LV2Logger::info(std::string info_message, ...)
{
    va_list args;
    va_start(args, info_message);
    lv2_log_note(&m_logger, info_message.c_str(), args);
    va_end(args);
}

void LV2Logger::error(std::string error_message, ...)
{
    va_list args;
    va_start(args, error_message);
    lv2_log_error(&m_logger, error_message.c_str(), args);
    va_end(args);
}

void LV2Logger::warn(std::string warn_message, ...)
{
    va_list args;
    va_start(args, warn_message);
    lv2_log_warning(&m_logger, warn_message.c_str(), args);
    va_end(args);
}
