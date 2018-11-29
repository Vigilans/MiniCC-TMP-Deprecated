#ifndef DFA_H_
#define DFA_H_
#include "dfa_state.hpp"

namespace cp {

/* ---------------------transition实现--------------------- */

// 状态机上的一个转换。
template <class From, char ch, class To>
struct transition {
    using from_state = From;
    using to_state = To;
    constexpr static char cond = ch;
};

// 按{状态->转移条件}的字典序对转换排序
template <class Left, class Right>
struct trans_compare : std::conditional_t<
    !std::is_same_v<typename Left::from_state, typename Right::from_state>,
    state_compare<typename Left::from_state, typename Right::from_state>,
    std::bool_constant<(Left::cond < Right::cond)>
> {};

using null_transition = transition<null_state, -1, null_state>;

/* ---------------------char_set实现--------------------- */

// 字符集合，内部有序集合存储的元素为char_constant<ch>。
template <class CharSeq = char_sequence<>>
struct char_set : ordered_group<int_seq_to_tuple<CharSeq>, integral_compare, char_constant<'\0'>> {
    // type-traits
    using super = ordered_group<int_seq_to_tuple<CharSeq>, integral_compare, char_constant<'\0'>>;

    template <char T>
    using insert = char_set<int_tuple_to_seq<typename super::template insert<char_constant<T>>::tuple>>;

    template <class... Charsets>
    using concat = char_set<int_tuple_to_seq<typename super::template concat<Charsets...>::tuple>>;
};

/* ---------------------trans_table实现--------------------- */

// 一个状态机的转换表，继承自有序集合，可以O(log(N))复杂度匹配转换。
// 提供了转移检索、字符/状态集获取功能。
template <class TransitionTuple = std::tuple<>>
struct trans_table;

template <class Transition, class... Rest>
struct trans_table<std::tuple<Transition, Rest...>> : public ordered_group<
    std::tuple<Transition, Rest...>, trans_compare, null_transition
> {
    // type-traits
    using super = ordered_group<std::tuple<Transition, Rest...>, trans_compare, null_transition>;
    using rest = trans_table<std::tuple<Rest...>>;

    // 按照源状态和转移条件检索目的状态
    template <class From, char ch>
    using trans = typename super::template find<transition<From, ch, null_state>>::to_state;

    template <class T>
    using insert = trans_table<typename super::template insert<T>::tuple>;

    template <class... TransTables>
    using concat = trans_table<typename super::template concat<TransTables...>::tuple>;

    // 字符集及状态集
    using states  = typename rest::states::template insert<
        typename Transition::from_state
    >::template insert<std::conditional_t<
        !std::is_same_v<typename Transition::to_state, null_state>,
        typename Transition::to_state,   // 如果to_state为null_state，则不插入
        typename Transition::from_state  // 这里通过插入一个已经存在的状态实现
    >>;
    using charset = typename trans_table<std::tuple<Rest...>>::charset::template insert<Transition::cond>;
};

template <>
struct trans_table<std::tuple<>> : public ordered_group<std::tuple<>, trans_compare, null_transition> {

    using super = ordered_group<std::tuple<>, trans_compare, null_transition>;

    template <class From, char ch>
    using trans = null_state;

    template <class T>
    using insert = trans_table<std::tuple<T>>;

    template <class... TransTables>
    using concat = trans_table<typename super::template concat<TransTables...>::tuple>;

    using states = empty_group;
    using charset = char_set<>;
};

template <class... Transitions>
struct _init_trans_table_impl;
template <>
struct _init_trans_table_impl<> { using result = trans_table<std::tuple<>>; };
template <class Transition, class... Rest>
struct _init_trans_table_impl<Transition, Rest...> {
    using result = typename _init_trans_table_impl<Rest...>::result::template insert<Transition>;
};
template <class... Transitions>
using init_trans_table = typename _init_trans_table_impl<Transitions...>::result;

/* ---------------------dfa实现--------------------- */

template <class TransTable, class InitialState, class AcceptStates>
struct dfa {
    // 基础数据定义
    using states  = typename TransTable::states;
    using charset = typename TransTable::charset;
    using trans_table = TransTable;
    using initial_state = InitialState;
    using accept_states = AcceptStates;
    using normal_states = typename states::template diff<accept_states>;
    using type = dfa<TransTable, InitialState, AcceptStates>;

    // 转换函数定义
    template <class From, char ch>
    using trans = typename TransTable::template trans<From, ch>;
};

}

#endif // !DFA_H_
