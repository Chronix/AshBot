#pragma once

#include <unordered_set>

namespace ashbot {
namespace sub_manager {

using sub_collection = std::unordered_set<std::string>;

void initialize();
void cleanup();

void add_sub(const char* pName);
bool is_sub(const char* pName);
bool was_sub(const char* pName);

bool is_regular(const char* pName, bool alsoSubRegular);
int  add_regular(const char* pName);
bool remove_regular(const char* pName);

void get_subs(bool fullRefresh);

}
}
