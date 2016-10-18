#pragma once

#include <string>

struct curl_slist;

namespace ashbot {

class http_response
{
    friend class http_client;
public:
    http_response() : error_(false), statusCode_(0) {}
public:
    bool                        has_error() const { return error_; }
    std::string&                data() { return data_; }
    long                        status_code() const { return statusCode_; }
    const std::string&          redirect_url() const { return redirectUrl_; }
private:
    bool                        error_;
    long                        statusCode_;
    std::string                 data_;
    std::string                 redirectUrl_;
};

class http_client
{
    using curl_handle = void*;
public:
    explicit                    http_client();
                                ~http_client();
public:
    void                        add_header(const char* name, const char* value);
    http_response               send_request(const char* url, bool https, bool followRedirects = true);
private:
    void                        build_request(http_response& res, const char* url, bool https, bool followRedirects);
private:   
    curl_handle                 hCurl_;
    curl_slist*                 curlHeaders_;
};

class twitch_http_client : public http_client
{
public:
    void                        add_twitch_std_headers();
    void                        add_twitch_auth_headers();
};

}
