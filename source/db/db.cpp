#include "db/db.h"
#include "db/db_debug_data.h"
#include "logging.h"

namespace ashbot {

db::db()
{
}

db::~db()
{
    PGconn* pc;
    while (connectionPool_.try_dequeue(pc)) PQfinish(pc);
}

int64_t db::get_user(const char* pUsername)
{
    static constexpr char SQL_GET_USER[] = "SELECT get_user($1::varchar);";

    db_result result = query(SQL_GET_USER, pUsername);
    int64_t id = -1;

    if (!result.is_ok() || result.row_count() != 1)
    {
        AshBotLogError << "Failed to get user id for user " << pUsername
                       << " [" << result.error_message() << "]";
    }
    else result.read_column(id, 0, 0);
    
    return id;
}

PGconn* db::get_connection()
{
    PGconn* pc;
    if (connectionPool_.try_dequeue(pc))
    {
        if (PQstatus(pc) == CONNECTION_OK) return pc;
        PQfinish(pc);
    }

    pc = PQconnectdb(SQL_CONNECTION_STRING);
    if (PQstatus(pc) != CONNECTION_OK)
    {
        AshBotLogFatal << "Cannot connect to the database: " << PQerrorMessage(pc);
        return nullptr;
    }

    return pc;
}

void db::release_connection(PGconn* pc)
{
    connectionPool_.enqueue(pc);
}
}
