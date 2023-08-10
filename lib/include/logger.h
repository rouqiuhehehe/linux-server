//
// Created by 115282 on 2023/8/10.
//

#ifndef LINUX_SERVER_LIB_KV_STORE_LOGGER_H_
#define LINUX_SERVER_LIB_KV_STORE_LOGGER_H_

#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>

#define LOG_INFO(fmtStr, ...) SPDLOG_INFO(fmt::format(fmtStr, ##__VA_ARGS__))
#define LOG_WARING(fmtStr, ...) SPDLOG_WARN(fmt::format(fmtStr, ##__VA_ARGS__))
#define LOG_ERROR(fmtStr, ...) SPDLOG_ERROR(fmt::format(fmtStr, ##__VA_ARGS__))

class Logger
{
public:
    explicit Logger (const std::string &filepath = "./")
    {
        auto dailyLogger = spdlog::daily_logger_mt("daily_logger_mt", filepath + "daily.log");
        spdlog::set_default_logger(dailyLogger);

#ifdef NDEBUG
        spdlog::set_level(spdlog::level::info);
#else
        spdlog::set_level(spdlog::level::trace);
#endif
        spdlog::set_pattern("*** [%Y-%m-%d %T.%e][thread %t][%^%l%$][%s:%!:%#] %v ***");
    }
    template <class ...Arg>
    void info (Arg &&...arg)
    {
        spdlog::info(std::forward <Arg>(arg)...);
    }
};

#endif //LINUX_SERVER_LIB_KV_STORE_LOGGER_H_
