#include <boost/lexical_cast.hpp>

#include "modules/songrequest/songrequest_provider.h"
#include "ashbot.h"
#include "channel_context.h"
#include "db/db.h"
#include "http_client.h"
#include "modules/songrequest/soundcloud_api.h"
#include "modules/songrequest/vimeo_api.h"
#include "modules/songrequest/youtube_api.h"
#include "sub_manager.h"
#include "thread_pool.h"

namespace {

constexpr char MSGSTR_USER_IS_BANNED[] = "Sorry, %s, you're banned from requesting songs.";
constexpr char MSGSTR_QUEUE_IS_FULL[] = "Sorry, %s, the song request queue is full.";
constexpr char MSGSTR_TOO_MANY_SONGS[] = "Sorry, %s, you already have %i songs in the queue.";
const char* const MSGSTRS_SONG_ERRORS[] =
{
    "Sorry, %s, your request could not be processed (site down?).",
    "Sorry, %s, this video does not exist.",
    "Sorry, %s, this song cannot be played in the streamer's country.",
    "Sorry, %s, this video cannot be embedded.",
    "Sorry, %s, this link is not a track (playlists and other types of links are not supported)."
};
constexpr char MSGSTR_SONG_TOO_LONG[] = "Sorry, %s, this song is longer than %i minutes.";
constexpr char MSGSTR_SONG_BANNED[] = "Nice try, %s.";
constexpr char MSGSTR_SONG_BANNED_BAN_REASON[] = "Requesting banned song";
constexpr char MSGSTR_REQUEST_ALREADY_EXISTS[] = "%s, this song is already in the queue.";
constexpr char MSGSTR_REQUEST_ADDED[] = "%s, %s has been added to the queue (RID %llu).";
constexpr char MSGSTR_NOW_PAUSED[] = "Song requests are not playing right now.";
constexpr char MSGSTR_CURRENT_SONG[] = "%s, current song is '%s'. It was requested by %s and its link is %s (RID %lld)";
constexpr char MSGSTR_SONGLIST_RESET[] = "Song list reset!";

enum
{
    SR_OPT_ENABLED = 0,
    SR_OPT_SUBONLY,
    SR_OPT_MAXLEN,
    SR_OPT_QCAPACITY,
    SR_OPT_PLEBSONGS,
    SR_OPT_REGSONGS,
    SR_OPT_SUBSONGS,
    SR_OPT_MODSONGS,

    SR_OPT_COUNT__
};

}

namespace ashbot {
namespace modules {
namespace songrequest {

songrequest_provider::songrequest_provider(channel_context* pcc)
    :   bot_module(pcc, false)
    ,   subOnly_(false)
    ,   queueCapacity_(40)
    ,   songsPerPleb_(2)
    ,   songsPerRegular_(3)
    ,   songsPerSub_(5)
    ,   songsPerMod_(5)
    ,   status_{}
    ,   localId_(0)
{
}

songrequest_provider::~songrequest_provider()
{
    enabled_ = false;
    if (workerThread_.joinable()) workerThread_.join();
}

bool songrequest_provider::initialize()
{
    bool init = load_config();
    if (init) workerThread_ = boost::thread(&songrequest_provider::request_worker, this);
    return init;
}

void songrequest_provider::request_song(twitch_user* pUser, const char* pLink)
{
    request_item_ptr item = std::make_shared<song_request_item>(pUser, pLink, localId_++,
                                                                song_request_action::new_song);
    AshBotLogDebug << "New song request " << item->Id << " - " << pLink << " - by " << pUser->username();
    items_.enqueue(move(item));
}

void songrequest_provider::set_enabled(bool enabled)
{
    bot_module::set_enabled(enabled);
    std::array<char, 8> buffer = boost::lexical_cast<decltype(buffer)>(enabled);
    set_config_value(SR_OPT_ENABLED, buffer.data());
}

void songrequest_provider::song_state_change(int64_t requestId, bool isPlaying)
{
    if (requestId == -1) return;

    mutex::scoped_lock lock(mutex_);

    status_.IsPlaying = isPlaying;
    status_.RequestId = requestId;

    static constexpr char SQL_GET_SONG[] = "SELECT s.name, u.username, s.source, s.link, s.track_id"
                                           "FROM song_requests sr"
                                           "INNER JOIN songs s ON sr.song_id = s.id"
                                           "INNER JOIN users u ON sr.user_id = u.id"
                                           "WHERE sr.id = $1::bigint";
    db_result result = db::get().query(SQL_GET_SONG, requestId);
    if (!result)
    {
        AshBotLogError << "Failed to retrieve song data [" << result.error_message() << "]";
        return;
    }

    result.read_column(status_.Song.Name, 0, 0);
    result.read_column(status_.RequestedBy, 0, 1);
    result.read_column(reinterpret_cast<int16_t&>(status_.Song.Source), 0, 2);
    result.read_column(status_.Song.Link, 0, 3);
    result.read_column(status_.Song.TrackId, 0, 4);

    status_.StdLink.assign(status_.Song.get_standardized_link());
}

bool songrequest_provider::load_config()
{
    static constexpr char SQL_LOAD_CONFIG[] = "SELECT value FROM sr_data ORDER BY id ASC;";
    static constexpr char SQL_LOAD_LIMITS[] = "SELECT userid, \"limit\" FROM sr_limits;";

    db_result dbConfig = db::get().query(SQL_LOAD_CONFIG);

    if (dbConfig.row_count() != SR_OPT_COUNT__)
    {
        AshBotLogError << "Expected 8 options in songrequest config table, got " 
                       << dbConfig.row_count();
        return false;
    }

    char options[SR_OPT_COUNT__][32];
    for (int i = 0; i < SR_OPT_COUNT__; ++i)
    {
        dbConfig.read_column(options[i], i, 0);
    }

    enabled_ = boost::lexical_cast<bool>(options[SR_OPT_ENABLED]);
    subOnly_ = boost::lexical_cast<bool>(options[SR_OPT_SUBONLY]);
    maxSongLength_ = std::chrono::seconds(boost::lexical_cast<int>(options[SR_OPT_MAXLEN]));
    queueCapacity_ = boost::lexical_cast<int>(options[SR_OPT_QCAPACITY]);
    songsPerPleb_ = boost::lexical_cast<int>(options[SR_OPT_PLEBSONGS]);
    songsPerRegular_ = boost::lexical_cast<int>(options[SR_OPT_REGSONGS]);
    songsPerSub_ = boost::lexical_cast<int>(options[SR_OPT_SUBSONGS]);
    songsPerMod_ = boost::lexical_cast<int>(options[SR_OPT_MODSONGS]);

    db_result dbLimits = db::get().query(SQL_LOAD_LIMITS);

    for (int i = 0; i < dbLimits.row_count(); ++i)
    {
        db::user_id userid;
        dbLimits.read_column(userid, i, 0);

        short count;
        dbLimits.read_column(count, i, 1);

        customLimits_[userid] = count;
    }

    return true;
}

void songrequest_provider::set_config_value(int config, const char* value)
{
    static constexpr char SQL_UPDATE_VALUE[] = "UPDATE sr_data SET value = $1::varchar WHERE id = $2::smallint";
    db::get().query(SQL_UPDATE_VALUE, value, static_cast<short>(config));
}

void songrequest_provider::request_worker()
{
    while (true)
    {
        request_item_ptr item;
        
        while (!items_.wait_dequeue_timed(item, std::chrono::seconds(5)))
        {
            if (!enabled_) return;
        }

        AshBotLogInfo << "Retrieved SR item " << item->Id;

        switch (item->Action)
        {
        case song_request_action::new_song: request_song(move(item)); break;
        case song_request_action::current_song: current_song(move(item)); break;
        case song_request_action::skip_song: skip_song(move(item)); break;
        case song_request_action::reset_songs: reset_songs(move(item)); break;
        case song_request_action::add_current_to_list: break;
        case song_request_action::add_new_to_list: break;
        case song_request_action::request_song_from_list: break;
        case song_request_action::promote_song: break;
        case song_request_action::ban_song: break;
        case song_request_action::prioritize_song: break;
        case song_request_action::ban_playlist: break;
        default: break;
        }
    }
}

void songrequest_provider::request_song(request_item_ptr&& item)
{
    namespace chrono = std::chrono;

    AshBotLogDebug << "Processing request " << item->Id;
    if (!request_song_check(item)) return;

    song s;
    if (!get_song(item, s))
    {
        if (s.Error != song::error::none)
        {
            int err = static_cast<int>(s.Error);
            AshBotSendMessage(MSGSTRS_SONG_ERRORS[err - 1], item->RequestedBy->username());
        }

        AshBotLogDebug << "Failed to retrieve details of song '" << s.Link
                       << "' (source " << static_cast<short>(s.Source) << ")";
        return;
    }

    if (s.Length > maxSongLength_)
    {
        AshBotLogDebug << "Song '" << s.Name << "' rejected, reason: too long";
        AshBotSendMessage(MSGSTR_SONG_TOO_LONG, item->RequestedBy->username(),
                          chrono::duration_cast<chrono::minutes>(maxSongLength_).count());
        return;
    }

    if (s.Banned)
    {
        AshBotLogDebug << "Song '" << s.Name << "' rejected, reason: banned";
        AshBotSendMessage(MSGSTR_SONG_BANNED, item->RequestedBy->username());
        
        if (s.BanOnRequest)
        {
            AshBotLogDebug << "Banning user " << item->RequestedBy->username() << " for "
                           << s.BanLength << " seconds";
            context()->timeout_user(item->RequestedBy->username(), s.BanLength, MSGSTR_SONG_BANNED_BAN_REASON);
        }

        return;
    }

    // for now this doesn't need a lock because all SR actions happen
    // sequentially on the module thread

    static constexpr char SQL_FIND_EXISTING_REQUEST[] = "SELECT id FROM active_song_requests WHERE song_id = $1::bigint LIMIT 1;";
    db_result result = db::get().query(SQL_FIND_EXISTING_REQUEST, s.Id);
    if (!result)
    {
        AshBotLogDebug << "Failed to check for existing song request [" << result.error_message() << "]";
        // inform user? this wasn't really checked for/handled in the old bot
        return;
    }

    if (result.row_count() > 0 && !result.is_field_null(0, 0))
    {
        int64_t rid;
        result.read_column(rid, 0, 0);
        AshBotLogDebug << "Song '" << s.Name << "' aready in queue (RID " << rid << "), discarding request";
        AshBotSendMessage(MSGSTR_REQUEST_ALREADY_EXISTS, item->RequestedBy->username());
        return;
    }

    static constexpr char SQL_GET_NEXT_POSITION[] = "SELECT COALESCE(MAX(pos), 0) + 1 FROM active_song_requests;";
    result = db::get().query(SQL_GET_NEXT_POSITION);
    if (!result)
    {
        AshBotLogDebug << "Failed to get next request position [" << result.error_message() << "]";
        return;
    }

    int64_t position;
    result.read_column(position, 0, 0);

    db::user_id userId = db::get().get_user(item->RequestedBy->username());

    static constexpr char SQL_INSERT_NEW_REQUEST[] = "INSERT INTO song_requests(song_id, user_id, pos) " 
                                                     "VALUES($1::bigint, $2::bigint, $3::bigint) RETURNING id;";
    result = db::get().query(SQL_INSERT_NEW_REQUEST, s.Id, userId, position);

    if (!result)
    {
        AshBotLogDebug << "Failed to insert new request into the database [" << result.error_message() << "]";
        return;
    }

    int64_t rid;
    result.read_column(rid, 0, 0);

    notify_song_list_change();
    AshBotSendMessage(MSGSTR_REQUEST_ADDED, item->RequestedBy->username(), s.Name.c_str(), rid);
}

void songrequest_provider::current_song(request_item_ptr&& item)
{
    if (!status_.IsPlaying) AshBotSendMessage(MSGSTR_NOW_PAUSED);
    else AshBotSendMessage(MSGSTR_CURRENT_SONG, item->RequestedBy->username(), status_.Song.Name.c_str(),
                           status_.RequestedBy.c_str(), status_.StdLink.c_str(), status_.RequestId);
}

void songrequest_provider::skip_song(request_item_ptr&& item)
{
    static constexpr char SQL_SKIP_SONG[] = "SELECT skip_song($1::bigint);";
    int64_t songId = item->Id == -1 ? status_.RequestId : item->Id;
    db::get().query(SQL_SKIP_SONG, songId);
}

void songrequest_provider::reset_songs(request_item_ptr&& item)
{
    static constexpr char SQL_RESET_SONGS[] = "UPDATE song_requests SET hidden = TRUE, pos = NULL;";
    db::get().query(SQL_RESET_SONGS);
    notify_song_list_change();
    AshBotSendMessage(MSGSTR_SONGLIST_RESET);
}

bool songrequest_provider::request_song_check(const request_item_ptr& item)
{
    if (!enabled_) return false;
    if (subOnly_ && !item->RequestedBy->sub() && !item->RequestedBy->mod()) return false;

    if (is_user_banned(item->RequestedBy->username()))
    {
        AshBotLogInfo << "User " << item->RequestedBy->username() << " is banned from requesting songs";
        AshBotSendMessage(MSGSTR_USER_IS_BANNED, item->RequestedBy);
        return false;
    }

    if (is_queue_full())
    {
        AshBotLogInfo << "Song queue is full";
        AshBotSendMessage(MSGSTR_QUEUE_IS_FULL, item->RequestedBy);
        return false;
    }

    int existingSongs;
    if (user_can_add_more_songs(item, existingSongs)) return true;

    AshBotLogInfo << "User " << item->RequestedBy->username() << " has too many songs in the queue";
    AshBotSendMessage(MSGSTR_TOO_MANY_SONGS, item->RequestedBy);

    return true;
}

bool songrequest_provider::is_user_banned(const char* pName)
{
    static constexpr char SQL_USER_BANNED[] = "SELECT 1 FROM users WHERE username = $1::varchar AND sr_ban = TRUE;";

    db_result result = db::get().query(SQL_USER_BANNED, pName);
    if (!result)
    {
        AshBotLogError << "Failed to retrieve songrequest ban status for " << pName << " [" << result.error_message() << "]";
        return false; // innocent until proven guilty
    }

    return result.row_count() > 0 && !result.is_field_null(0, 0);
}

bool songrequest_provider::is_queue_full() const
{
    static constexpr char SQL_QUEUE_COUNT[] = "SELECT COUNT(*) FROM active_song_requests;";

    db_result result = db::get().query(SQL_QUEUE_COUNT);
    int songCount;

    if (!result || !result.read_column<int>(songCount, 0, 0))
    {
        AshBotLogError << "Failed to retrieve number of songs in the request queue [" << result.error_message() << "]";
        return false;
    }

    return songCount >= queueCapacity_;
}

bool songrequest_provider::user_can_add_more_songs(const request_item_ptr& item, int& existingSongs) const
{
    static constexpr char SQL_GET_USER_SONG_COUNT[] = "SELECT COUNT(*) FROM active_song_requests WHERE username = $1::varchar;";
    const char* pName = item->RequestedBy->username();

    db_result result = db::get().query(SQL_GET_USER_SONG_COUNT, pName);
    if (!result || !result.read_column<int>(existingSongs, 0, 0))
    {
        AshBotLogError << "Failed to retrieve number of active requests of user " << pName << " [" << result.error_message() << "]";
        return true;
    }

    if (item->RequestedBy->mod()) return existingSongs < songsPerMod_;
    if (item->RequestedBy->sub()) return existingSongs < songsPerSub_;
    if (sub_manager::is_regular(pName, false)) return existingSongs < songsPerRegular_;
    return existingSongs < songsPerPleb_;
}

bool songrequest_provider::get_song(const request_item_ptr& item, song& s)
{
    static constexpr char SQL_GET_SONG_TID[] = "SELECT * FROM songs WHERE track_id = $1::varchar AND source = $2::smallint;";
    static constexpr char SQL_GET_SONG_LINK[] = "SELECT * FROM songs WHERE link = $1::varchar;";
    static constexpr char SQL_INSERT_SONG[] = "INSERT INTO songs(source, name, track_id, length, banned, ban_user_on_request, ban_length) "
                                              "VALUES($1::smallint, $2::varchar, $3::varchar, $4::smallint, $5::boolean, $6::boolean, $7::integer) "
                                              "RETURNING id;";

    s.parse_link(item->Link.c_str());

    switch (s.Source)
    {
        case song::source::vimeo:
        case song::source::youtube:
            {
                db_result result = db::get().query(SQL_GET_SONG_TID, s.TrackId.c_str(), static_cast<short>(s.Source));
                if (result.row_count() > 0)
                {
                    fill_song(s, result);
                    return true;
                }
                break;
            }
        case song::source::soundcloud:
            {
                std::string url = soundcloud::strip_soundcloud_link(s.Link);
                db_result result = db::get().query(SQL_GET_SONG_LINK, url.c_str());
                if (result.row_count() > 0)
                {
                    fill_song(s, result);
                    return true;
                }
                break;
            }
        default: return false;
    }

    switch (s.Source)
    {
        case song::source::youtube:
        {
            if (!youtube::retrieve_video_data(s, STREAMER_COUNTRY)) return false;
            break;
        }
        case song::source::soundcloud:
        {
            if (!soundcloud::retrieve_song_data(s)) return false;
            break;
        }
        case song::source::vimeo:
        {
            if (!vimeo::retrieve_video_data(s)) return false;
            break;
        }
        
    }

    AshBotLogDebug << "Retrieved song data, title is " << s.Name;

    if (s.Name.empty())
    {
        s.Error = song::error::does_not_exist;
        return false;
    }
    
    auto secLen = std::chrono::duration_cast<std::chrono::seconds>(s.Length);
    db_result result = db::get().query(SQL_INSERT_SONG, static_cast<short>(s.Source), s.Name.c_str(),
                                       s.TrackId.c_str(), static_cast<short>(secLen.count()), false, false, 0);
    
    if (result) result.read_column(s.Id, 0, 0);
    else
    {
        AshBotLogError << "Failed to save song data [" << result.error_message() << "]";
        return false; // if the song isn't saved, the SR site won't be able to get its details later
    }

    return true;
}

void songrequest_provider::fill_song(song& s, const db_result& dbRes)
{
    s.Error = song::error::none;
    dbRes.read_column(s.Id, 0, 0);
    dbRes.read_column(s.Name, 0, 2);
    
    short lengthSec;
    dbRes.read_column(lengthSec, 0, 5);
    s.Length = std::chrono::seconds(lengthSec);

    dbRes.read_column(s.Banned, 0, 6);
    dbRes.read_column(s.BanOnRequest, 0, 7);

    dbRes.read_column(s.BanLength, 0, 8);
}

void songrequest_provider::notify_song_list_change(bool skipCurrentSong)
{
    static constexpr char ListChangeUrl[] = SR_BASE_URL "SongListChange";
    static constexpr char SkipCurrentUrl[] = SR_BASE_URL "SkipCurrentSong";

    http_client client;
    client.add_header("User-Agent", "AshBot v1.x");
    http_response response = client.send_request(skipCurrentSong ? SkipCurrentUrl : ListChangeUrl, false);

    if (response.has_error())
    {
        AshBotLogError << "Error during song list change notification, status code " << response.status_code();
    }
}

}
}
}

