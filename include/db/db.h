#pragma once

#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/smart_ptr/shared_array.hpp>

#include <libpq/libpq-fe.h>

#include "db_types.h"
#include "string_pool.h"

#define ASHBOT_PARAM_BUF_LEN    32

#pragma warning(push)
// conversion from x to y, possible loss of data
// 'x': forcing value to bool 'true' or 'false' (performance warning)
#pragma warning(disable: 4244 4800)

namespace ashbot {

namespace bf = boost::fusion;

class db_result
{
    using pgresult_ptr = std::shared_ptr<PGresult>;
public:
    db_result(PGresult* pRes = nullptr)
        :   pRes_(create_result_ptr(pRes)),
            rowCount_(pRes ? PQntuples(pRes) : 0)
    {}

    bool                    is_ok() const
    {
        ExecStatusType status = PQresultStatus(pRes_.get());
        return status == PGRES_TUPLES_OK || status == PGRES_COMMAND_OK;
    }

    explicit                operator bool() const { return is_ok(); }

    const char*             error_message() const { return PQresultErrorMessage(pRes_.get()); }

    int                     row_count() const { return rowCount_; }
    bool                    empty() const { return row_count() == 0; }

    bool                    read_column(char* pResultBuffer, int row, int column) const
    {
        char* pData = PQgetvalue(pRes_.get(), row, column);
        int dataSize = PQgetlength(pRes_.get(), row, column);
        int toCopy = std::min(dataSize, ASHBOT_STRING_BUFFER_SIZE - 1);

        if (!pData) return false;
        memcpy(pResultBuffer, pData, toCopy);
        pResultBuffer[toCopy] = 0;
        
        return true;
    }

    template<typename _T>
    std::enable_if_t<std::is_integral<_T>::value, bool>
                            read_column(_T& result, int row, int column) const
    {
        _T* pMem = reinterpret_cast<_T*>(PQgetvalue(pRes_.get(), row, column));
        if (!pMem) return false;

        switch (sizeof(_T))
        {
        case 8: result = ntohll(*pMem); break;
        case 4: result = ntohl(*pMem); break;
        case 2: result = ntohs(*pMem); break;
        default: result = *pMem; break;
        }

        return true;
    }

    bool is_field_null(int row, int column) const
    {
        return PQgetisnull(pRes_.get(), row, column);
    }
private:
    static void delete_result(PGresult* pRes)
    {
        PQclear(pRes);
    }

    static pgresult_ptr create_result_ptr(PGresult* pRes)
    {
        return pgresult_ptr(pRes, delete_result);
    }
private:
    pgresult_ptr            pRes_;
    int                     rowCount_;
};

class db
{
public:
    using user_id = int64_t;
public:
                            ~db();
private:
                            db();
public:
    template<typename... _Args>
    db_result               query(const char* commandText, _Args&... args);

    static const char*      strptr(const char* ps) { return ps; }

    user_id                 get_user(const char* pUsername);

    static db&              get()
    {
        static db db_;
        return db_;
    }

    static constexpr user_id InvalidUserId = -1;
private:
    PGconn*                 get_connection();
    void                    release_connection(PGconn* pc);
private:
    cc_queue<PGconn*>       connectionPool_;
};

namespace detail {
template<typename _T, typename _Enable = void>
struct fill_params_core;
}

struct pgsql_array_base
{
};

template<typename _T>
struct pgsql_array : pgsql_array_base
{
    template<typename _Ty, typename _Enable>
    friend struct detail::fill_params_core;
public:
    pgsql_array(_T* data, size_t count)
    :   data_(data),
        count_(count),
        dbDataLength_(0),
        dbDataInit_(false) {}
    ~pgsql_array() {}
public:
    const _T& operator[](size_t index) const
    {
        return data_[index];
    }

    size_t count() const { return count_; }
private:
    _T*                             data_;
    size_t                          count_;
    boost::shared_array<uint8_t>    dbData_;
    size_t                          dbDataLength_;
    bool                            dbDataInit_;
};

template<typename CharT, typename TraitsT>
std::basic_ostream<CharT, TraitsT>& operator<<(std::basic_ostream<CharT, TraitsT>& strm, const pgsql_array_base& arr)
{
    strm << "<array>";
    return strm;
}

namespace detail {

struct cmd_error_helper
{
    explicit cmd_error_helper(boost::log::record_ostream& logOss)
        : logOss_(logOss)
    {}

    template<typename _T>
    void operator()(_T& t) const
    {
        logOss_ << t << " ";
    }

    boost::log::record_ostream& logOss_;
};

template<typename _BFVector>
void log_command_error(const char* errorMessage, const char* commandText, _BFVector& commandParams)
{
    using namespace boost::log;

    record record = __logger::get().open_record(keywords::severity = trivial::error);
    if (record)
    {
        record_ostream oss(record);
        oss << "Error executing SQL command '" << commandText << "' with params [ ";
        bf::for_each(commandParams, cmd_error_helper(oss));
        oss << " ]; error message: " << errorMessage;
        oss.flush();
        __logger::get().push_record(std::move(record));
    }
}

template<>
struct fill_params_core<const char*, void>
{
    void operator()(const char*& pBuf, const char* pVal, int& length) const
    {
        pBuf = pVal;
        length = static_cast<int>(strlen(pVal));
    }
};

template<typename _T>
struct fill_params_core<_T, std::enable_if_t<std::is_integral<_T>::value>>
{
    void operator()(const char*& pBuf, _T& obj, int& length) const
    {
        switch (sizeof(_T))
        {
        case 8: obj = htonll(obj); break;
        case 4: obj = htonl(obj); break;
        case 2: obj = htons(obj); break;
        }

        length = sizeof(_T);
        pBuf = reinterpret_cast<const char*>(&obj);
    }
};

template<>
struct fill_params_core<pgsql_array<const std::string*>, void>
{
    void operator()(const char*& pBuf, pgsql_array<const std::string*>& array, int& length) const
    {
        if (!array.dbDataInit_)
        {
            // int32 flags:
            // dimension count + has null + element oid + count + start index
            size_t binSize = 4 + 4 + 4 + 4 + 4; // each integer is 4 bytes
            for (size_t i = 0; i < array.count(); ++i)
            {
                binSize += 4; // element size
                binSize += array[i]->length();
            }

            array.dbData_.reset(new uint8_t[binSize]);

            uint32_t* pUiData = reinterpret_cast<uint32_t*>(array.dbData_.get());
            *pUiData++ = htonl(1); // dimension count
            *pUiData++ = 0; // no NULLs in the array
            *pUiData++ = htonl(id::OID_VARCHAR); // array type OID
            *pUiData++ = htonl(static_cast<uint32_t>(array.count()));  // count of elements
            *pUiData++ = htonl(1); // index of first element (not really sure what this even is tbh)

            uint8_t* pArrayData = reinterpret_cast<uint8_t*>(pUiData);
            for (size_t i = 0; i < array.count(); ++i)
            {
                *reinterpret_cast<uint32_t*>(pArrayData) = htonl(static_cast<uint32_t>(array[i]->length()));
                pArrayData += 4;
                memcpy(pArrayData, array[i]->data(), array[i]->size());
                pArrayData += array[i]->size();
            }

            array.dbDataLength_ = binSize;
            array.dbDataInit_ = true;
        }

        pBuf = reinterpret_cast<const char*>(array.dbData_.get());
        length = static_cast<int>(array.dbDataLength_);
    }
};

struct fill_params
{
    fill_params(size_t& counter, const char** ppValues, int* pParamLengths)
        :   i(counter),
            pValues(ppValues),
            pLengths(pParamLengths)
    {}

    template<typename _T>
    void operator()(const _T& obj) const
    {
        fill_params_core<_T>()(pValues[i], const_cast<_T&>(obj), const_cast<int&>(pLengths[i]));
        ++i;
    }

    size_t&             i;
    const char**        pValues;
    int*                pLengths;
};

template<typename... _Args>
db_result query(PGconn* pc, const char* commandText, _Args&... args)
{
    constexpr size_t ArgCount = sizeof...(args);
    bf::vector<_Args...> params(args...);

    const char* paramValues[ArgCount];
    int paramLengths[ArgCount], paramFormats[ArgCount];

    std::fill_n(paramFormats, ArgCount, 1);

    size_t counter = 0;
    bf::for_each(params, fill_params(counter, paramValues, paramLengths));

    db_result dbRes(PQexecParams(pc, commandText, ArgCount, nullptr, paramValues,
                                 paramLengths, paramFormats, 1));

    if (!dbRes)
    {
        log_command_error(dbRes.error_message(), commandText, params);
    }

    return dbRes;
}

inline db_result query(PGconn* pc, const char* commandText)
{
    // cannot simply use PQexec here because that returns data in text format
    db_result dbRes(PQexecParams(pc, commandText, 0, nullptr, nullptr, nullptr, nullptr, 1));

    if (!dbRes)
    {
        AshBotLogError << "Error executing SQL command '"
                       << commandText << "', error message: "
                       << dbRes.error_message();
    }

    return dbRes;
}

}

template<typename... _Args>
db_result db::query(const char* commandText, _Args&... args)
{
    PGconn* pc = get_connection();
    db_result dbRes = detail::query(pc, commandText, args...);
    release_connection(pc);
    return dbRes;
}

}

#pragma warning(pop)
