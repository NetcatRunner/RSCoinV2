#pragma once

#include "RST/log/Log.hpp"

namespace RSCoin::Log {
    class Logger {
    public:
        static void Init();

        static std::shared_ptr<RST::Log::Logger> GetLogger() { 
            if (s_Logger == nullptr)
                Init();
            return s_Logger;
        };

    private:
        static std::shared_ptr<RST::Log::Logger> s_Logger;
    };
}

#define RSCoin_TRACE(...) LOG_TRACE_L(RSCoin::Log::Logger::GetLogger(), __VA_ARGS__)
#define RSCoin_DEBUG(...) LOG_DEBUG_L(RSCoin::Log::Logger::GetLogger(), __VA_ARGS__)
#define RSCoin_INFO(...)  LOG_INFO_L(RSCoin::Log::Logger::GetLogger(),  __VA_ARGS__)
#define RSCoin_WARN(...)  LOG_WARN_L(RSCoin::Log::Logger::GetLogger(),  __VA_ARGS__)
#define RSCoin_ERROR(...) LOG_ERROR_L(RSCoin::Log::Logger::GetLogger(), __VA_ARGS__)
#define RSCoin_FATAL(...) LOG_FATAL_L(RSCoin::Log::Logger::GetLogger(), __VA_ARGS__)
