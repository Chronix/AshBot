#include <boost/lexical_cast.hpp>

#include "modules/songrequest/songrequest_provider.h"
#include "db/db.h"
#include "thread_pool.h"

namespace {

constexpr char SQL_LOAD_CONFIG[] = "SELECT value FROM sr_data ORDER BY id ASC;";
constexpr char SQL_LOAD_LIMITS[] = "SELECT userid, limit FROM sr_limits;";

constexpr char SQL_GET_SONG_TID[] = "SELECT * FROM songs WHERE track_id = $1::varchar AND source = $2::smallint;";
constexpr char SQL_GET_SONG_LINK[] = "SELECT * FROM songs WHERE link = $1::varchar;";

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

songrequest_provider::songrequest_provider(channel_context& cc)
    :   bot_module(cc),
        enabled_(false),
        subOnly_(false),
        queueCapacity_(40),
        songsPerPleb_(2),
        songsPerRegular_(3),
        songsPerSub_(5),
        songsPerMod_(5)
{
}

bool songrequest_provider::initialize()
{
    bool init = load_config();
    if (init) workerThread_ = boost::thread(&songrequest_provider::request_worker, this);
    return init;
}

bool songrequest_provider::load_config()
{
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
        dbConfig.read_column(options[i], i, 1);
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

void songrequest_provider::request_worker()
{
    while (true)
    {
        request_item_ptr item;
        
        while (!items_.wait_dequeue_timed(item, std::chrono::seconds(5)))
        {
            if (!enabled_) return;
        }

        AshBotLogInfo << "Retrieved SR item " << item->id;

        switch (item->action)
        {
        case song_request_action::new_song: request_song(move(item)); break;
        case song_request_action::current_song: break;
        case song_request_action::skip_song: break;
        case song_request_action::reset_songs: break;
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
    AshBotLogDebug << "Processing request " << item->id;
}

bool songrequest_provider::request_song_check(const request_item_ptr& item)
{
    if (!enabled_) return false;

    return true;
}
}
}
}