#pragma once

#include <boost/container/flat_map.hpp>
#include <boost/container/static_vector.hpp>
#include <boost/variant/variant.hpp>

#include "cc_queue.h"
#include "modules/bot_module.h"
#include "songrequest.h"

#define ASHBOT_SR_EXTRA_DATA_MAX 2

namespace ashbot {
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

        std::string         link;
        std::string         requestedBy;
        int64_t             id;
        song_request_action action;
        extra_data          extraData;
    };

    using request_item_ptr = std::shared_ptr<song_request_item>;
    using user_limits_map = boost::container::flat_map<int64_t, int>;
public:
    explicit                    songrequest_provider(channel_context& cc);
public:
    bool                        initialize();
private:
    bool                        load_config();

    void                        request_worker();

    void                        request_song(request_item_ptr&& item);
    void                        current_song(request_item_ptr&& item);

    bool                        request_song_check(const request_item_ptr& item);
private:
    std::atomic<bool>           enabled_;
    bool                        subOnly_;
    int                         queueCapacity_;
    int                         songsPerPleb_;
    int                         songsPerRegular_;
    int                         songsPerSub_;
    int                         songsPerMod_;
    song_duration               maxSongLength_;

    user_limits_map             customLimits_;

    boost::thread               workerThread_;
    blocking_queue<request_item_ptr>
                                items_;
};

}
}
}
