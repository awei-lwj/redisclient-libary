#pragma once

#include "Config.h"
#include "RedisBuffer.h"
#include "RedisValue.h"

#include <deque>
#include <boost/system/error_code.hpp>

namespace redisclient
{
    class RedisSyncClient;
    class RedisValue;

    class Pipeline
    {
    public:
        REDIS_CLIENT_DECL Pipeline(RedisSyncClient &client);

        // add command to pipeline
        REDIS_CLIENT_DECL Pipeline &command(std::string cmd, std::deque<RedisBuffer> args);

        // Sends all command to to the redis server
        // For every request command will get response value
        REDIS_CLIENT_DECL RedisValue finish();
        REDIS_CLIENT_DECL RedisValue finish(boost::system::error_code &ec);

    private:
        std::deque<std::deque<RedisBuffer>> commands;
        RedisSyncClient &client;
    };

}

#ifndef REDIS_CLIENT_HEADER_ONLY
#include "Pipeline.cpp"
#endif