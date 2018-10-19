#ifndef REGEX_DFA_H_
#define REGEX_DFA_H_
#include "utility.hpp"

namespace cp {

// 由Regex AST的结点位置进行编码的状态。
template <size_t... I>
struct state;

// 按编码的字典序对集合进行比较。
template <class Left, class Right>
struct state_compare;

template <size_t... I1, size_t... I2>
struct state_compare<state<I1...>, state<I2...>>
   : lex_compare<size_t, std::index_sequence<I1...>, std::index_sequence<I2...>>;

// 状态的集合，集合里的状态有序且不重复。
template <class... States>
struct state_group {
    // 检查组里是否含有指定状态
    template <class State>
    static constexpr auto has_state = (std::is_same_v<States, State> || ...);

    // 插入新状态
    template <class State>
    using insert = std::conditional_t<
        has_state<State>, state_group<States...>, state_group<State, States...>
    >;
};

// 状态组的列表，列表里的组不保证有序且可重复。
template <class... Groups>
struct group_list;

template <class... Left, class... Right>
struct concat<group_list<Left...>, group_list<Right...>> : group_list<Left..., Right...> {};

template <>
struct group_list<> {
    template <class State>
    using find = empty_group;

    template <class NewGroup>
    using insert = group_list<NewGroup>;
};

template <class Group, class... Rest>
struct group_list<Group, Rest...> {
    // 检索第一个包含指定状态的组
    template <class State>
    using find = std::conditional_t<
        Group::has_state<State>, Group, group_set<Rest...>::template find<State>
    >;

    // 插入新组，并将新组放置在等价组旁边
    template <class NewGroup>
    using insert = std::conditional_t<
        std::is_same_v<Group, NewGroup>,
        group_list<NewGroup, Group, Rest...>,
        concat<group_list<Group>, group_list<Rest...>::insert<NewGroup>>
    >;
};

using null_state = state<>;
using empty_group = state_group<>;

// 状态机上的一个转换。
template <class From, class To, char ch>
struct transition {
    using from_state = From;
    using to_state = To;
    constexpr static char cond = ch;
};

// 一个状态机的转换表。
// 提供了转移检索、字符/状态集获取功能。
template <class... Transitions>
struct trans_table;

template <>
struct trans_table<> {
    template <class From, char ch> 
    using get = null_state;

    using states = empty_group;
    using charset = char_set<>;
};

template <class Transition, class... Rest>
struct trans_table<Transition, Rest...> {
    // 按照源状态和转移条件检索目的状态
    template <class From, char ch>
    using get = std::conditional_t<
        std::is_same_v<typename Transition::from_state, From> && (Transition::cond == ch),
        typename Transition::to_state, trans_table<Rest...>::template get<From, ch>
    >;

    // 字符集及状态集
    using states = trans_table<Rest...>::states::insert<Transition::From>::insert<Transition::To>;
    using charset = trans_table<Rest...>::charset::insert<ch>;
};

template <class TransTable, class TerminalStates>
struct dfa {
    template <class From, char ch>
    using trans = TransTable::get<From, ch>;

    using accept_states = TerminalStates;
    using charset = TransTable::charset;
};

namespace detail {

template <class dfa>
struct min_dfa_proc {

};

}

template <class dfa>
using min_dfa = detail::min_dfa_proc<dfa>::result;

}

#endif // !REGEX_DFA_H_
