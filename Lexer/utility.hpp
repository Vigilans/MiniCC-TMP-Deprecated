#ifndef UTILITY_H_
#define UTILITY_H_
#include <type_traits>

namespace cp {

template <char c>
struct char_constant : std::integral_constant<char, c> {};

template <class T, T c1, T c2>
constexpr std::bool_constant<c1 == c2> operator==(std::integral_constant<T, c1>, std::integral_constant<T, c2>) { return {}; }

template <class T, T c1, T c2>
constexpr std::bool_constant<c1 != c2> operator!=(std::integral_constant<T, c1>, std::integral_constant<T, c2>) { return {}; }

template <char c>
constexpr static auto ch = char_constant<c>{};

template <char... chars>
struct char_sequence {
    template <size_t i>
    constexpr static decltype(auto) get() {
        static_assert(i < sizeof...(chars));
        return char_constant<std::get<i>(std::make_tuple(chars...))>{};
    }
};

template <bool v, class T1, class T2>
constexpr decltype(auto) op_cond(std::bool_constant<v>, T1 lhs, T2 rhs) {
    if constexpr (v) return lhs;
    else return rhs;
}

template <class Variable, class Condition, class Transfer>
constexpr decltype(auto) op_loop(Variable state, Condition cond, Transfer trans) {
    if constexpr (cond(state).value) {
        return op_loop(trans(state), cond, trans);
    } else {
        return state;
    }
}

}


#endif // !UTILITY_H_
