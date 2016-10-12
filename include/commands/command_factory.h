#pragma once

#include <unordered_map>

#include "bot_command.h"

namespace ashbot {
namespace commands {

class command_factory
{
    using command_creator = command_ptr(*)();

    using cstr = const char*;

    struct cstr_equal
    {
        bool operator()(cstr pc1, cstr pc2) const
        {
            return _stricmp(pc1, pc2) == 0;
        }
    };

    struct cstr_hash
    {
        size_t operator()(cstr pc) const
        {
            static const size_t FNV_offset_basis = 14695981039346656037ULL;
            static const size_t FNV_prime = 1099511628211ULL;

            size_t length = strlen(pc);

            size_t val = FNV_offset_basis;
            for (size_t next = 0; next < length; ++next)
            {
                val ^= static_cast<size_t>(pc[next]);
                val *= FNV_prime;
            }

            return val;
        }
    };

    using command_creator_map = std::unordered_map<
        cstr,
        command_creator,
        cstr_hash,
        cstr_equal
    >;
public:
    static const command_ptr    null_command;
public:
    template<typename _Command, typename... _Keywords>
    void register_command(_Keywords... keywords);

    command_ptr create_command(const char* pKeyword);
private:
    command_creator_map         creatorMap_;
};

template <typename _Command, typename... _Keywords>
void command_factory::register_command(_Keywords... keywords)
{
    const char* _keywords[] = { keywords... };
    for (const char* pKeyword : _keywords)
    {
        creatorMap_[pKeyword] = make_command<_Command>;
    }
}

inline command_ptr command_factory::create_command(const char* pKeyword)
{
    auto cc = creatorMap_.find(pKeyword);
    if (cc != creatorMap_.end()) return cc->second();
    return command_ptr();
}

}
}
