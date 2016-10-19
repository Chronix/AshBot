#include <curl/curl.h>

#include "http_client.h"
#include "tokens.h"
#include "util.h"

namespace ashbot {

namespace {

size_t curl_write(char* buffer, size_t size, size_t nmemb, void* userdata)
{
    std::string* pStr = static_cast<std::string*>(userdata);
    size_t total = size * nmemb;
    pStr->append(buffer, total);
    return total;
}

}

http_client::http_client()
    :   hCurl_(curl_easy_init())
    ,   curlHeaders_(nullptr)
{
}

http_client::~http_client()
{
    if (curlHeaders_) curl_slist_free_all(curlHeaders_);
    curl_easy_cleanup(hCurl_);
}

void http_client::add_header(const char* name, const char* value)
{
    std::string header(name);
    header.append(": ");
    header.append(value);
    curlHeaders_ = curl_slist_append(curlHeaders_, header.c_str());
}

http_response http_client::send_request(const char* url, bool https, bool followRedirects)
{
    http_response response;

    build_request(response, url, https, followRedirects);

    CURLcode retCode = curl_easy_perform(hCurl_);
    if (retCode != CURLE_OK) response.error_ = true;

    curl_easy_getinfo(hCurl_, CURLINFO_RESPONSE_CODE, &response.statusCode_);

    char* redirectUrl = nullptr;
    curl_easy_getinfo(hCurl_, CURLINFO_REDIRECT_URL, &redirectUrl);
    if (redirectUrl) response.redirectUrl_.assign(redirectUrl);

    curl_easy_reset(hCurl_);

    return response;
}

void http_client::build_request(http_response& res, const char* url, bool https, bool followRedirects)
{
    curl_easy_setopt(hCurl_, CURLOPT_URL, url);
    
    if (https)
    {
        curl_easy_setopt(hCurl_, CURLOPT_USE_SSL, CURLUSESSL_ALL);
        curl_easy_setopt(hCurl_, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
    }
    
    if (!followRedirects) curl_easy_setopt(hCurl_, CURLOPT_FOLLOWLOCATION, 0);

    curl_easy_setopt(hCurl_, CURLOPT_WRITEFUNCTION, curl_write);
    curl_easy_setopt(hCurl_, CURLOPT_WRITEDATA, &res.data_);

    if (curlHeaders_) curl_easy_setopt(hCurl_, CURLOPT_HTTPHEADER, curlHeaders_);
}

void twitch_http_client::add_twitch_std_headers()
{
    add_header("Accept", "application/vnd.twitchtv.v3+json");
    add_header("Client-ID", tokens::twitch_client_id());
}

void twitch_http_client::add_twitch_auth_headers()
{
#   define ASHBOT_OAUTH "OAuth "

    add_twitch_std_headers();
    char auth[128] = ASHBOT_OAUTH;
    strncat(auth, tokens::twitch_user(), array_size(auth) - array_size(ASHBOT_OAUTH));
    // or (static_strlen() + 1), same thing                 ^^^^^^^^^^^
    add_header("Authorization", auth);
}
}
