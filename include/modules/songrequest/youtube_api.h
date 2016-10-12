#pragma once

#include "song.h"

namespace ashbot {
namespace modules {
namespace songrequest {

class youtube
{
public:
    static bool             retrieve_video_data(song& songData, const char* streamerCountry);
private:
    static bool             parse_json(std::string& json, song& songData, const char* streamerCountry);
    static song_duration    parse_duration(const char* pDuration);
};

}
}
}
