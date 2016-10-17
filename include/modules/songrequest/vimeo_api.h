#pragma once

namespace ashbot {
namespace modules {
namespace songrequest {

struct song;

class vimeo
{
private:
    vimeo() = delete;
public:
    static bool         retrieve_video_data(song& songData);
private:
    static bool         parse_json(std::string& json, song& song);
};

}
}
}
