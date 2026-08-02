#pragma once

// GCC 4.8 / C++11：无 std::make_unique（C++14）
#include <memory>
#include <utility>

namespace TankBattle
{
    template <typename T, typename... Args>
    std::unique_ptr<T> MakeUnique(Args&&... args)
    {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
}
