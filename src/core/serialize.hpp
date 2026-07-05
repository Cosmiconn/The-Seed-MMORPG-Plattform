#pragma once
#include <vector>
#include <cstdint>
#include <span>
#include <string>
#include <type_traits>
#include <stdexcept>

namespace TheSeed::Serialize {

class ISerializer {
public:
    virtual ~ISerializer() = default;
    virtual void Write(std::span<const uint8_t> data) = 0;
    virtual void WriteU32(uint32_t v) = 0;
    virtual void WriteF32(float v) = 0;
    virtual void WriteString(const std::string& s) = 0;
    virtual std::vector<uint8_t> Finalize() = 0;
};

class IDeserializer {
public:
    virtual ~IDeserializer() = default;
    virtual bool Read(std::span<uint8_t> out) = 0;
    virtual uint32_t ReadU32() = 0;
    virtual float ReadF32() = 0;
    virtual std::string ReadString() = 0;
};

// Delta-Kompression: Nur geänderte Bits senden
std::vector<uint8_t> ComputeDelta(std::span<const uint8_t> oldState, std::span<const uint8_t> newState);
std::vector<uint8_t> ApplyDelta(std::span<const uint8_t> oldState, std::span<const uint8_t> delta);

// --- Konkrete Implementierungen ---
class BinarySerializer : public ISerializer {
public:
    void Write(std::span<const uint8_t> data) override;
    void WriteU32(uint32_t v) override;
    void WriteF32(float v) override;
    void WriteString(const std::string& s) override;
    std::vector<uint8_t> Finalize() override;

private:
    std::vector<uint8_t> m_buffer;
};

class BinaryDeserializer : public IDeserializer {
public:
    explicit BinaryDeserializer(std::span<const uint8_t> data);

    bool Read(std::span<uint8_t> out) override;
    uint32_t ReadU32() override;
    float ReadF32() override;
    std::string ReadString() override;

private:
    std::vector<uint8_t> m_data;
    size_t m_pos = 0;

    bool CanRead(size_t bytes) const;
};

} // namespace TheSeed::Serialize
