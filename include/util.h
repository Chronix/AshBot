#pragma once

namespace ashbot {

template<typename T, size_t N>
constexpr size_t array_size(T (&array)[N])
{
    return N;
}

template<typename TRet = int, typename T, size_t N>
constexpr
std::enable_if_t<std::is_integral<T>::value, TRet>
static_strlen(T (&array)[N])
{
    return static_cast<TRet>(array_size(array) - 1);
}

}
