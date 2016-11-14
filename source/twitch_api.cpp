#include <jsoncons/json.hpp>

#include "twitch_api.h"
#include "http_client.h"
#include "logging.h"
#include "util.h"

namespace ashbot {
namespace twitch_api {

namespace jc = jsoncons;

int get_channel_subscribers(const char* pChannel, int limit, int offset, sub_manager::sub_collection& subs)
{
    static constexpr char URL_FORMAT[] = "https://api.twitch.tv/kraken/channels/%s/subscriptions?limit=%i&offset=%i&direction=desc";

    char url[128];
    int written = snprintf(url, array_size(url), URL_FORMAT, pChannel, limit, offset);

    if (written < 0 || written >= array_size(url))
    {
        AshBotLogFatal << "Subscriber API endpoint url overflow (channel: " << pChannel
                       << " limit: " << limit << " offset: " << offset << ")";
        return 0;
    }

    twitch_http_client client;
    client.add_twitch_auth_headers();
    http_response response = client.send_request(url, true);

    if (response.has_error())
    {
        AshBotLogError << "Failed to get list of channel subscribers (channel: " << pChannel
                       << " limit: " << limit << " offset: " << offset << ") - http status code: "
                       << response.status_code();
        return 0;
    }

    jc::json subsObject = jc::json::parse(response.data());
    auto total = subsObject.find("_total");
    if (total == subsObject.object_range().end() || total->value().as_integer() == 0)
    {
        AshBotLogWarn << "Twitch API reports 0 subscribers for channel " << pChannel;
        return true;
    }

    jc::json subsArray = subsObject["subscriptions"];
    if (!subsArray.is_array())
    {
        AshBotLogError << "Twitch API returned malformed json data for channel " << pChannel
                       << " (subscriptions not an array)";
        return 0;
    }

    int added = 0;

    for (auto& subObject : subsArray.array_range())
    {
        try
        {
            jc::json userObject = subObject["user"];
            std::string name = userObject["name"].as_string();
            subs.insert(move(name));
            ++added;
        }
        catch (std::exception& ex)
        {
            AshBotLogError << "Error parsing sub array element (" << ex.what() << ")";
        }
    }

    return added;
}

void get_channel_chatters(const char* pChannel, std::vector<std::string>& names)
{
    static constexpr char URL_FORMAT[] = "https://tmi.twitch.tv/group/user/%s/chatters";
    char url[128];
    int written = snprintf(url, array_size(url), URL_FORMAT, pChannel);

    if (written < 0 || written >= array_size(url))
    {
        AshBotLogFatal << "Viewer API endpoint url overflow (channel: " << pChannel << ")";
        return;
    }

    http_client client;
    http_response response = client.send_request(url, true);

    if (response.has_error())
    {
        AshBotLogError << "Failed to get list of channel viewers (channel: " << pChannel
                       << "), status code " << response.status_code();
        return;
    }

    jc::json viewersObject = jc::json::parse(response.data());

    size_t chatterCount = viewersObject["chatter_count"].as_uinteger();
    names.reserve(chatterCount);

    for (auto chatterCategory : viewersObject["chatters"].object_range())
    {
        for (auto chatter : chatterCategory.value().array_range())
        {
            names.push_back(chatter.as_string());
        }
    }
}

}
}
