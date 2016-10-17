#pragma once

namespace ashbot {
namespace modules {
namespace songrequest {

struct song;

class soundcloud
{
private:
    soundcloud() = delete;
public:
    static bool         retrieve_song_data(song& songData);
    static std::string  strip_soundcloud_link(const std::string& link);
private:
    static bool         parse_json(std::string& json, song& song);
};

}
}
}
