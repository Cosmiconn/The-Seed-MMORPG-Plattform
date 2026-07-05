#include "core/serialize.hpp"
#include <cstring>
#include <algorithm>

namespace TheSeed::Serialize {

// --- BinarySerializer ---

void BinarySerializer::Write(std::span<const uint8_t> data) {
    m_buffer.insert(m_buffer.end(), data.begin(), data.end());
}

void BinarySerializer::WriteU32(uint32_t v) {
    m_buffer.push_back(static_cast<uint8_t>(v));
    m_buffer.push_back(static_cast<uint8_t>(v >> 8));
    m_buffer.push_back(static_cast<uint8_t>(v >> 16));
    m_buffer.push_back(static_cast<uint8_t>(v >> 24));
}

void BinarySerializer::WriteF32(float v) {
    static_assert(sizeof(float) == 4, "float muss 4 Bytes sein");
    uint32_t bits;
    std::memcpy(&bits, &v, sizeof(float));
    WriteU32(bits);
}

void BinarySerializer::WriteString(const std::string& s) {
    WriteU32(static_cast<uint32_t>(s.size()));
    m_buffer.insert(m_buffer.end(), s.begin(), s.end());
}

std::vector<uint8_t> BinarySerializer::Finalize() {
    return std::move(m_buffer);
}

// --- BinaryDeserializer ---

BinaryDeserializer::BinaryDeserializer(std::span<const uint8_t> data)
    : m_data(data.begin(), data.end()) {}

bool BinaryDeserializer::CanRead(size_t bytes) const {
    return m_pos + bytes <= m_data.size();
}

bool BinaryDeserializer::Read(std::span<uint8_t> out) {
    if (!CanRead(out.size())) return false;
    std::memcpy(out.data(), m_data.data() + m_pos, out.size());
    m_pos += out.size();
    return true;
}

uint32_t BinaryDeserializer::ReadU32() {
    if (!CanRead(4)) throw std::runtime_error("ReadU32: Nicht genug Daten");
    uint32_t v = static_cast<uint32_t>(m_data[m_pos])
               | (static_cast<uint32_t>(m_data[m_pos + 1]) << 8)
               | (static_cast<uint32_t>(m_data[m_pos + 2]) << 16)
               | (static_cast<uint32_t>(m_data[m_pos + 3]) << 24);
    m_pos += 4;
    return v;
}

float BinaryDeserializer::ReadF32() {
    uint32_t bits = ReadU32();
    float v;
    std::memcpy(&v, &bits, sizeof(float));
    return v;
}

std::string BinaryDeserializer::ReadString() {
    uint32_t len = ReadU32();
    if (!CanRead(len)) throw std::runtime_error("ReadString: Nicht genug Daten");
    std::string s;
    s.resize(len);
    std::memcpy(s.data(), m_data.data() + m_pos, len);
    m_pos += len;
    return s;
}

// --- Delta-Kompression ---

std::vector<uint8_t> ComputeDelta(std::span<const uint8_t> oldState, std::span<const uint8_t> newState) {
    size_t minLen = std::min(oldState.size(), newState.size());
    size_t maxLen = std::max(oldState.size(), newState.size());

    std::vector<uint8_t> delta;
    delta.reserve(maxLen + 8);

    // Format: [uint32_t newSize][uint32_t oldSize][XOR-Daten]
    delta.push_back(static_cast<uint8_t>(newState.size() & 0xFF));
    delta.push_back(static_cast<uint8_t>((newState.size() >> 8) & 0xFF));
    delta.push_back(static_cast<uint8_t>((newState.size() >> 16) & 0xFF));
    delta.push_back(static_cast<uint8_t>((newState.size() >> 24) & 0xFF));
    delta.push_back(static_cast<uint8_t>(oldState.size() & 0xFF));
    delta.push_back(static_cast<uint8_t>((oldState.size() >> 8) & 0xFF));
    delta.push_back(static_cast<uint8_t>((oldState.size() >> 16) & 0xFF));
    delta.push_back(static_cast<uint8_t>((oldState.size() >> 24) & 0xFF));

    for (size_t i = 0; i < minLen; ++i) {
        delta.push_back(oldState[i] ^ newState[i]);
    }

    // Restliche neue Daten anhängen (falls newState > oldState)
    for (size_t i = minLen; i < newState.size(); ++i) {
        delta.push_back(newState[i]);
    }

    return delta;
}

std::vector<uint8_t> ApplyDelta(std::span<const uint8_t> oldState, std::span<const uint8_t> delta) {
    if (delta.size() < 8) return std::vector<uint8_t>(oldState.begin(), oldState.end());

    size_t newSize = static_cast<size_t>(delta[0])
                   | (static_cast<size_t>(delta[1]) << 8)
                   | (static_cast<size_t>(delta[2]) << 16)
                   | (static_cast<size_t>(delta[3]) << 24);
    size_t oldSize = static_cast<size_t>(delta[4])
                   | (static_cast<size_t>(delta[5]) << 8)
                   | (static_cast<size_t>(delta[6]) << 16)
                   | (static_cast<size_t>(delta[7]) << 24);

    if (oldSize != oldState.size()) {
        // Größen-Mismatch → Fallback
        return std::vector<uint8_t>(oldState.begin(), oldState.end());
    }

    std::vector<uint8_t> result(newSize);
    size_t minLen = std::min(oldState.size(), newSize);
    size_t dataOffset = 8;

    for (size_t i = 0; i < minLen; ++i) {
        if (dataOffset + i < delta.size()) {
            result[i] = oldState[i] ^ delta[dataOffset + i];
        } else {
            result[i] = oldState[i];
        }
    }

    // Restliche neue Daten
    for (size_t i = minLen; i < newSize; ++i) {
        if (dataOffset + i < delta.size()) {
            result[i] = delta[dataOffset + i];
        }
    }

    return result;
}

} // namespace TheSeed::Serialize
