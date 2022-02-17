#include "ThrowError.h"
#include "RedisAsyncClient.h"

#include <memory>
#include <functional>

namespace redisclient
{
    RedisAsyncClient::RedisAsyncClient(boost::asio::io_service &ioService)
        : spimpl(std::make_shared<RedisClientImpl>(ioService))
    {
        spimpl->errorHandler = std::bind(&RedisClientImpl::defaultErrorHandler,std::placeholders::_1);
    }

    RedisAsyncClient::~RedisAsyncClient()
    {
        spimpl->close();
    }

    void RedisAsyncClient::connect(
        const boost::asio::local::stream_protocol::endpoint &endpoint,
        std::function<void(boost::system::error_code)> handler        
    )
    {
        if ( spimpl->state == State::Closed )
        {
            spimpl->redisParser = RedisParser();
            std::move(spimpl->socket);
        }

        if( spimpl->state == State::Unconnected || spimpl->state == State::Closed)
        {
            spimpl->state = State::Connecting;
            spimpl->socket.async_connect(endpoint,
                std::bind(&RedisClientImpl::handleAsyncConnect,spimpl,
                std::placeholders::_1, std::move(handler))
            );
        }
        else
        {
            handler(boost::system::error_code());    
        }
    }

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS
    void RedisAsyncClient::connect(
        const boost::asio::local::stream_protocol::endpoint &endpoint,
            std::function<void(boost::system::error_code)> handler
    )
    {
        if( spimpl->state == State::Unconnected || spimpl->state == State::Closed)
        {
            spimpl->state = State::Connecting;
            spimpl->socket.async_connect(endpoint,
                std::bind(&RedisClientImpl::handleAsyncConnect,spimpl,
                std::placeholders::_1, std::move(handler))
            );
        }
        else
        {
            handler(boost::system::error_code());    
        }
    }
#endif
    
    bool RedisAsyncClient::isConnected() const
    {
        return spimpl->getState() == State::Connected || spimpl->getState() == State::Subscribed;
    }

    void RedisAsyncClient::disconnect()
    {
        spimpl->close();
    }

    void RedisAsyncClient::installErrorHandler(std::function<void(const std::string &)> handler)
    {
        spimpl->errorHandler = std::move(handler);
    }

    void RedisAsyncClient::command(
        const std::string &cmd,
        std::deque<RedisBuffer> args,
        std::function<void(RedisValue)> handler
    )
    {
        if(statementValid())
        {
            args.emplace_front(cmd);

            spimpl->post(
                std::bind(&RedisClientImpl::doAsyncCommand, spimpl,
                spimpl->makeCommand(args),std::move(handler))
            );
        }
    }

    RedisAsyncClient::Handle RedisAsyncClient::subscribe(
        const std::string &channelName,
        std::function<void(std::vector<char> msg)> msgHandler,
        std::function<void(RedisValue)> handler
    )
    {
        auto handleID = spimpl->subscribe("subscribe",channelName,msgHandler,handler);
        return { handleID, channelName};
    }

    RedisAsyncClient::Handle RedisAsyncClient::psubscribe(
        const std::string &pattern,
        std::function<void(std::vector<char> msg)> msgHandler,
        std::function<void(RedisValue)> handler
    )
    {
        auto handleId = spimpl->subscribe("psubscribe", pattern, msgHandler, handler);
        return{ handleId , pattern };
    }

    void RedisAsyncClient::unsubscribe(const Handle &handle)
    {
        spimpl->unsubscribe("unsubscribe", handle.id, handle.channel, dummyHandler);
    }

    void RedisAsyncClient::punsubscribe(const Handle &handle)
    {
        spimpl->unsubscribe("punsubscribe", handle.id, handle.channel, dummyHandler);
    }

    void RedisAsyncClient::singleShotSubscribe(
        const std::string &channel,
        std::function<void(std::vector<char> msg)> msgHandler,
        std::function<void(RedisValue)> handler
    )
    {
        spimpl->singleShotSubscribe("subscribe", channel, msgHandler, handler);
    }

    void RedisAsyncClient::singleShotPSubscribe(
        const std::string &pattern,
        std::function<void(std::vector<char> msg)> msgHandler,
        std::function<void(RedisValue)> handler
    )
    {
        spimpl->singleShotSubscribe("psubscribe", pattern, msgHandler, handler);
    }

    void RedisAsyncClient::publish(
        const std::string &channel, 
        const RedisBuffer &msg,
        std::function<void(RedisValue)> handler
    )
    {
        assert( spimpl->state == State::Connected );

        static const std::string publishStr = "PUBLISH";

        if( spimpl->state == State::Connected )
        {
            std::deque<RedisBuffer> items(3);

            items[0] = publishStr;
            items[1] = channel;
            items[2] = msg;

            spimpl->post(std::bind(&RedisClientImpl::doAsyncCommand, spimpl,
                    spimpl->makeCommand(items), std::move(handler)));
        }
        else
        {
            std::stringstream ss;

            ss << "RedisAsyncClient::command called with invalid state " << to_string(spimpl->state);

            spimpl->errorHandler(ss.str());
        }

    }

    RedisAsyncClient::State RedisAsyncClient::state() const
    {
        return spimpl->getState();
    }

    bool RedisAsyncClient::statementValid() const
    {
        assert( spimpl->state == State::Connected );

        if( spimpl->state != State::Connected )
        {
            std::stringstream ss;

            ss << "RedisAsyncClient::command called with invalid state " << to_string(spimpl->state);

            spimpl->errorHandler(ss.str());
            return false;
        }

        return true;
    }

}