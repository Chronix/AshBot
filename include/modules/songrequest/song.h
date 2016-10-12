#pragma once

#include <boost/xpressive/xpressive.hpp>

#include "songrequest.h"

namespace ashbot {
namespace modules {
namespace songrequest {

namespace bx = boost::xpressive;

class song
{
    friend class youtube;
public:
    enum class error
    {
        none,
        host_error,
        does_not_exist,
        region_restricted,
        not_embeddable,
        not_a_track
    };

    enum class source
    {
        youtube,
        soundcloud,
        vimeo
    };
public:
                            song();
public:
    void                    parse_link(const char* pLink);

    const char*             track_id() const { return trackId_; }
private:
    bool                    try_process_youtube_link(const char* pLink);
    bool                    try_process_soundcloud_link(const char* pLink);
    bool                    try_process_vimeo_link(const char* pLink);
    void                    process_youtube_id(const char* pLink);
private:
    int64_t                 id_;
    char                    name_[512];
    std::string             link_;
    char                    trackId_[64];
    source                  source_;
    song_duration           length_;
    bool                    banned_;
    bool                    banOnRequest_;
    error                   error_;
    clock_type::time_point  createdAt_;
};

}
}
}
