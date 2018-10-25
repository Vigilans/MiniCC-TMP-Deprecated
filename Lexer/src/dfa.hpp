#ifndef DFA_H_
#define DFA_H_
#include "dfa_state.hpp"

namespace cp {

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

template <class TransTable, class InitialState, class AcceptStates>
struct dfa {
    // 基础数据定义
    using states  = TransTable::states;
    using charset = TransTable::charset;
    using initial_state = InitialState;
    using accept_states = AcceptStates;
    using normal_states = states::diff<accept_states>;

    // 转换函数定义
    template <class From, char ch>
    using trans = TransTable::get<From, ch>;
    
private: /* ---------------------min_dfa算法实现--------------------- */
    // 组分割阶段
    template <class... Groups>
    struct _min_dfa_proc_split {
        using cur_groups = group_list<Groups...>;

        using new_groups = group_list<>;

        using result = std::conditional_t<
            std::is_same_v<new_groups, cur_groups>,
            new_groups, _min_dfa_proc_split<new_groups>::result
        >;
    };

    using _division_list = _min_dfa_proc_split<normal_states, accept_states>::result;

    // 映射阶段
    template <class ListOrGroup> struct _min_dfa_proc_rep;

    template <> struct _min_dfa_proc_rep<group_list<>> { using result = empty_group; };

    template <> struct _min_dfa_proc_rep<empty_group> { using result = empty_group; };

    template <class Group, class... Rest> // 映射List中每个Group至它们的代表状态
    struct _min_dfa_proc_rep<group_list<Group, Rest...>> { 
        using result = _min_dfa_proc_rep<group_list<Rest...>>::result::insert<Group::front>;
    };

    template <class State, class... Rest> // 映射Group中的每个状态至它们所在组的代表状态
    struct _min_dfa_proc_rep<state_group<State, Rest...>> {
        using rep_state = _division_list::find<State>::front;
        using result = _min_dfa_proc_rep<state_group<Rest...>>::result::insert<rep_state>;
    };

    // 构建新转换表阶段
    template <class RepStates>
    struct _min_dfa_proc_trans;

    template <>
    struct _min_dfa_proc_trans<empty_group> { using result = trans_table<>; };

    template <class RepState, class... Rest>
    struct _min_dfa_proc_trans<state_group<RepState, Rest...>> {
        // 对状态作一次字符表映射，每次映射结果存进新转换表中
        template <class char_set> struct map_charset;
            
        template <> struct map_charset<char_set<>> { using result = trans_table<>; };

        template <char ch, char... rest>
        struct map_charset<char_set<ch, rest...>> {
            using dest_rep = _division_list::find<trans<RepState, ch>>;
            using new_trans = transition<RepState, dest_rep, ch>;
            using result = map_charset<rest...>::result::insert<new_trans>;
        };

        // 由于transtion的排序以源状态为第一优先级, RepStates又是有序的，故直接拼接的结果也是有序的
        using result = map_charset<charset>::result::concat<_min_dfa_proc_trans<Rest...>::result>;
    };

    // 构建min_dfa阶段
    struct _min_dfa_proc_build {
        using rep_intial  = _division_list::find<initial_state>::front;
        using rep_accepts = _min_dfa_proc_rep<accept_states>::result;
        using rep_states  = _min_dfa_proc_rep<_division_list>::result;
        using trans_table = _min_dfa_proc_trans<rep_states>::result;
        using result = dfa<trans_table, rep_intial, rep_accepts>;
    };

public:
    // 获取最小化DFA
    using minimum = _min_dfa_proc_build::result;
};

}

#endif // !DFA_H_
