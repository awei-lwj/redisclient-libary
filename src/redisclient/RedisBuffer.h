#ifndef REDISCLIENT_REDISBUFFER_H
#define REDISCLIENT_REDISBUFFER_H

#include "Config.h"

#include <boost/variant.hpp>

#include <string>
#include <vector>

namespace redisclient
{
    struct RedisBuffer
    {
        RedisBuffer() = default;

        // decline the buffer
        inline RedisBuffer(const char *ptr, size_t dataSize);
        inline RedisBuffer(const char *s);
        inline RedisBuffer(std::string s);
        inline RedisBuffer(std::vector<char> buf);

        inline size_t size() const;

        boost::variant<std::string, std::vector<char>> data;
    };

    RedisBuffer::RedisBuffer(const char *ptr, size_t dataSize)
        : data(std::vector<char>(ptr, ptr + dataSize))
    {
    }

    RedisBuffer::RedisBuffer(const char *s)
        : data(std::string(s))
    {
    }

    RedisBuffer::RedisBuffer(std::string s)
        : data(std::move(s))
    {
    }

    RedisBuffer::RedisBuffer(std::vector<char> buf)
        : data(std::move(buf))
    {
    }

    size_t RedisBuffer::size() const
    {
        if (data.type() == typeid(std::string))
            return boost::get<std::string>(data).size();
        else
            return boost::get<std::vector<char>>(data).size();
    }

};

#endif // REDISCLIENT_REDISBUFFER_H