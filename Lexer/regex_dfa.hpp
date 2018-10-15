#ifndef REGEX_DFA_H_
#define REGEX_DFA_H_
#include "utility.hpp"

namespace cp::dfa {

template <int... I> 
struct state {};

using null_state = state<>;

template <class From, class To, char ch>
struct transition {
    using from_state = From;
    using to_state = To;
    static constexpr auto cond = ch;
};

template <class... Transitions>
struct dfa;

template <>
struct dfa<> {
    template <class From, char ch>
    using trans = null_state;
};

template <class Transition, class... Rest>
struct dfa<Transition, Rest...> {
    template <class From, char ch>
    using trans = std::conditional_t<
        std::is_same_v<typename Transition::from_state, From> && (Transition::cond == ch),
        typename Transition::to_state, typename dfa<Rest...>::template trans<From, ch>
    >;
};

}

#endif // !REGEX_DFA_H_
