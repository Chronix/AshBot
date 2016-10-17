#include <boost/xpressive/xpressive.hpp>

#include <jsoncons/json.hpp>

#include "modules/songrequest/soundcloud_api.h"
#include "http_client.h"
#include "modules/songrequest/song.h"
#include "tokens.h"

namespace ashbot {
namespace modules {
namespace songrequest {

namespace bx = boost::xpressive;

namespace jc = jsoncons;

bool soundcloud::retrieve_song_data(song& songData)
{
    static constexpr char Url[] = "https://api.soundcloud.com/resolve.json?url=%s&client_id=%s";

    char actualUrl[512];
    snprintf(actualUrl, 512, Url, songData.Link.c_str(), tokens::soundcloud());

    http_client client;
    client.add_header("Referer", "https://www.ashwini.tv");
    http_response response = client.send_request(actualUrl, true, false);

    if (response.has_error())
    {
        if (response.status_code() == 404) songData.Error = song::error::does_not_exist;
        else songData.Error = song::error::host_error;
        return false;
    }

    if (response.redirect_url().find("/tracks/") == std::string::npos)
    {
        songData.Error = song::error::not_a_track;
        return false;
    }

    response = client.send_request(response.redirect_url().c_str(), true);
    if (response.has_error())
    {
        songData.Error = song::error::host_error;
        return false;
    }

    return parse_json(response.data(), songData);
}

std::string soundcloud::strip_soundcloud_link(const std::string& link)
{
    static const bx::sregex rx = "http" >> -*bx::as_xpr('s') >> "://soundcloud.com/";
    return regex_replace(link, rx, "");
}

bool soundcloud::parse_json(std::string& json, song& song)
{
    jc::json jsonObject = jc::json::parse(json);

    if (jsonObject["embeddable_by"].as_string() != "all")
    {
        song.Error = song::error::not_embeddable;
        return false;
    }

    song.Name.assign(jsonObject["title"].as_string());
    song.Link.assign(strip_soundcloud_link(jsonObject["permalink_url"].as_string()));
    song.Length = std::chrono::seconds(jsonObject["duration"].as_integer());

    song.Error = song::error::none;

    return true;
}
}
}
}
