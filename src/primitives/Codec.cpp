#include "primitives/Codec.hpp"

#include <cstdint>

namespace RSCoin::Primitives {

    namespace {
        class ByteWriter {
        public:
            template <typename T>
            void writeLE(T value) {
                for (unsigned shift = 0; shift < sizeof(T) * 8; shift += 8)
                    _bytes.push_back(static_cast<std::byte>((value >> shift) & 0xFF));
            }

            void writeBytes(core::BytesView data) {
                writeLE<std::uint64_t>(data.size());
                _bytes.insert(_bytes.end(), data.begin(), data.end());
            }

            template <std::size_t N>
            void writeFixed(const core::FixedBytes<N>& data) {
                _bytes.insert(_bytes.end(), data.bytes.begin(), data.bytes.end());
            }

            core::Bytes take() { return std::move(_bytes); }

        private:
            core::Bytes _bytes;
        };

        class ByteReader {
        public:
            explicit ByteReader(core::BytesView data) : _data(data) {}

            template <typename T>
            core::Result<T> readLE() {
                if (_offset + sizeof(T) > _data.size())
                    return core::fail(core::ErrorCode::validation, "truncated encoding");
                T value{};
                for (unsigned i = 0; i < sizeof(T); ++i)
                    value |= static_cast<T>(std::to_integer<T>(_data[_offset + i]) << (8 * i));
                _offset += sizeof(T);
                return value;
            }

            core::Result<core::Bytes> readBytes() {
                const auto size = readLE<std::uint64_t>();
                if (!size)
                    return core::fail(size.error());
                if (_offset + *size > _data.size())
                    return core::fail(core::ErrorCode::validation, "truncated encoding");
                core::Bytes out(_data.begin() + static_cast<std::ptrdiff_t>(_offset), _data.begin() + static_cast<std::ptrdiff_t>(_offset + *size));
                _offset += *size;
                return out;
            }

            template <std::size_t N>
            core::Result<core::FixedBytes<N>> readFixed() {
                if (_offset + N > _data.size())
                    return core::fail(core::ErrorCode::validation, "truncated encoding");
                core::FixedBytes<N> out;
                for (std::size_t i = 0; i < N; ++i)
                    out.bytes[i] = _data[_offset + i];
                _offset += N;
                return out;
            }

            bool finished() const noexcept { return _offset == _data.size(); }

        private:
            core::BytesView _data;
            std::size_t _offset{};
        };

        void writeHeaderTo(ByteWriter& writer, const BlockHeader& header) {
            writer.writeLE<std::uint32_t>(header.version);
            writer.writeFixed(header.parentHash);
            writer.writeFixed(header.stateRoot);
            writer.writeFixed(header.transactionsRoot);
            writer.writeLE<std::uint64_t>(header.height);
            writer.writeLE<std::uint64_t>(header.timestamp);
            writer.writeFixed(header.beneficiary);
            writer.writeBytes(header.extraData);
            writer.writeBytes(header.consensusSeal);
            writer.writeLE<std::uint64_t>(header.extensions.size());
            for (const auto& extension : header.extensions) {
                writer.writeLE<std::uint16_t>(extension.tag);
                writer.writeBytes(extension.payload);
            }
        }

        void writeTransactionTo(ByteWriter& writer, const Transaction& transaction) {
            writer.writeLE<std::uint8_t>(transaction.type);
            writer.writeFixed(transaction.from);
            writer.writeFixed(transaction.to);
            writer.writeLE<std::uint64_t>(transaction.value.units);
            writer.writeLE<std::uint64_t>(transaction.nonce);
            writer.writeBytes(transaction.payload);
            writer.writeBytes(transaction.signature.data);
        }

        core::Result<BlockHeader> readHeaderFrom(ByteReader& reader) {
            BlockHeader header;

            auto version = reader.readLE<std::uint32_t>();
            if (!version)
                return core::fail(version.error());
            header.version = *version;

            auto parentHash = reader.readFixed<32>();
            auto stateRoot = reader.readFixed<32>();
            auto transactionsRoot = reader.readFixed<32>();
            if (!parentHash || !stateRoot || !transactionsRoot)
                return core::fail(core::ErrorCode::validation, "truncated header");
            header.parentHash = *parentHash;
            header.stateRoot = *stateRoot;
            header.transactionsRoot = *transactionsRoot;

            auto height = reader.readLE<std::uint64_t>();
            auto timestamp = reader.readLE<std::uint64_t>();
            auto beneficiary = reader.readFixed<20>();
            if (!height || !timestamp || !beneficiary)
                return core::fail(core::ErrorCode::validation, "truncated header");
            header.height = *height;
            header.timestamp = *timestamp;
            header.beneficiary = *beneficiary;

            auto extraData = reader.readBytes();
            auto consensusSeal = reader.readBytes();
            auto extensionCount = reader.readLE<std::uint64_t>();
            if (!extraData || !consensusSeal || !extensionCount)
                return core::fail(core::ErrorCode::validation, "truncated header");
            header.extraData = std::move(*extraData);
            header.consensusSeal = std::move(*consensusSeal);

            for (std::uint64_t i = 0; i < *extensionCount; ++i) {
                auto tag = reader.readLE<std::uint16_t>();
                auto payload = reader.readBytes();
                if (!tag || !payload)
                    return core::fail(core::ErrorCode::validation, "truncated header extension");
                header.extensions.push_back({*tag, std::move(*payload)});
            }
            return header;
        }

        core::Result<Transaction> readTransactionFrom(ByteReader& reader) {
            Transaction transaction;

            auto type = reader.readLE<std::uint8_t>();
            auto from = reader.readFixed<20>();
            auto to = reader.readFixed<20>();
            auto value = reader.readLE<std::uint64_t>();
            auto nonce = reader.readLE<std::uint64_t>();
            if (!type || !from || !to || !value || !nonce)
                return core::fail(core::ErrorCode::validation, "truncated transaction");
            transaction.type = *type;
            transaction.from = *from;
            transaction.to = *to;
            transaction.value = core::Amount{*value};
            transaction.nonce = *nonce;

            auto payload = reader.readBytes();
            auto signature = reader.readBytes();
            if (!payload || !signature)
                return core::fail(core::ErrorCode::validation, "truncated transaction");
            transaction.payload = std::move(*payload);
            transaction.signature.data = std::move(*signature);
            return transaction;
        }
    }

    core::Bytes encode(const BlockHeader& header) {
        ByteWriter writer;
        writeHeaderTo(writer, header);
        return writer.take();
    }

    core::Bytes encode(const Transaction& transaction) {
        ByteWriter writer;
        writeTransactionTo(writer, transaction);
        return writer.take();
    }

    core::Bytes encodeForSigning(const Transaction& transaction, std::uint64_t chainId) {
        ByteWriter writer;
        writer.writeLE<std::uint8_t>(transaction.type);
        writer.writeFixed(transaction.from);
        writer.writeFixed(transaction.to);
        writer.writeLE<std::uint64_t>(transaction.value.units);
        writer.writeLE<std::uint64_t>(transaction.nonce);
        writer.writeBytes(transaction.payload);
        writer.writeLE<std::uint64_t>(chainId);
        return writer.take();
    }

    core::Bytes encode(const Block& block) {
        ByteWriter writer;
        writeHeaderTo(writer, block.header);
        writer.writeLE<std::uint64_t>(block.transactions.size());
        for (const auto& transaction : block.transactions)
            writeTransactionTo(writer, transaction);
        return writer.take();
    }

    core::Result<BlockHeader> decodeHeader(core::BytesView data) {
        ByteReader reader(data);
        return readHeaderFrom(reader);
    }

    core::Result<Transaction> decodeTransaction(core::BytesView data) {
        ByteReader reader(data);
        return readTransactionFrom(reader);
    }

    core::Result<Block> decodeBlock(core::BytesView data) {
        ByteReader reader(data);

        auto header = readHeaderFrom(reader);
        if (!header)
            return core::fail(header.error());

        auto count = reader.readLE<std::uint64_t>();
        if (!count)
            return core::fail(count.error());

        Block block;
        block.header = std::move(*header);
        for (std::uint64_t i = 0; i < *count; ++i) {
            auto transaction = readTransactionFrom(reader);
            if (!transaction)
                return core::fail(transaction.error());
            block.transactions.push_back(std::move(*transaction));
        }
        return block;
    }

}
