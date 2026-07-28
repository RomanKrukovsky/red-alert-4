// Copyright (c) Red Alert 4 project. Canonical little-endian serialization.
//
// Used for the command stream, replays, saves and network packets. Everything that
// crosses a process boundary or gets hashed goes through here, so the byte layout
// is defined in exactly one place and cannot drift between writer and reader.
#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace RA4
{

class ByteWriter
{
public:
    void WriteUInt8(uint8_t V) { Buffer.push_back(V); }
    void WriteInt8(int8_t V) { WriteUInt8(uint8_t(V)); }

    void WriteUInt16(uint16_t V)
    {
        Buffer.push_back(uint8_t(V & 0xFF));
        Buffer.push_back(uint8_t((V >> 8) & 0xFF));
    }
    void WriteInt16(int16_t V) { WriteUInt16(uint16_t(V)); }

    void WriteUInt32(uint32_t V)
    {
        for (int32_t I = 0; I < 4; ++I)
        {
            Buffer.push_back(uint8_t((V >> (I * 8)) & 0xFF));
        }
    }
    void WriteInt32(int32_t V) { WriteUInt32(uint32_t(V)); }

    void WriteUInt64(uint64_t V)
    {
        for (int32_t I = 0; I < 8; ++I)
        {
            Buffer.push_back(uint8_t((V >> (I * 8)) & 0xFF));
        }
    }
    void WriteInt64(int64_t V) { WriteUInt64(uint64_t(V)); }

    void WriteBool(bool V) { WriteUInt8(V ? 1u : 0u); }

    void WriteString(const std::string& S)
    {
        WriteUInt16(uint16_t(S.size()));
        Buffer.insert(Buffer.end(), S.begin(), S.end());
    }

    void WriteBytes(const uint8_t* Data, size_t Count) { Buffer.insert(Buffer.end(), Data, Data + Count); }

    const std::vector<uint8_t>& GetBuffer() const { return Buffer; }
    std::vector<uint8_t>& GetMutableBuffer() { return Buffer; }
    size_t Size() const { return Buffer.size(); }
    void Reset() { Buffer.clear(); }

private:
    std::vector<uint8_t> Buffer;
};

class ByteReader
{
public:
    ByteReader(const uint8_t* InData, size_t InSize) : Data(InData), Size(InSize) {}
    explicit ByteReader(const std::vector<uint8_t>& InBuffer) : Data(InBuffer.data()), Size(InBuffer.size()) {}

    // Every read is bounds checked and sets the error flag instead of throwing or
    // reading past the end. A malicious client packet must never be able to crash
    // the dedicated server, so the failure mode is "stream is now invalid" and the
    // caller drops the whole packet.
    uint8_t ReadUInt8()
    {
        if (Offset + 1 > Size) { bError = true; return 0; }
        return Data[Offset++];
    }
    int8_t ReadInt8() { return int8_t(ReadUInt8()); }

    uint16_t ReadUInt16()
    {
        if (Offset + 2 > Size) { bError = true; Offset = Size; return 0; }
        const uint16_t V = uint16_t(uint32_t(Data[Offset]) | (uint32_t(Data[Offset + 1]) << 8));
        Offset += 2;
        return V;
    }
    int16_t ReadInt16() { return int16_t(ReadUInt16()); }

    uint32_t ReadUInt32()
    {
        if (Offset + 4 > Size) { bError = true; Offset = Size; return 0; }
        uint32_t V = 0;
        for (int32_t I = 0; I < 4; ++I)
        {
            V |= uint32_t(Data[Offset + size_t(I)]) << (I * 8);
        }
        Offset += 4;
        return V;
    }
    int32_t ReadInt32() { return int32_t(ReadUInt32()); }

    uint64_t ReadUInt64()
    {
        if (Offset + 8 > Size) { bError = true; Offset = Size; return 0; }
        uint64_t V = 0;
        for (int32_t I = 0; I < 8; ++I)
        {
            V |= uint64_t(Data[Offset + size_t(I)]) << (I * 8);
        }
        Offset += 8;
        return V;
    }
    int64_t ReadInt64() { return int64_t(ReadUInt64()); }

    bool ReadBool() { return ReadUInt8() != 0; }

    std::string ReadString()
    {
        const uint16_t Len = ReadUInt16();
        if (Offset + Len > Size) { bError = true; Offset = Size; return std::string(); }
        std::string S(reinterpret_cast<const char*>(Data + Offset), Len);
        Offset += Len;
        return S;
    }

    bool HasError() const { return bError; }
    size_t Remaining() const { return Size - Offset; }
    size_t Tell() const { return Offset; }

private:
    const uint8_t* Data = nullptr;
    size_t Size = 0;
    size_t Offset = 0;
    bool bError = false;
};

} // namespace RA4
