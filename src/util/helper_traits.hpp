#pragma once

#include <concepts>
#include <type_traits>

namespace lockfree
{

template <typename T>
concept CopyableT = std::is_nothrow_copy_assignable_v<T>
&& std::is_trivially_copy_assignable_v<T>;

}