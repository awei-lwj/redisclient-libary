#ifndef REDISCLIENT_REDISVALUE_H
#define REDISCLIENT_REDISVALUE_H

#include "Config.h"

#include <boost/variant.hpp>
#include <string>
#include <vector>

namespace redisclient
{
    class RedisValue
    {
    public:
        struct ErrorTag
        {
        };

        REDIS_CLIENT_DECL RedisValue();
        REDIS_CLIENT_DECL RedisValue(RedisValue &&other);
        REDIS_CLIENT_DECL RedisValue(int64_t temp);
        REDIS_CLIENT_DECL RedisValue(const char *s);

        REDIS_CLIENT_DECL RedisValue(const std::string &s);
        REDIS_CLIENT_DECL RedisValue(std::vector<char> buf);
        REDIS_CLIENT_DECL RedisValue(std::vector<char> buf, struct ErrorTag);
        REDIS_CLIENT_DECL RedisValue(std::vector<RedisValue> array);

        RedisValue(const RedisValue &) = default;
        RedisValue &operator=(const RedisValue &) = default;
        RedisValue &operator=(RedisValue &) = default;

        // Return string
        REDIS_CLIENT_DECL std::string toString() const;

        // Return vector<char>
        REDIS_CLIENT_DECL std::vector<char> toByteArray() const;

        // Return vector<RedisValue>
        REDIS_CLIENT_DECL int64_t toInt64() const;

        // Return Redisvalue array
        REDIS_CLIENT_DECL std::vector<RedisValue> toArray() const;

        // Return the string representation of the value
        REDIS_CLIENT_DECL std::string inspect() const;

        // Now begin to check the state
        // Return true if value not a error
        REDIS_CLIENT_DECL bool isOk() const;
        // Return true if value is a error
        REDIS_CLIENT_DECL bool isError() const;

        // Return true if this is a null.
        REDIS_CLIENT_DECL bool isNull() const;
        // Return true if type is an int
        REDIS_CLIENT_DECL bool isInt() const;
        // Return true if type is an array
        REDIS_CLIENT_DECL bool isArray() const;
        // Return true if type is a string/byte array. Alias for isString();
        REDIS_CLIENT_DECL bool isByteArray() const;
        // Return true if type is a string/byte array. Alias for isByteArray().
        REDIS_CLIENT_DECL bool isString() const;

        // Methods for increasing performance
        // Throws: boost::bad_get if the type does not match
        REDIS_CLIENT_DECL std::vector<char> &getByteArray();
        REDIS_CLIENT_DECL const std::vector<char> &getByteArray() const;
        REDIS_CLIENT_DECL std::vector<RedisValue> &getArray();
        REDIS_CLIENT_DECL const std::vector<RedisValue> &getArray() const;

        REDIS_CLIENT_DECL bool operator==(const RedisValue &rhs) const;
        REDIS_CLIENT_DECL bool operator!=(const RedisValue &rhs) const;

    protected:
        template<typename T>
        T castTo() const;

        template<typename T>
        bool typeEqual() const;

    private:
        struct NullTag
        {
            inline bool operator == (const NullTag&) const
            {
                return true;
            }
        };


        boost::variant<NullTag,int64_t,std::vector<char>,std::vector<RedisValue>> value;
        bool error;
    };

    template<typename T>
    T RedisValue::castTo() const
    {
        if( value.type() == typeid(T) )
            return boost::get<T>( value );
        else
            return T();
    }

    template<typename T>
    bool RedisValue::typeEqual() const
    {
        if ( value == typeid(T) )
            return true;
        else
            return false;
    }
};

#ifndef REDIS_CLIENT_HEADER_ONLY
    #include "redisclient/RedisValue.cpp"
#endif


#endif /* REDISCLIENT_REDISVALUE_H */