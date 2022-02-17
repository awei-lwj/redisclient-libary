#include "RedisParser.h"

#include <assert.h>
#include <sstream>

#ifndef DEBUG_REDIS_PARSER
    #include<iostream>
#endif

namespace redisclient
{
    RedisParser::RedisParser()
        : bulkSize(0)
    {
        buf.reserve(64);
    }

    std::pair<size_t, RedisParser::ParseResult> RedisParser::parse(const char *ptr, size_t size)
    {
        return RedisParser::parseChunk(ptr, size);
    }

    // For parse chunk judgement
    std::pair<size_t, RedisParser::ParseResult> RedisParser::parseChunk(const char *ptr, size_t size)
    {
        size_t pos = 0;
        Statement state = Start;

        if(!states.empty())
        {
            state = states.top();
            states.pop();
        }

        while(pos < size)
        {
            char c = ptr[pos++];

        // For Debug
        #ifndef DEBUG_REDIS_PARSER
            std::cerr << "state: " << state << ", c: " << "\n";
        #endif 

            // Begin to judge the statement
            // enum Statement : 
            // Start,StartArray,String,StringLF,ErrorString,
            // ErrorLF,Integer,IntegerLF,BulkSize,BulkSizeLF,
            // Bulk,BulkCR,BulkCR,ArraySize,ArraySizeLF,

            switch(state)
            {
                // StartArray
                case StartArray:

                // Start
                case Start:
                    buf.clear();

                    // Begin to judge char c
                    switch(c)
                    {
                        case stringReply:
                            state = String;
                            break;
                        case errorReply:
                            state = ErrorString;
                            break;
                        case integerReply:
                            state = Integer;
                            break;
                        case bulkReply:
                            state = BulkSize;
                            break;
                        case arrayReply:
                            state = BulkSize;
                            bulkSize = 0;
                            break;
                        default:
                            return std::make_pair(pos,Error);
                    } // end switch
                    break;
                // end case

                // String
                case String:
                    if ( c == '\r')
                    {
                        state = StringLF;
                    }
                    else if ( isChar(c) && !isControlChar(c) )
                    {
                        buf.push_back(c);
                    }
                    else
                    {
                        std::stack<Statement>().swap(states);
                        return std::make_pair(pos,Error);
                    }

                    break;
                // end case

                // ErrorString
                case ErrorString:
                    if( c == '\r' )
                    {
                        state = ErrorLF;
                    }
                    else if( isChar(c) && !isControlChar(c) )
                    {
                        buf.push_back(c);
                    }
                    else
                    {
                        std::stack<Statement>().swap(states);
                        return std::make_pair(pos, Error);
                    }
                    break;
                // end case

                // BulkSize
                case BulkSize:
                    if( c == '\r')
                    {
                        if ( buf.empty() )
                        {
                            std::stack<Statement>().swap(states);
                            return std::make_pair(pos, Error);
                        }
                        else
                        {
                            state = BulkSizeLF;
                        }
                    }
                    else if( isdigit(c) || c == '-' )
                    {
                        buf.push_back(c);
                    }
                    else
                    {
                        std::stack<Statement>().swap(states);
                        return std::make_pair(pos, Error);
                    }
                    break;
                // end case

                // StringLF:
                case StringLF:
                    if ( c == '\n' )
                    {
                        state = Start;
                        redisValue = RedisValue(buf);
                    }
                    else
                    {
                        std::stack<Statement>().swap(states);
                        return std::make_pair(pos, Error);
                    }
                    break;
                // end case

                // ErrorLF
                case ErrorLF:
                    if ( c == '\n' )
                    {
                        state = Start;
                        RedisValue::ErrorTag tag;
                        redisValue = RedisValue(buf,tag);
                    }
                    else
                    {
                        std::stack<Statement>().swap(states);
                        return std::make_pair(pos, Error);
                    }
                    break;
                // end case

                // BulkSizeLF
                case BulkSizeLF:
                    if ( c == '\n' )
                    {
                        bulkSize = bufferToLong(buf.data(),buf.size());
                        buf.clear();

                        if( bulkSize == -1 )
                        {
                            state = Start;
                            redisValue = RedisValue(); // Nil
                        }
                        else if( bulkSize == 0 )
                        {
                            state = BulkCR;
                        }
                        else if( bulkSize < 0 )
                        {
                            std::stack<Statement>().swap(states);
                            return std::make_pair(pos, Error);
                        }
                        else
                        {
                            buf.reserve(bulkSize);

                            long int available = size - pos;
                            long int canRead = std::min(bulkSize, available);

                            if( canRead > 0 )
                            {
                                buf.assign(ptr + pos, ptr + pos + canRead);
                                pos += canRead;
                                bulkSize -= canRead;
                            }


                            if (bulkSize > 0)
                            {
                                state = Bulk;
                            }
                            else
                            {
                                state = BulkCR;
                            }

                        } // end if

                    }
                    else
                    {
                        std::stack<Statement>().swap(states);
                        return std::make_pair(pos, Error);
                    }   // end if
                    break;
                // end case

                // Bulk
                case Bulk:
                    assert( bulkSize > 0 );

                    long int available = size - pos + 1;
                    long int canRead   = std::min(available, bulkSize);

                    buf.insert(buf.end(),ptr + pos - 1,ptr + pos - 1 + canRead);
                    bulkSize -= canRead;
                    pos      += canRead - 1;

                    if( bulkSize == 0 )
                    {
                        state = BulkCR;
                    }

                    break;
                // end case

                // BulkCR
                case BulkCR:
                    if ( c == '\r')
                    {
                        state = BulkLF;
                    }
                    else
                    {
                        std::stack<Statement>().swap(states);
                        return std::make_pair(pos, Error);
                    }

                    break;
                // end case

                // BulkLF
                case BulkLF:
                    if ( c == '\n')
                    {
                        state = Start;
                        redisValue = RedisValue(buf);
                    }
                    else
                    {
                        std::stack<Statement>().swap(states);
                        return std::make_pair(pos, Error);
                    }
                    break;
                // end case

                // ArraySize
                case ArraySize:
                    if ( c == '\r' )
                    {
                        if( buf.empty() )
                        {
                            std::stack<Statement>().swap(states);
                            return std::make_pair(pos, Error);
                        }
                        else
                        {
                            state = ArraySizeLF;
                        }
                    }
                    else if ( isdigit(c) || c == '-' )
                    {
                        buf.push_back(c);
                    }
                    else
                    {
                        std::stack<Statement>().swap(states);
                        return std::make_pair(pos, Error);
                    }
                    break;
                // end case

                // ArraySizeLF
                case ArraySizeLF:
                    if ( c == '\n' )
                    {
                        int64_t arraySize = bufferToLong(buf.data(),buf.size());
                        std::vector<RedisValue> array;

                        if( arraySize == -1 )
                        {
                            state = Start;
                            redisValue = RedisValue();  // Nil value
                        }
                        else if( arraySize == 0 )
                        {
                            state = Start;
                            redisValue = RedisValue(std::move(array));  // Empty array
                        }
                        else if( arraySize < 0 )
                        {
                            std::stack<Statement>().swap(states);
                            return std::make_pair(pos, Error);
                        }
                        else
                        {
                            array.reserve(arraySize);
                            arraySizes.push(arraySize);
                            arrayValues.push(std::move(array));

                            state = StartArray;
                        }

                    }
                    else
                    {
                        std::stack<Statement>().swap(states);
                        return std::make_pair(pos, Error);
                    } // end if

                    break;
                // end case

                // Integer
                case Integer:
                    if ( c == '\r' )
                    {
                        if ( buf.empty() )
                        {
                            std::stack<Statement>().swap(states);
                            return std::make_pair(pos, Error);
                        }
                        else
                        {
                            state = IntegerLF;
                        }
                    }
                    else if( isdigit(c) || c == '-' )
                    {
                        buf.push_back(c);
                    }
                    else
                    {
                        std::stack<Statement>().swap(states);
                        return std::make_pair(pos, Error);
                    }
                    break;
                // end case

                // IntegerLF
                case IntegerLF:
                    if( c == '\n')
                    {
                        int64_t value = bufferToLong(buf.data(),buf.size());

                        buf.clear();
                        redisValue = RedisValue(value);
                        state = Start;
                    }
                    else
                    {
                        std::stack<Statement>().swap(states);
                        return std::make_pair(pos, Error);
                    }
                    break;
                // end case

                default:
                    std::stack<Statement>().swap(states);
                    return std::make_pair(pos, Error);
            } // end switch

            if( state == Start )
            {
                if( !arraySizes.empty() )
                {
                    assert(arraySizes.size() > 0);
                    arrayValues.top().getArray().push_back(redisValue);

                    while( !arraySizes.empty() && --arraySizes.top() == 0 )
                    {
                        arraySizes.pop();
                        redisValue = std::move(arrayValues.top());
                        arrayValues.pop();

                        if( !arraySizes.empty() )
                        {
                            arrayValues.top().getArray().push_back(redisValue);
                        }
                    }
                }

                if (arraySizes.empty())
                {
                    // done
                    break;
                }
            } // end if

        } // end while

        if( arraySizes.empty() && state == Start)
        {
            return std::make_pair(pos, Completed);
        }
        else
        {
            states.push(state);
            return std::make_pair(pos, Uncompleted);
        }
    }
    // end parseChunk

    RedisValue RedisParser::result()
    {
        return std::move(redisValue);
    }

    // Convert string to long
    long int RedisParser::bufferToLong(const char *str,size_t size)
    {
        long int value = 0;
        bool sign = false;

        if( str == nullptr || size == 0 )
        {
            return 0;
        }

        if( *str == '-' )
        {
            sign = true;
            ++str;
            --size;

            if( size == 0 ) 
            {
                return 0;
            }
        }

        for(const char *end = str + size; str != end; ++str)
        {
            char c = *str;

            // char must be valid
            assert(c >= '0' && c <= '9');

            value = value * 10;
            value += c - '0';
        }

        return sign ? -value : value;
    }

}