#include "Logger.hpp"

namespace RSCoin::Log {
    std::shared_ptr<RST::Log::Logger> Logger::s_Logger = nullptr;

    void Logger::Init() {
        s_Logger = RST::Log::Registry::init("RSCoin", RST::Log::LogLevel::Trace);
    }
}
