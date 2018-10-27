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
    !std::is_same_v<Left::from_state, Right::from_state>,
    state_compare<Left::from_state, Right::from_state>,
    std::bool_constant<(Left::ch < Right::ch)>
> {};

/* ---------------------trans_table实现--------------------- */

// 一个状态机的转换表，继承自有序集合，可以O(log(N))复杂度匹配转换。
// 提供了转移检索、字符/状态集获取功能。
template <class... Transitions>
struct trans_table;

template <>
struct trans_table<> : public ordered_group<trans_compare> {
    template <class From, char ch>
    using trans   = void;
    using states  = empty_group;
    using charset = char_set<>;
};

template <class Transition, class... Rest>
struct trans_table<Transition, Rest...> : public ordered_group<trans_compare, Transitions...> {
    // type-traits定义
    using super = ordered_group<trans_compare, Transitions...>;

    // 按照源状态和转移条件检索目的状态
    template <class From, char ch>
    using trans = super::find<transition<From, ch, void>>::to_state;

    // 字符集及状态集
    using states  = trans_table<Rest...>::states::insert<Transition::From>::insert<Transition::To>;
    using charset = trans_table<Rest...>::charset::insert<ch>;
};

/* ---------------------dfa实现--------------------- */

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
    using trans = TransTable::trans<From, ch>;

private:
    /* ---------------------min_dfa算法实现--------------------- */
    // 组分割阶段
    template <class... Groups>
    struct _min_dfa_proc_split {
        template <class... Divisions> // 存储划分的列表
        using division_list = group_list<std::is_same, Divisions...>;

        using cur_groups = division_list<Groups...>;
        
        // 由当前划分集合产生新的划分集合
        template <class... Divisions> struct _new_groups_proc;
        template <> struct _new_groups_proc<> { using result = division_list<>; };
        template <class Division, class... Rest>
        struct _new_groups_proc<Division, Rest...> {
            // 对 pair<状态, 状态经字符集转换映射后的集合> 进行判等。只判断映射后集合是否相等。
            template <class Left, class Right>
            struct state_group_equal : std::is_same<Left::second_type, Right::second_type> {};

            template <class... Pairs> // 存储状态-集合对的列表
            using pair_list = group_list<state_group_equal, Pairs...>;

            // 将当前划分的各状态通过字符集映射至<源状态, 目标状态集合>的列表。
            template <class Division> struct pairs_proc;
            template <> struct pairs_proc<empty_group> { using result = pair_list<>; };
            template <class State, class... SRest>
            struct pairs_proc<state_group<State, SRest...>> {
                template <class char_set> struct map_charset;
                template <char... chs> struct map_charset {
                    using result = state_group<trans<State, chs>...>;
                };

                using state_group_pair = std::pair<State, map_charset<charset>::result>;
                using result = pairs_proc<state_group<SRest...>>::result::insert<state_group_pair>;
            };

            // 将<原状态, 目标状态集合>的列表压缩成新的几个小划分
            template <class PairList> struct compress_proc;
            template <> struct compress_proc<pair_list<>> { using result = division_list<>; };
            template <class Pair> struct compress_proc<pair_list<Pair>> { using result = division_list<state_group<Pair::first_type>>; };
            template <class Pair, class Neighbor, class... PRest>
            struct compress_proc<pair_list<Pair, Neighbor, PRest...>> {
                using divisions = compress_proc<pair_list<Neighbor, PRest...>>;
                using result = std::conditional_t<
                    std::is_same_v<Pair::second_type, Neighbor::second_type>,
                    concat_list<divisions::front::insert<Pair::first_type>, divisions::rest>, // 若目标状态集合相等，则插进相邻组
                    concat_list<state_group<Pair::first_type>, divisions> // 否则，新建一个组
                >;
            };

            using result = compress_proc<pairs_proc<Division>::result>::result::concat<_new_groups_proc<Rest...>>;
        };

        using new_groups = _new_groups_proc<Groups...>;

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
        using rep_state = _division_list::first_group_of<State>::front;
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
            using dest_rep = _division_list::first_group_of<trans<RepState, ch>>::front;
            using new_trans = transition<RepState, ch, dest_rep>;
            using result = map_charset<rest...>::result::insert<new_trans>;
        };

        // 由于transtion的排序以源状态为第一优先级, RepStates又是有序的，故直接拼接的结果也是有序的
        using result = map_charset<charset>::result::concat<_min_dfa_proc_trans<Rest...>::result>;
    };

    // 构建min_dfa阶段
    struct _min_dfa_proc_build {
        using rep_intial  = _division_list::first_group_of<initial_state>::front;
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
