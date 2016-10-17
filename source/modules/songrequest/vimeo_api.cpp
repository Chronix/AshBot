#include <jsoncons/json.hpp>

#include "modules/songrequest/vimeo_api.h"
#include "http_client.h"
#include "modules/songrequest/song.h"
#include "tokens.h"

namespace ashbot {
namespace modules {
namespace songrequest {

namespace jc = jsoncons;

bool vimeo::retrieve_video_data(song& songData)
{
    static constexpr char VimeoReferer[] = "Ashwini.tv/WiniBot v3";

    http_client client;
    client.add_header("Referer", VimeoReferer);

    std::string auth("bearer ");
    auth.append(tokens::vimeo());
    client.add_header("Authorization", auth.c_str());

    std::string url("https://api.vimeo.com/videos/");
    url.append(songData.TrackId);

    http_response response = client.send_request(url.c_str(), true);
    if (response.has_error())
    {
        if (response.status_code() == 404) songData.Error = song::error::does_not_exist;
        else songData.Error = song::error::host_error;
        return false;
    }

    return parse_json(response.data(), songData);
}

bool vimeo::parse_json(std::string& json, song& song)
{
    jc::json jsonObject = jc::json::parse(json);

    song.Name.assign(jsonObject["name"].as_string());
    song.Length = std::chrono::seconds(jsonObject["duration"].as_integer());
    song.parse_iso8601_created_date(jsonObject["created_time"].as_string());
    song.Link.clear();
    song.Error = song::error::none;

    return true;
}
}
}
}
