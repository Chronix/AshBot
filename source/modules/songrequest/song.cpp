#include <iomanip>
#include <sstream>

#include "modules/songrequest/song.h"

#define YT_ID_LENGTH   11

namespace ashbot {
namespace modules {
namespace songrequest {

namespace regex {

// (v=){1}[a-zA-Z0-9\\-_]+
const bx::cregex YouTubeIdV = bx::repeat<1>("v=") >> +bx::set[bx::alnum | '-' | '_'];

// ^(?:https?://)?(?:www\.|m\.)?(?:youtu\.be/|youtube\.com(?:/embed/|/v/|/watch\?v=|/watch\?.+&v=))([\w-]{11})(?:.+)?$
const bx::cregex YouTubeId =
    bx::bos >>
    !("http" >> !bx::as_xpr('s') >> "://") >>
    !("www." | bx::as_xpr("m.")) >>
    ("youtu.be" | bx::as_xpr("youtube.com") >> (bx::as_xpr("/embed/") | "/v/" | "/watch?v=" | "/watch?" >> +bx::_ >> "&v=")) >>
    (bx::s1 = bx::repeat<YT_ID_LENGTH>(bx::set[bx::_w | '-'])) >>
    !(+bx::_) >>
    bx::eos;

// \d+$
const bx::cregex VimeoId = +bx::_d >> bx::eos;

}

song::song()
    :   Id(-1),
        Source(source::youtube),
        Banned(false),
        BanOnRequest(false),
        Error(error::none)
{
}

void song::parse_link(const char* pLink)
{
    if (try_process_youtube_link(pLink) || try_process_soundcloud_link(pLink) || try_process_vimeo_link(pLink)) return;
    process_youtube_id(pLink);
}

void song::parse_iso8601_created_date(const std::string& stringRep)
{
    // this does not handle the timezone part
    // since the createdAt field is purely informative
    // and in fact unused right now, it does not matter

    tm s_tm = {};
    std::istringstream iss(stringRep);
    iss >> std::get_time(&s_tm, "%Y-%m-%dT%H:%M:%S");
    CreatedAt = clock_type::from_time_t(mktime(&s_tm));
}

bool song::try_process_youtube_link(const char* pLink)
{
    bx::cmatch match;
    if (regex_match(pLink, match, regex::YouTubeId))
    {
        TrackId.assign(match[1].first, YT_ID_LENGTH);
    }
    else
    {
        if (!regex_search(pLink, match, regex::YouTubeIdV)) return false;
        TrackId.assign(match[1].first + 2, YT_ID_LENGTH);
    }

    Source = source::youtube;
    return true;
}

bool song::try_process_soundcloud_link(const char* pLink)
{
    if (!strstr(pLink, "soundcloud")) return false;
    Link.assign(pLink);
    Source = source::soundcloud;
    return true;
}

bool song::try_process_vimeo_link(const char* pLink)
{
    if (!strstr(pLink, "vimeo")) return false;

    bx::cmatch match;
    if (regex_search(pLink, match, regex::VimeoId))
    {
        TrackId.assign(match[1].first);
        Source = source::vimeo;
        return true;
    }

    return false;
}

void song::process_youtube_id(const char* pLink)
{
    TrackId.assign(pLink);
    Source = source::youtube;
}

}
}
}