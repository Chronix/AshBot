#include <boost/date_time.hpp>
#include <boost/xpressive/xpressive.hpp>

#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonpath/json_query.hpp>

#include "modules/songrequest/youtube_api.h"
#include "http_client.h"
#include "tokens.h"

namespace ashbot {
namespace modules {
namespace songrequest {

namespace bx = boost::xpressive;

namespace jc = jsoncons;
namespace jp = jc::jsonpath;

namespace regex {

const bx::cregex VideoLength = "PT" >> !((bx::s1 = +bx::_d) >> 'H') >> !((bx::s2 = +bx::_d) >> 'M') >> (bx::s3 = +bx::_d) >> 'S';

}

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

}

constexpr char Url[] = "https://www.googleapis.com/youtube/v3/videos?id=%s&key=%s&prettyPrint=false"
                       "&part=snippet,contentDetails,status"
                       "&fields=pageInfo/totalResults,items/snippet(title,publishedAt),"
                       "items/contentDetails(duration,regionRestriction),items/status/embeddable";

bool youtube::retrieve_video_data(song& songData, const char* streamerCountry)
{
    char actualUrl[512];
    sprintf(actualUrl, Url, songData.track_id(), tokens::youtube());

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
        songData.error_ = song::error::does_not_exist;
        return false;
    }

    jc::json item = jp::json_query(ytData, "$.items[0]")[0];
    const jc::json& snippet = item["snippet"];

    const jc::json& publishedAt = snippet["publishedAt"];
    tm s_tm = {};
    std::istringstream iss(publishedAt.as_string());
    iss >> std::get_time(&s_tm, "%Y-%m-%dT%H:%M:%S");
    songData.createdAt_ = clock_type::from_time_t(mktime(&s_tm));

    const jc::json& title = snippet["title"];
    std::string name = title.as_string();
    name.copy(songData.name_, name.length());
    songData.name_[name.length()] = 0;

    const jc::json& contentDetails = item["contentDetails"];
    songData.length_ = parse_duration(contentDetails["duration"].as_string().c_str());

    auto regionRestriction = contentDetails.find("regionRestriction");
    if (regionRestriction != contentDetails.members().end())
    {
        const jc::json& rr = regionRestriction->value();

        auto allowed = rr.find("allowed");
        if (allowed != contentDetails.members().end())
        {
            if (!contains_value(allowed->value().array_value(), streamerCountry))
            {
                songData.error_ = song::error::region_restricted;
                return false;
            }
        }

        auto blocked = rr.find("blocked");
        if (blocked != contentDetails.members().end())
        {
            if (contains_value(blocked->value().array_value(), streamerCountry))
            {
                songData.error_ = song::error::region_restricted;
                return false;
            }
        }
    }

    if (!jp::json_query(item, "$.status.embeddable").as_bool())
    {
        songData.error_ = song::error::not_embeddable;
        return false;
    }

    songData.link_.clear();
    songData.error_ = song::error::none;
    return true;
}

song_duration youtube::parse_duration(const char* pDuration)
{
    song_duration duration = {};

    bx::cmatch match;
    if (!regex_match(pDuration, match, regex::VideoLength)) return duration;
    
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
}
}