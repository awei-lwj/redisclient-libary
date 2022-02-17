#include "RedisSyncClient.h"
#include "PipeLine.h"
#include "ThrowError.h"

#include <memory>
#include <functional>

namespace redisclient
{
    RedisSyncClient::RedisSyncClient(boost::asio::io_service &ioService)
        : spimpl(std::make_shared<RedisClientImpl>(ioService)),
          connectTimeout(boost::posix_time::hours(365 * 24)),
          commandTimeout(boost::posix_time::hours(365 * 24)),
          tcpNoDelay(true),
          tcpKeepAlive(false)
    {
        spimpl->errorHandler = std::bind(&RedisClientImpl::defaultErrorHandler, std::placeholders::_1);
    }

    RedisSyncClient::RedisSyncClient(RedisSyncClient &&other)
        : spimpl(std::move(other.spimpl)),
          connectTimeout(std::move(other.connectTimeout)),
          commandTimeout(std::move(other.commandTimeout)),
          tcpNoDelay(std::move(other.tcpNoDelay)),
          tcpKeepAlive(std::move(other.tcpKeepAlive))
    {
    }

    RedisSyncClient::~RedisSyncClient()
    {
        if (spimpl)
            spimpl->close();
    }

    void RedisSyncClient::connect(const boost::asio::ip::tcp::endpoint &endpoint)
    {
        boost::system::error_code ec;

        connect(endpoint, ec);
        detail::throwIfError(ec);
    }

    void RedisSyncClient::connect(
        const boost::asio::ip::tcp::endpoint &endpoint,
        boost::system::error_code &ec)
    {
        // Open Socket
        spimpl->socket.open(endpoint.protocol(), ec);

        // No problems
        if (!ec && tcpNoDelay)
        {
            spimpl->socket.set_option(boost::asio::ip::tcp::no_delay(true), ec);
        }

        // boost asio does not support `connect` with timeout
        int socket = spimpl->socket.native_handle();
        struct sockaddr_in addr;

        addr.sin_family = AF_INET;
        addr.sin_port = htons(endpoint.port());
        addr.sin_addr.s_addr = inet_addr(endpoint.address().to_string().c_str());

        // Set non-blocking
        int arg = 0;
        if ((arg = fcntl(socket, F_GETFL, NULL)) < 0)
        {
            ec = boost::system::error_code(
                errno,
                boost::asio::error::get_system_category());
        }

        arg |= O_NONBLOCK;

        if ((arg = fcntl(socket, F_GETFL, NULL)) < 0)
        {
            ec = boost::system::error_code(
                errno,
                boost::asio::error::get_system_category());
        }

        // Create connection
        int result = ::connect(socket, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
        if (result < 0)
        {
            if (errno == EINPROGRESS)
            {
                for (;;)
                {
                    // Poll Selecting
                    pollfd pfd;
                    pfd.fd = socket;
                    pfd.events = POLLOUT;

                    result = ::poll(&pfd, 1, connectTimeout.total_microseconds());

                    if (result < 0)
                    {
                        if (errno == EINTR)
                        {
                            // Try to connect Redis Server again
                            continue;
                        }
                        else
                        {
                            ec = boost::system::error_code(
                                errno,
                                boost::asio::error::get_system_category());
                            return;
                        }
                    }
                    else if (result > 0)
                    {
                        // Checking for redis client error
                        int valopt;
                        socklen_t optlen = sizeof(valopt);

                        if (getsockopt(socket, SOL_SOCKET, SO_ERROR,
                                       reinterpret_cast<void *>(&valopt), &optlen) < 0)
                        {
                            ec = boost::system::error_code(
                                errno,
                                boost::asio::error::get_system_category());
                            return;
                        }

                        if (valopt)
                        {
                            ec = boost::system::error_code(
                                errno,
                                boost::asio::error::get_system_category());
                            return;
                        }
                    }
                    else
                    {
                        // Timeout
                        ec = boost::system::error_code(
                            errno,
                            boost::asio::error::get_system_category());
                        return;
                    } // end if

                } // end for
            }
            else
            {
                ec = boost::system::error_code(
                    errno,
                    boost::asio::error::get_system_category());
                return;
            } // end if errno == EINPROGRESS

        } // end if result < 0

        if ((arg = fcntl(socket, F_GETFL, NULL)) < 0)
        {
            ec = boost::system::error_code(
                errno,
                boost::asio::error::get_system_category());
            return;
        }

        arg &= (~O_NONBLOCK);

        if (fcntl(socket, F_SETFL, arg) < 0)
        {
            ec = boost::system::error_code(
                errno,
                boost::asio::error::get_system_category());
            return;
        }

        if (!ec)
        {
            spimpl->state = State::Connected;
        }
    }

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS
    void RedisSyncClient::connect(const boost::asio::ip::tcp::endpoint &endpoint)
    {
        boost::system::error_code ec;

        connect(endpoint, ec);
        detail::throwIfError;
    }

    void RedisSyncClient::connect(
        const boost::asio::ip::tcp::endpoint &endpoint,
        boost::system::error_code &ec)
    {
        spimpl->socket.open(endpoint.protocol(), ec);

        if (!ec)
        {
            spimpl->socket.connect(endpoint, ec);
        }

        if (!ec)
        {
            spimpl->state = State::Connected;
        }
    }
#endif

    bool RedisSyncClient::isConnected() const
    {
        return spimpl->getState() == State::Connected ||
               spimpl->getState() == State::Subscribed;
    }

    void RedisSyncClient::disConnected()
    {
        spimpl->close();
    }

    void RedisSyncClient::installErrorHandler(
        std::function<void(const std::string &)> handler)
    {
        spimpl->errorHandler = std::move(handler);
    }

    RedisValue RedisSyncClient::command(
        std::string cmd,
        std::deque<RedisBuffer> args)
    {
        boost::system::error_code ec;
        RedisValue result = command(std::move(cmd), std::move(args), ec);

        detail::throwIfError(ec);
        return result;
    }

    RedisValue RedisSyncClient::command(
        std::string cmd,
        std::deque<RedisBuffer> args,
        boost::system::error_code &ec)
    {
        if (statementValid())
        {
            args.push_front(std::move(cmd));

            return spimpl->doSyncCommand(args, commandTimeout, ec);
        }
        else
        {
            return RedisValue();
        }
    }

    Pipeline RedisSyncClient::pipelined()
    {
        Pipeline pipe(*this);
        return pipe;
    }

    RedisValue RedisSyncClient::pipelined(std::deque<std::deque<RedisBuffer>> commands)
    {
        boost::system::error_code ec;
        RedisValue result = pipelined(std::move(commands), ec);

        detail::throwIfError(ec);
        return result;
    }

    RedisValue RedisSyncClient::pipelined(
        std::deque<std::deque<RedisBuffer>> commands,
        boost::system::error_code &ec)
    {
        if (statementValid())
        {
            return spimpl->doSyncCommand(commands, commandTimeout, ec);
        }
        else
        {
            return RedisValue();
        }
    }

    RedisSyncClient::State RedisSyncClient::state() const
    {
        return spimpl->getState();
    }

    bool RedisSyncClient::statementValid() const
    {
        assert(state() == State::Connected);

        if (state() != State::Connected)
        {
            std::stringstream ss;

            ss << "RedisClient::command called with invalid state "
               << to_string(state());

            spimpl->errorHandler(ss.str());
            return false;
        }

        return true;
    }

    RedisSyncClient &RedisSyncClient::setConnectTimeout(
        const boost::posix_time::time_duration &timeout)
    {
        connectTimeout = timeout;
        return *this;
    }

    RedisSyncClient &RedisSyncClient::setCommandTimeout(
        const boost::posix_time::time_duration &timeout)
    {
        commandTimeout = timeout;
        return *this;
    }

    RedisSyncClient &RedisSyncClient::setTcpNoDelay(bool enable)
    {
        tcpNoDelay = enable;
        return *this;
    }

    RedisSyncClient &RedisSyncClient::setTcpKeepAlive(bool enable)
    {
        tcpKeepAlive = enable;
        return *this;
    }

}