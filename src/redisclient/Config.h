#ifndef REDISCLIENT_CONFIG_H
#define REDISCLIENT_CONFIG_H

// Default to a header-only complication
#ifndef REDIS_CLIENT_HEADER_ONLY
#    ifndef REDIS_CLIENT_SEPARATED_COMPILICATION
#       define REDIS_CLIENT_HEADER_ONLY
#    endif
#endif

#ifdef REDIS_CLIENT_HEADER_ONLY
#    define REDIS_CLIENT_DECL inline
#else
#    if defined(WIN32) && defined(REDIS_CLIENT_DYNLIB)
#        // Build to a Window dynamic library (DLL)
#        ifdef REDIS_CLIENT_BUILD
#            define REDIS_CLIENT_DECL __declspec(dllexport)
#        else
#            define REDIS_CLIENT_DECL __declspec(dllimport)
#        endif
#    endif
#endif

#ifndef REDIS_CLIENT_DECL
#    define REDIS_CLIENT_DECL
#endif


#endif // REDISCLIENT_CONFIG_H