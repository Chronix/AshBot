#include <unordered_set>

#include <boost/asio.hpp>

#include "sub_manager.h"
#include "ashbot.h"
#include "db/db.h"
#include "mutex.h"
#include "thread_pool.h"
#include "twitch_api.h"

#define REFRESH_TIMER_EXPIRY boost::posix_time::hours(12)

namespace ashbot {
namespace sub_manager {

namespace {

sub_collection                  Subscribers;
boost::asio::deadline_timer*    pRefreshTimer;
mutex                           Mutex;

void refresh_timer_elapsed(const boost::system::error_code& ec)
{
    if (ec)
    {
        if (ec.value() == boost::asio::error::operation_aborted) return;
        AshBotLogError << "Sub manager timer error: " << ec.message();
    }

    get_subs(true);
    AshBotLogInfo << "Refreshed subs";

    pRefreshTimer->expires_from_now(REFRESH_TIMER_EXPIRY);
    pRefreshTimer->async_wait(refresh_timer_elapsed);
}

void update_sub_db()
{
    static constexpr char SQL_UPDATE_SUBS[] = "SELECT update_subs($1::varchar[])";
    static constexpr char SQL_UPDATE_NONSUBS[] = "UPDATE users SET subscribed = FALSE WHERE subscribed = TRUE AND NOT username = ANY($1::varchar[]);";

    std::vector<const std::string*> subs;
    subs.reserve(Subscribers.size());
    for (auto& sub : Subscribers) subs.push_back(&sub);

    pgsql_array<const std::string*> dbArray(subs.data(), subs.size());
    db::get().query(SQL_UPDATE_SUBS, dbArray);
    db::get().query(SQL_UPDATE_NONSUBS, dbArray);
}

}

void initialize()
{
    pRefreshTimer = new boost::asio::deadline_timer(tp_get_ioservice(), REFRESH_TIMER_EXPIRY);
    pRefreshTimer->async_wait(refresh_timer_elapsed);
}

void cleanup()
{
    delete pRefreshTimer;
}

void add_sub(const char* pName)
{
    static constexpr char SQL_SET_SUB[] = "UPDATE users SET subscribed = TRUE WHERE id = $1::bigint";

    {
        mutex::scoped_lock l(Mutex);
        Subscribers.emplace(pName);
    }

    db::user_id userId = db::get().get_user(pName);
    db_result rSub = db::get().query(SQL_SET_SUB, pName);
    if (!rSub)
    {
        AshBotLogError << "Failed to upgrade user " << pName << " to sub status [" << rSub.error_message() << "]";
    }
}

bool is_sub(const char* pName)
{
    static constexpr char SQL_IS_SUB[] = "SELECT 1 FROM users WHERE username = $1::varchar AND subscribed = TRUE";

    {
        mutex::scoped_lock l(Mutex);
        if (!Subscribers.empty()) return Subscribers.find(pName) != Subscribers.end();
    }

    db_result rSub = db::get().query(SQL_IS_SUB, pName);
    if (!rSub)
    {
        AshBotLogError << "Failed to determine if user " << pName << " is a subscriber [" << rSub.error_message() << "]";
        return false;
    }

    return !rSub.is_field_null(0, 0);
}

bool was_sub(const char* pName)
{
    static constexpr char SQL_WAS_SUB[] = "SELECT 1 FROM users WHERE username = $1::varchar AND subscribed = FALSE AND ever_subscribed = TRUE";

    db_result rSub = db::get().query(SQL_WAS_SUB, pName);
    if (!rSub)
    {
        AshBotLogError << "Failed to determine if user " << pName << " was a subscriber [" << rSub.error_message() << "]";
        return false;
    }

    return !rSub.is_field_null(0, 0);
}

bool is_regular(const char* pName, bool alsoSubRegular)
{
    static constexpr char SQL_IS_REGULAR[] = "SELECT 1 FROM users WHERE username = $1::varchar AND regular = TRUE";
    static constexpr char SQL_IS_SUB_REGULAR[] = "SELECT 1 FROM users WHERE username = $1::varchar AND (regular = TRUE OR subscribed = TRUE)";

    db_result rReg = db::get().query(alsoSubRegular ? SQL_IS_SUB_REGULAR : SQL_IS_REGULAR, pName);
    if (!rReg)
    {
        AshBotLogError << "Failed to determine if user " << pName << " is a regular [" << rReg.error_message() << "]";
        return false;
    }

    return !rReg.is_field_null(0, 0);
}

int add_regular(const char* pName)
{
    if (is_regular(pName, false)) return 1;

    db::user_id userId = db::get().get_user(pName);
    if (userId == db::InvalidUserId) return -1;

    static constexpr char SQL_ADD_REGULAR[] = "UPDATE users SET regular = TRUE WHERE id = $1::bigint";
    db_result rReg = db::get().query(SQL_ADD_REGULAR, userId);
    if (!rReg)
    {
        AshBotLogError << "Failed to upgrade user " << pName << " to regular status [" << rReg.error_message() << "]";
        return 0;
    }

    return 1;
}

bool remove_regular(const char* pName)
{
    if (!is_regular(pName, false)) return false;

    static constexpr char SQL_REMOVE_REGULAR[] = "UPDATE users SET regular = FALSE WHERE username = $1::varchar";
    db_result rReg = db::get().query(SQL_REMOVE_REGULAR, pName);
    if (!rReg)
    {
        AshBotLogError << "Failed to downgrade user " << pName << " from regular status [" << rReg.error_message() << "]";
        return false;
    }

    return true;
}

void get_subs(bool fullRefresh)
{
    mutex::scoped_lock l(Mutex);

    int offset = 0;
    int count = 100;

    if (fullRefresh) Subscribers.clear();
    else count = 10;

    for (; offset < 10000; offset += 100)
    {
        int result = twitch_api::get_channel_subscribers(ASHBOT_CHANNEL, count, offset, Subscribers);
        if (result == 0 || !fullRefresh || Subscribers.size() < 100) break;
    }

    if (!Subscribers.empty()) update_sub_db();
    printf("Subcount: %llu", Subscribers.size());

#if defined(WIN32) || defined(_WIN32)
    char title[32];
    snprintf(title, 32, "AshBot - Subs: %llu", Subscribers.size());
    SetConsoleTitleA(title);
#endif
}

}
}
