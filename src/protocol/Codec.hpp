#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>

#include "core/Result.hpp"
#include "core/Types.hpp"

namespace RSCoin::Protocol {

    class Writer {
    public:
        template <std::unsigned_integral T>
        Writer& write(T value) {
            for (unsigned shift = 0; shift < sizeof(T) * 8; shift += 8)
                _bytes.push_back(static_cast<std::byte>((value >> shift) & 0xFF));
            return *this;
        }

        template <std::size_t N>
        Writer& writeFixed(const core::FixedBytes<N>& value) {
            _bytes.insert(_bytes.end(), value.bytes.begin(), value.bytes.end());
            return *this;
        }

        Writer& writeBytes(core::BytesView data) {
            write(static_cast<std::uint32_t>(data.size()));
            _bytes.insert(_bytes.end(), data.begin(), data.end());
            return *this;
        }

        core::Bytes take() { return std::move(_bytes); }

    private:
        core::Bytes _bytes;
    };

    class Reader {
    public:
        Reader(core::BytesView data) : _data(data) {}

        template <std::unsigned_integral T>
        core::Result<T> read() {
            if (_offset + sizeof(T) > _data.size())
                return core::fail(core::ErrorCode::protocol, "truncated payload");

            T value{};
            for (unsigned i = 0; i < sizeof(T); ++i)
                value |= static_cast<T>(std::to_integer<T>(_data[_offset + i]) << (8 * i));
            _offset += sizeof(T);
            return value;
        }

        template <std::size_t N>
        core::Result<core::FixedBytes<N>> readFixed() {
            if (_offset + N > _data.size())
                return core::fail(core::ErrorCode::protocol, "truncated payload");

            core::FixedBytes<N> value;
            for (std::size_t i = 0; i < N; ++i)
                value.bytes[i] = _data[_offset + i];
            _offset += N;
            return value;
        }

        core::Result<core::Bytes> readBytes() {
            const auto size = read<std::uint32_t>();
            if (!size)
                return core::fail(size.error());
            if (_offset + *size > _data.size())
                return core::fail(core::ErrorCode::protocol, "truncated payload");

            core::Bytes value(_data.begin() + static_cast<std::ptrdiff_t>(_offset), _data.begin() + static_cast<std::ptrdiff_t>(_offset + *size));
            _offset += *size;
            return value;
        }

        bool finished() const noexcept { return _offset == _data.size(); }

    private:
        core::BytesView _data;
        std::size_t _offset{};
    };

}
