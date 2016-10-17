#pragma once

namespace ashbot {
namespace modules {
namespace songrequest {

struct song;

class youtube
{
    youtube() = delete;
public:
    static bool retrieve_video_data(song& songData, const char* streamerCountry);
private:
    static bool parse_json(std::string& json, song& songData, const char* streamerCountry);
};

}
}
}
