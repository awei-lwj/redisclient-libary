#pragma once

#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

// For throwError
namespace redisclient
{
    namespace detail
    {
        inline void throwError(const boost::system::error_code &errorCode)
        {
            boost::system::system_error error(errorCode);
            throw error;
        }

        inline void throwIfError(const boost::system::error_code &errorCode)
        {
            if (errorCode)
            {
                throwError(errorCode);
            }
        }

    }

}