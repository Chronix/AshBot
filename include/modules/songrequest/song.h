#pragma once

#include <boost/xpressive/xpressive.hpp>

#include "songrequest.h"

namespace ashbot {
namespace modules {
namespace songrequest {

namespace bx = boost::xpressive;

struct song
{
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
    void                    parse_iso8601_created_date(const std::string& stringRep);
private:
    bool                    try_process_youtube_link(const char* pLink);
    bool                    try_process_soundcloud_link(const char* pLink);
    bool                    try_process_vimeo_link(const char* pLink);
    void                    process_youtube_id(const char* pLink);
public:
    int64_t                 Id;
    std::string             Name;
    std::string             Link;
    std::string             TrackId;
    source                  Source;
    duration                Length;
    bool                    Banned;
    bool                    BanOnRequest;
    duration                BanLength;
    error                   Error;
    clock_type::time_point  CreatedAt;
};

}
}
}
