#include <boost/date_time.hpp>
#include <boost/xpressive/xpressive.hpp>

#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonpath/json_query.hpp>

#include "modules/songrequest/youtube_api.h"
#include "http_client.h"
#include "modules/songrequest/song.h"
#include "tokens.h"

namespace ashbot {
namespace modules {
namespace songrequest {

namespace bx = boost::xpressive;

namespace jc = jsoncons;
namespace jp = jc::jsonpath;

namespace {

bool contains_value(const jc::json::array& array, const char* pValue)
{
    if (array.size() == 0) return false;

    auto found = std::find_if(array.begin(), array.end(),
        [pValue](const jc::json& v)
    {
        return _stricmp(v.as_string().c_str(), pValue) == 0;
    });

    return found != array.end();
}

duration parse_duration(const char* pDuration)
{
    static const bx::cregex VideoLength = "PT" >> !((bx::s1 = +bx::_d) >> 'H') >> !((bx::s2 = +bx::_d) >> 'M') >> (bx::s3 = +bx::_d) >> 'S';

    duration duration = {};

    bx::cmatch match;
    if (!regex_match(pDuration, match, VideoLength)) return duration;

    int seconds = strtol(match[3].first, nullptr, 10);
    duration += std::chrono::seconds(seconds);

    if (*match[2].first != 'P')
    {
        int minutes = strtol(match[2].first, nullptr, 10);
        duration += std::chrono::minutes(minutes);
    }

    if (*match[1].first != 'P')
    {
        int hours = strtol(match[1].first, nullptr, 10);
        duration += std::chrono::hours(hours);
    }

    return duration;
}
}

bool youtube::retrieve_video_data(song& songData, const char* streamerCountry)
{
    static constexpr char Url[] = "https://www.googleapis.com/youtube/v3/videos?id=%s&key=%s&prettyPrint=false"
                                  "&part=snippet,contentDetails,status"
                                  "&fields=pageInfo/totalResults,items/snippet(title,publishedAt),"
                                  "items/contentDetails(duration,regionRestriction),items/status/embeddable";

    char actualUrl[512];
    snprintf(actualUrl, 512, Url, songData.TrackId.c_str(), tokens::youtube());

    http_client http;
    http_response response = http.send_request(actualUrl, true);

    if (!response.has_error()) return parse_json(response.data(), songData, streamerCountry);

    return true;
}

bool youtube::parse_json(std::string& json, song& songData, const char* streamerCountry)
{
    jc::json ytData = jc::json::parse(json);

    jc::json totalResult = jp::json_query(ytData, "$.pageInfo.totalResults");
    if (totalResult.size() == 0 || totalResult[0].as_integer() == 0)
    {
        songData.Error = song::error::does_not_exist;
        return false;
    }

    jc::json item = jp::json_query(ytData, "$.items[0]")[0];
    const jc::json& snippet = item["snippet"];

    songData.parse_iso8601_created_date(snippet["publishedAt"].as_string());
    
    const jc::json& title = snippet["title"];
    songData.Name.assign(snippet["title"].as_string());

    const jc::json& contentDetails = item["contentDetails"];
    songData.Length = parse_duration(contentDetails["duration"].as_string().c_str());

    auto regionRestriction = contentDetails.find("regionRestriction");
    if (regionRestriction != contentDetails.object_range().end())
    {
        const jc::json& rr = regionRestriction->value();

        auto allowed = rr.find("allowed");
        if (allowed != contentDetails.object_range().end())
        {
            if (!contains_value(allowed->value().array_value(), streamerCountry))
            {
                songData.Error = song::error::region_restricted;
                return false;
            }
        }

        auto blocked = rr.find("blocked");
        if (blocked != contentDetails.object_range().end())
        {
            if (contains_value(blocked->value().array_value(), streamerCountry))
            {
                songData.Error = song::error::region_restricted;
                return false;
            }
        }
    }

    if (!jp::json_query(item, "$.status.embeddable").as_bool())
    {
        songData.Error = song::error::not_embeddable;
        return false;
    }

    songData.Link.clear();
    songData.Error = song::error::none;
    return true;
}

}
}
}
