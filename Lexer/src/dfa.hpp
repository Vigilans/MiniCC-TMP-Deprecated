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

template <>
struct trans_table<std::tuple<>> : public ordered_group<std::tuple<>, trans_compare, null_transition> {
    template <class From, char ch>
    using trans   = null_state;

    template <class T>
    using insert  = trans_table<std::tuple<T>>;

    using states  = empty_group;
    using charset = char_set<>;
};

template <class Transition, class... Rest>
struct trans_table<std::tuple<Transition, Rest...>> : public ordered_group<
    std::tuple<Transition, Rest...>, trans_compare, null_transition
> {
    // type-traits定义
    using super = ordered_group<std::tuple<Transition, Rest...>, trans_compare, null_transition>;

    // 按照源状态和转移条件检索目的状态
    template <class From, char ch>
    using trans = typename super::template find<transition<From, ch, null_state>>::to_state;

    template <class T>
    using insert = trans_table<typename super::template insert<T>::tuple>;

    // 字符集及状态集
    using states  = typename trans_table<std::tuple<Rest...>>::states::template insert<typename Transition::from_state>::template insert<typename Transition::to_state>;
    using charset = typename trans_table<std::tuple<Rest...>>::charset::template insert<Transition::cond>;
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

    // 转换函数定义
    template <class From, char ch>
    using trans = typename TransTable::template trans<From, ch>;

public:
    /* ---------------------min_dfa算法实现--------------------- */
    // 组分割阶段
    template <class... Divisions> // 存储划分的列表
    using division_list = group_list<std::tuple<Divisions...>, std::is_same, empty_group>;

    template <class... Groups>
    struct _min_dfa_impl_split {
        using cur_groups = division_list<Groups...>;
        
        // 由当前划分集合产生新的划分集合
        template <class... Divisions> 
        struct _new_groups_impl;
        template <> 
        struct _new_groups_impl<> { using result = division_list<>; };
        template <class Division, class... Rest>
        struct _new_groups_impl<Division, Rest...> {
            // 对 pair<状态, 状态经字符集转换映射后的集合> 进行判等。只判断映射后集合是否相等。相等的Pair会被插在一起。
            template <class Left, class Right>
            struct state_group_equal : std::is_same<typename Left::second_type, typename Right::second_type> {};

            template <class... Pairs> // 存储状态-集合对的列表
            using pair_list = group_list<std::tuple<Pairs...>, state_group_equal, std::pair<null_state, empty_group>>;

            // 将当前划分的各状态通过字符集映射至<源状态, 目标状态集合>的列表。
            template <class Division> 
            struct pairs_impl;
            template <> 
            struct pairs_impl<empty_group> { using result = pair_list<>; };
            template <class State, class... SRest>
            struct pairs_impl<state_group<State, SRest...>> {
                template <class char_set> struct map_charset;
                template <char... chs> struct map_charset<char_set<chs...>> {
                    using result = state_group<trans<State, chs>...>; // 这里没有使用state_group的有序性质，但顺序有意义
                };

                using state_group_pair = std::pair<State, typename map_charset<charset>::result>;
                using result = typename pairs_impl<state_group<SRest...>>::result::template insert<state_group_pair>;
            };

            // 将<原状态, 目标状态集合>的列表压缩成新的几个小划分
            template <class PairList> 
            struct compress_impl;
            template <> 
            struct compress_impl<pair_list<>> { using result = division_list<>; };
            template <class Pair> 
            struct compress_impl<pair_list<Pair>> { using result = division_list<state_group<typename Pair::first_type>>; };
            template <class Pair, class Neighbor, class... PRest>
            struct compress_impl<pair_list<Pair, Neighbor, PRest...>> {
                using divisions = typename compress_impl<pair_list<Neighbor, PRest...>>::result;
                using combined_group   = typename divisions::front::template insert<typename Pair::first_type>;
                using individual_group = typename state_group<typename Pair::first_type>;
                using result = std::conditional_t<
                    std::is_same_v<typename Pair::second_type, typename Neighbor::second_type>,
                    typename division_list<combined_group  >::template concat<typename divisions::rest>, // 若目标状态集合相等，则插进相邻组
                    typename division_list<individual_group>::template concat<divisions> // 否则，新建一个组
                >;
            };
            
            using mapped_pairs   = typename pairs_impl<Division>::result;
            using new_divisions  = typename compress_impl<mapped_pairs>::result;
            using rest_divisions = typename _new_groups_impl<Rest...>::result;
            using result = typename new_divisions::template concat<rest_divisions>::result;
        };

        using new_groups = typename _new_groups_impl<Groups...>::result;

        using result = std::conditional_t<
            std::is_same_v<new_groups, cur_groups>,
            new_groups, typename _min_dfa_impl_split<new_groups>::result
        >;
    };

    using _division_list = typename _min_dfa_impl_split<normal_states, accept_states>::result;

//    // 映射阶段
//    template <class ListOrGroup> struct _min_dfa_impl_rep;
//    template <> struct _min_dfa_impl_rep<division_list<>> { using result = empty_group; };
//    template <> struct _min_dfa_impl_rep<empty_group> { using result = empty_group; };
//    template <class Group, class... Rest> struct _min_dfa_impl_rep<division_list<Group, Rest...>> { // 映射List中每个Group至它们的代表状态
//        using result = typename _min_dfa_impl_rep<division_list<Rest...>>::result::template insert<typename Group::front>;
//    };
//    template <class State, class... Rest> struct _min_dfa_impl_rep<state_group<State, Rest...>> { // 映射Group中的每个状态至它们所在组的代表状态
//        using rep_state = typename _division_list::template first_group_of<State>::front;
//        using result    = typename _min_dfa_impl_rep<state_group<Rest...>>::result::template insert<rep_state>;
//    };
//
//    // 构建新转换表阶段
//    template <class RepStates> struct _min_dfa_impl_trans;
//    template <>                struct _min_dfa_impl_trans<empty_group> { using result = trans_table<>; };
//    template <class RepState, class... Rest> struct _min_dfa_impl_trans<state_group<RepState, Rest...>> {
//        // 对状态作一次字符表映射，每次映射结果存进新转换表中
//        template <class char_set> struct map_charset;
//        template <> struct map_charset<char_set<>> { using result = trans_table<>; };
//        template <char ch, char... rest>
//        struct map_charset<char_set<ch, rest...>> {
//            using dest_rep = typename _division_list::template first_group_of<trans<RepState, ch>>::front;
//            using new_trans = transition<RepState, ch, dest_rep>;
//            using result = typename map_charset<rest...>::result::template insert<new_trans>;
//        };
//
//        // 由于transtion的排序以源状态为第一优先级, RepStates又是有序的，故直接拼接的结果也是有序的
//        using result = typename map_charset<charset>::result::template concat<typename _min_dfa_impl_trans<Rest...>::result>;
//    };
//
//    // 构建min_dfa阶段
//    struct _min_dfa_impl_build {
//        using rep_intial  = typename _division_list::template first_group_of<initial_state>::front;
//        using rep_accepts = typename _min_dfa_impl_rep<accept_states>::result;
//        using rep_states  = typename _min_dfa_impl_rep<_division_list>::result;
//        using trans_table = typename _min_dfa_impl_trans<rep_states>::result;
//        using result = dfa<trans_table, rep_intial, rep_accepts>;
//    };
//
//public:
//    // 获取最小化DFA
//    using minimum = typename _min_dfa_impl_build::result;
};

}

#endif // !DFA_H_
