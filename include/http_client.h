#pragma once

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
private:
    bool                        error_;
    long                        statusCode_;
    std::string                 data_;
};

class http_client
{
    using curl_handle = void*;
public:
    explicit                    http_client();
                                ~http_client();
public:
    void                        add_header(const char* name, const char* value);
    http_response               send_request(const char* url, bool https);
private:
    void                        build_request(http_response& res, const char* url, bool https);
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
