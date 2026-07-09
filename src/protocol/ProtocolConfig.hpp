#pragma once

#include <cstdint>
#include <string>

#include "config/Section.hpp"
#include "core/Result.hpp"

namespace RSCoin::Protocol {

    struct ProtocolConfig {
        static constexpr const char* kSection = "protocol";

        std::string name;
        std::uint32_t version{};

        static core::Result<ProtocolConfig> from(const Config::Section& section);
    };

}
