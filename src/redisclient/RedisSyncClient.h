#ifndef REDISCLIENT_REDISSYNCCLIENT_H
#define REDISCLIENT_REDISSYNCCLIENT_H

#include "RedisClientImpl.h"
#include "RedisBuffer.h"
#include "RedisValue.h"
#include "Config.h"
#include "PipeLine.h"

#include <boost/asio/io_service.hpp>
#include <boost/noncopyable.hpp>

#include <string>
#include <list>
#include <functional>

namespace redisclient
{
    class RedisClientImpl;
    class Pipeline;

    class RedisSyncClient : boost::noncopyable
    {
    public:
        typedef RedisClientImpl::State State;

        REDIS_CLIENT_DECL RedisSyncClient(boost::asio::io_service &ioService);
        REDIS_CLIENT_DECL RedisSyncClient(RedisSyncClient &&other);
        REDIS_CLIENT_DECL ~RedisSyncClient(); 

        // Connect to redis server
        REDIS_CLIENT_DECL void connect(
            const boost::asio::ip::tcp::endpoint &endpoint,
            boost::system::error_code &ec
        );

        REDIS_CLIENT_DECL void connect(
            const boost::asio::ip::tcp::endpoint &endpoint
        );

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS
        REDIS_CLIENT_DECL void connect(
            const boost::asio::ip::tcp::endpoint &endpoint
        );

        REDIS_CLIENT_DECL void connect(
            const boost::asio::ip::tcp::endpoint &endpoint,
            boost::system::error_code &ec
        );        
#endif
        // Return true if creates redis connection succeeded
        REDIS_CLIENT_DECL bool isConnected() const;

        // Disconnect from redis server
        REDIS_CLIENT_DECL void disConnected();

        // Set custom error handler
        REDIS_CLIENT_DECL void installErrorHandler(
            std::function<void(const std::string &)> handler
        );

        // Execute command on Redis Server with the list of arguments
        REDIS_CLIENT_DECL RedisValue command(
            std::string cmd,
            std::deque<RedisBuffer> args
        );

        REDIS_CLIENT_DECL RedisValue command(
            std::string cmd,
            std::deque<RedisBuffer> args,
            boost::system::error_code &ec
        );

        // Create pipe line
        REDIS_CLIENT_DECL Pipeline pipelined();

        REDIS_CLIENT_DECL RedisValue pipelined(
            std::deque<std::deque<RedisBuffer>> commands,
            boost::system::error_code &ec
        );

        REDIS_CLIENT_DECL RedisValue pipelined(
            std::deque<std::deque<RedisBuffer>> commands
        );

        // Return connection statement
        REDIS_CLIENT_DECL State state() const;

        REDIS_CLIENT_DECL RedisSyncClient &setConnectTimeout(
            const boost::posix_time::time_duration &timeout
        );

        REDIS_CLIENT_DECL RedisSyncClient &setCommandTimeout(
            const boost::posix_time::time_duration &timeout
        );

        REDIS_CLIENT_DECL RedisSyncClient &setTcpNoDelay(bool enable);
        REDIS_CLIENT_DECL RedisSyncClient &setTcpKeepAlive(bool enable);

    protected:
        REDIS_CLIENT_DECL bool statementValid() const;

    private:
        std::shared_ptr<RedisClientImpl> spimpl;
        boost::posix_time::time_duration connectTimeout;
        boost::posix_time::time_duration commandTimeout;

        bool tcpNoDelay;
        bool tcpKeepAlive;
    
    };

}


#ifdef REDIS_CLIENT_HEADER_ONLY
    #include "RedisSyncClient.h"
#endif



#endif /* REDISCLIENT_REDISYNCCLIENT_H */