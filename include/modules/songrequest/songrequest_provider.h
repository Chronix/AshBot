#pragma once

#include <boost/container/flat_map.hpp>
#include <boost/container/static_vector.hpp>
#include <boost/variant/variant.hpp>
#include <boost/thread.hpp>

#include "cc_queue.h"
#include "modules/bot_module.h"
#include "song.h"
#include "songrequest.h"
#include "twitch_user.h"

#define ASHBOT_SR_EXTRA_DATA_MAX 2

namespace ashbot {

class db_result;

namespace remote {
class control_server;
}

namespace modules {
namespace songrequest {

class songrequest_provider : public bot_module
{
    enum class song_request_action
    {
        new_song,
        current_song,
        skip_song,
        reset_songs,
        add_current_to_list,
        add_new_to_list,
        request_song_from_list,
        promote_song,
        ban_song,
        prioritize_song,
        ban_playlist
    };

    struct song_request_item
    {
        using extra_data_item = boost::variant<std::string, int, bool>;
        using extra_data = boost::container::static_vector<extra_data_item,
                                                           ASHBOT_SR_EXTRA_DATA_MAX>;

        std::string             Link;
        twitch_user::ptr        RequestedBy;
        int64_t                 Id;
        song_request_action     Action;
        extra_data              ExtraData;

        song_request_item(twitch_user* pUser, const char* pLink,
                          int64_t id, song_request_action action)
            :   Link(pLink)
            ,   RequestedBy(pUser)
            ,   Id(id)
            ,   Action(action)
        {
        }
    };

    struct song_request_status
    {
        int64_t                 RequestId;
        song                    Song;
        std::string             RequestedBy;
        std::string             StdLink;
        bool                    IsPlaying;
    };

    using request_item_ptr = std::shared_ptr<song_request_item>;
    using user_limits_map = boost::container::flat_map<int64_t, int>;

    friend remote::control_server;
public:
    explicit                    songrequest_provider(channel_context* pcc);
    virtual                     ~songrequest_provider();
public:
    bool                        initialize();

    void                        request_song(twitch_user* pUser, const char* pLink);

    void                        set_enabled(bool enabled) override;

    void                        song_state_change(int64_t requestId, bool isPlaying);
private:
    bool                        load_config();
    static void                 set_config_value(int config, const char* value);

    void                        request_worker();

    void                        request_song(request_item_ptr&& item);
    void                        current_song(request_item_ptr&& item);
    void                        skip_song(request_item_ptr&& item);
    void                        reset_songs(request_item_ptr&& item);

    bool                        request_song_check(const request_item_ptr& item);
    static bool                 is_user_banned(const char* pName);
    bool                        is_queue_full() const;
    bool                        user_can_add_more_songs(const request_item_ptr& item, int& existingSongs) const;

    static bool                 get_song(const request_item_ptr& item, song& s);
    static void                 fill_song(song& s, const db_result& dbRes);

    static void                 notify_song_list_change(bool skipCurrentSong = false);
private:
    bool                        subOnly_;
    int                         queueCapacity_;
    int                         songsPerPleb_;
    int                         songsPerRegular_;
    int                         songsPerSub_;
    int                         songsPerMod_;
    duration                    maxSongLength_;

    user_limits_map             customLimits_;

    boost::thread               workerThread_;
    blocking_queue<request_item_ptr>
                                items_;
    song_request_status         status_;
    std::atomic<int64_t>        localId_;
};

}
}
}
