#include <lv2logger.hpp>

LV2Logger::LV2Logger(LV2_URID_Map *map, LV2_Log_Log *log)
{
    lv2_log_logger_init(&m_logger, map, log);
}

void LV2Logger::info(std::string info_message, va_list args)
{
    lv2_log_vprintf(&m_logger, m_logger.Note, info_message.c_str(), args);
}

void LV2Logger::error(std::string error_message, va_list args)
{
    lv2_log_vprintf(&m_logger, m_logger.Error, error_message.c_str(), args);
}

void LV2Logger::warn(std::string warn_message, va_list args)
{
    lv2_log_vprintf(&m_logger, m_logger.Warning, warn_message.c_str(), args);
}
