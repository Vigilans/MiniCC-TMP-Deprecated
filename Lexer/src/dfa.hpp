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

    // 按照源状态和转移条件检索目的状态
    template <class From, char ch>
    using trans = typename super::template find<transition<From, ch, null_state>>::to_state;

    template <class T>
    using insert = trans_table<typename super::template insert<T>::tuple>;

    template <class... TransTables>
    using concat = trans_table<typename super::template concat<TransTables...>::tuple>;

    // 字符集及状态集
    using states  = typename trans_table<std::tuple<Rest...>>::states::template insert<typename Transition::from_state>::template insert<typename Transition::to_state>;
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
    using initial_state = InitialState;
    using accept_states = AcceptStates;
    using normal_states = typename states::template diff<accept_states>;
    using type = dfa<TransTable, InitialState, AcceptStates>;

    // 转换函数定义
    template <class From, char ch>
    using trans = typename TransTable::template trans<From, ch>;
};

/* ---------------------min_dfa算法实现--------------------- */

template <class DFA> 
struct _min_dfa_impl {

    // 划分细分阶段
    template <class... Groups> // 存储一个划分的各小组的列表
    using division = group_list<std::tuple<Groups...>, std::is_same, empty_group>;

    template <class Division, bool recursive>
    struct split_to_atom; // 持续细分划分，直到不能再细分为止
    template <class Division>
    struct split_to_atom<Division, false> { using result = Division; }; // 停止递归时，直接返回当前划分
    template <class... Groups>
    struct split_to_atom<division<Groups...>, true> { // 主递归过程
        using cur_division = division<Groups...>;

        // 由当前划分产生新的划分
        template <class... Groups>
        struct split_division;
        template <>
        struct split_division<> { using result = division<>; };
        template <class ThisGroup, class... RestGroups>
        struct split_division<ThisGroup, RestGroups...> {
            // 对 pair<源状态 --(字符集)--> 目标集合> 进行判等。只判断映射后目标集合是否相等。相等的Pair会被插在一起。
            template <class Left, class Right>
            struct type_pair_group_equal : std::is_same<typename Left::second, typename Right::second> {};

            template <class... Pairs> // 存储pair<源状态 --(字符集)--> 目标集合>的列表
            using pair_list = group_list<std::tuple<Pairs...>, type_pair_group_equal, type_pair<null_state, empty_group>>;

            // 获取Group中所有状态的pair<源状态 --(字符集)--> 目标集合>的映射列表。
            template <class GroupTuple> 
            struct map;
            template <> 
            struct map<std::tuple<>> { using result = pair_list<>; };
            template <class ThisState, class... RestStates>
            struct map<std::tuple<ThisState, RestStates...>> {
                template <class char_set> struct map_charset;
                template <char... chs> struct map_charset<char_set<chs...>> {
                    using result = state_group<typename DFA::template trans<ThisState, chs>...>; // 这里没有使用state_group的有序性质，但顺序有意义
                };

                using this_mapped_pair = type_pair<ThisState, typename map_charset<typename DFA::charset>::result>;
                using rest_mapped_pairs = typename map<std::tuple<RestStates...>>::result;
                using result = typename rest_mapped_pairs::template insert<this_mapped_pair>;
            };

            // 将pair<源状态 --(字符集)--> 目标集合>列表压缩至新的几个小组，得到新的分划Π
            template <class PairList>
            struct reduce;
            template <>
            struct reduce<pair_list<>>     { using result = division<>; };
            template <class Pair>
            struct reduce<pair_list<Pair>> { using result = division<state_group<typename Pair::first>>; };
            template <class ThisPair, class Neighbor, class... RestPairs>
            struct reduce<pair_list<ThisPair, Neighbor, RestPairs...>> {
                using rest_groups = typename reduce<pair_list<Neighbor, RestPairs...>>::result;
                using near_merged = typename rest_groups::front::template insert<typename ThisPair::first>; // 与邻居合并
                using independent = typename state_group<typename ThisPair::first>; // 当前状态自成一组
                using result = std::conditional_t<
                    std::is_same_v<typename ThisPair::second, typename Neighbor::second>,
                    typename division<near_merged>::template concat<typename rest_groups::rest>, // 若目标状态集合相等，则插进相邻组
                    typename division<independent>::template concat<rest_groups> // 否则，新建一个组
                >;
            };

            using new_this_group  = typename reduce<typename map<typename ThisGroup::reverse::tuple>::result>::result; // reverse::tuple是为了逆序遍历
            using new_rest_groups = typename split_division<RestGroups...>::result;
            using result = typename new_this_group::template concat<new_rest_groups>;
        };

        using new_division = typename split_division<Groups...>::result;

        // 当新划分与原划分相等时，停止递归
        using result = typename split_to_atom<new_division, !std::is_same_v<new_division, cur_division>>::result;
    };

    // 获取最终的不可再细分的划分（初始划分为S-F与F）
    using final_division = typename split_to_atom<division<typename DFA::normal_states, typename DFA::accept_states>, true>::result;

    // 映射阶段，映射参数至一组代表状态
    template <class ListOrGroup> struct map_rep;
    template <> struct map_rep<division<>>  { using result = empty_group; };
    template <> struct map_rep<empty_group> { using result = empty_group; };
    template <class Group, class... Rest> struct map_rep<division<Group, Rest...>> {    // 映射List中每个Group至它们的代表状态
        using result = typename map_rep<division<Rest...>>::result::template insert<typename Group::front>; // 同组的代表元素不会重复插入
    };
    template <class State, class... Rest> struct map_rep<state_group<State, Rest...>> { // 映射Group中的每个状态至它们所在组的代表状态
        using rep_state = typename final_division::template first_group_of<State>::front;
        using result    = typename map_rep<state_group<Rest...>>::result::template insert<rep_state>;
    };
    
    // 构建新转换表阶段
    template <class... RepStates> struct set_trans_table { using result = trans_table<>; };
    template <class RepState, class... Rest> struct set_trans_table<state_group<RepState, Rest...>> {
        // 对状态作一次字符表映射，每次映射结果存进新转换表中
        template <class char_set> struct map_charset { using result = trans_table<>; };
        template <char ch, char... rest> struct map_charset<char_set<ch, rest...>> {
            using dest_state = typename DFA::template trans<RepState, ch>;
            using dest_rep   = typename final_division::template first_group_of<dest_state>::front;
            using new_trans  = transition<RepState, ch, dest_rep>;
            using result = typename map_charset<char_set<rest...>>::result::template insert<new_trans>;
        };
    
        // 由于transtion的排序以源状态为第一优先级, RepStates又是有序的，故直接拼接的结果也是有序的
        using this_table = typename map_charset<typename DFA::charset>::result;
        using rest_table = typename set_trans_table<state_group<Rest...>>::result;
        using result = typename this_table::template concat<rest_table>;
    };
    
    // 构建min_dfa阶段
    struct build {
        using rep_initial = typename final_division::template first_group_of<typename DFA::initial_state>::front;
        using rep_accepts = typename map_rep<typename DFA::accept_states>::result;
        using rep_states  = typename map_rep<final_division>::result;
        using trans_table = typename set_trans_table<rep_states>::result;
        using result = dfa<trans_table, rep_initial, rep_accepts>;
    };
};

template <class DFA>
using min_dfa = typename _min_dfa_impl<DFA>::build::result;

}

#endif // !DFA_H_
