#pragma once
#include <type_traits>

template <typename T>
class CriticalStrike {
    static_assert(std::is_arithmetic_v<T>, "Multiplier must be numeric (int/float)");
private:
    T multiplier;
public:
    explicit CriticalStrike(T mult) : multiplier(mult) {}
    T calculateDamage(T baseDamage) const { return baseDamage * multiplier; }
    T getMultiplier() const { return multiplier; }
};

