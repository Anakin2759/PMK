/**
 * ************************************************************************
 *
 * @file PacketStream.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-23
 * @version 0.1
 * @brief 二进制流读写工具
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <cstring>
#include <concepts>
#include <string_view>

namespace shared
{

class PacketWriter
{
public:
    std::vector<uint8_t> buffer;

    PacketWriter() { buffer.reserve(128); }

    void writeUint8(uint8_t v) { buffer.push_back(v); }

    void writeUint16(uint16_t v)
    {
        uint8_t bytes[2];
        std::memcpy(bytes, &v, 2);
        buffer.insert(buffer.end(), std::begin(bytes), std::end(bytes));
    }

    void writeUint32(uint32_t v)
    {
        uint8_t bytes[4];
        std::memcpy(bytes, &v, 4);
        buffer.insert(buffer.end(), std::begin(bytes), std::end(bytes));
    }

    void writeBool(bool v) { writeUint8(v ? 1 : 0); }

    void writeString(std::string_view str)
    {
        if (str.size() > UINT16_MAX)
        {
            throw std::length_error("String too long for packet");
        }
        writeUint16(static_cast<uint16_t>(str.size()));
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(str.data());
        buffer.insert(buffer.end(), ptr, ptr + str.size());
    }

    template <typename T>
        requires std::is_trivial_v<T> && (!std::is_pointer_v<T>)
    void writePOD(const T& t)
    {
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&t);
        buffer.insert(buffer.end(), ptr, ptr + sizeof(T));
    }
};

class PacketReader
{
    std::span<const uint8_t> data;
    size_t cursor = 0;

public:
    PacketReader(std::span<const uint8_t> data) : data(data) {}

    bool hasCapacity(size_t size) const { return cursor + size <= data.size(); }

    uint8_t readUint8()
    {
        if (!hasCapacity(1)) throw std::out_of_range("buffer overflow");
        return data[cursor++];
    }

    uint16_t readUint16()
    {
        if (!hasCapacity(2)) throw std::out_of_range("buffer overflow");
        uint16_t v;
        std::memcpy(&v, data.data() + cursor, 2);
        cursor += 2;
        return v;
    }

    uint32_t readUint32()
    {
        if (!hasCapacity(4)) throw std::out_of_range("buffer overflow");
        uint32_t v;
        std::memcpy(&v, data.data() + cursor, 4);
        cursor += 4;
        return v;
    }

    bool readBool() { return readUint8() != 0; }

    std::string readString()
    {
        uint16_t len = readUint16();
        if (!hasCapacity(len)) throw std::out_of_range("buffer overflow");
        std::string s(reinterpret_cast<const char*>(data.data() + cursor), len);
        cursor += len;
        return s;
    }

    template <typename T>
        requires std::is_trivial_v<T> && (!std::is_pointer_v<T>)
    T readPOD()
    {
        if (!hasCapacity(sizeof(T))) throw std::out_of_range("buffer overflow");
        T t;
        std::memcpy(&t, data.data() + cursor, sizeof(T));
        cursor += sizeof(T);
        return t;
    }
};

} // namespace shared
