#ifndef REGEX_DFA_H_
#define REGEX_DFA_H_
#include "utility.hpp"
#include <tuple>

namespace cp {

/* ---------------------前置声明--------------------- */

template <size_t... I> struct state; // 由Regex AST的结点位置进行编码的状态。
template <class... States> struct state_group; // 状态的集合，集合里的状态有序且不重复。
template <class... Groups> struct group_list; // 状态组的列表，列表里的组不保证有序且可重复。

using null_state = state<>;
using empty_group = state_group<>;

/* ---------------------state实现--------------------- */

template <size_t... I>
struct state : std::index_sequence<I...> {
    using code = std::index_sequence<I...>;
    static_assert(is_ascending<size_t, code>::value);
};

// 按编码的字典序对集合进行比较。
template <class Left, class Right>
constexpr bool state_compare_v = lex_compare<size_t, Left::code, Right::code>::value;

/* ---------------------state_group实现--------------------- */

template <class... States>
struct state_group : std::tuple<States...> {
    // 定义基础的type_traits
    using this_type = state_group<States...>;
    using super = std::tuple<States...>;
    using front = std::tuple_element<0, super>;
    
    // 获取元素数量
    constexpr static auto size() { return sizeof...(States); }

    // 合并多个状态或状态集合
    template <class... GroupsOrStates>
    struct _concat_proc;

    template <>
    struct _concat_proc<> { using result = state_group<States...>; };

    template <class... TheseStates, class... Rest> // 合并一个状态集合
    struct _concat_proc<state_group<TheseStates...>, Rest...> {
        using result = state_group<States..., TheseStates...>::concat<Rest...>;
    };

    template <class ThisState, class... Rest> // 合并一个状态
    struct _concat_proc<ThisState, Rest...> {
        using result = state_group<States..., ThisState>::concat<Rest...>;
    };

    template <class... GroupsOrStates>
    using concat = _concat_proc<GroupsOrStates...>::result;

    // 分解出第一个状态，返回 { 第一个状态，剩下的状态集合 }
    template <class Group = state_group<States...>>
    struct pop_first;

    template <>
    struct pop_first<state_group<>> : std::pair<null_state, empty_group> {};

    template <class State, class... Rest>
    struct pop_first<state_group<State, Rest...>> : std::pair<State, state_group<Rest...>> {};

    // 二分法在I处将Sequence切分
    template <size_t I>
    struct _split_proc;

    template <>
    struct _split_proc<0> { using result = std::pair<state_group<>, state_group<States...>>; };

    template <>
    struct _split_proc<1> {
        using poped = pop_first<state_group<States...>>;
        using result = std::pair<state_group<poped::first_type>, poped::second_type>;
    };

    template <size_t I>
    struct _split_proc {
        using a = _split_proc<I/2>::result;
        using b = a::second_type::_split_proc<(I+1)/2>::result;
        using result = std::pair<a::first_type::concat<b::first_type>, b::second_type>;
    };

    template <size_t I>
    using split = _split_proc<I>::result;

    // 获取有序状态集的前半与后半部分
    using first_half = split<size() / 2>::first_type;
    using second_half = pop_first<split<size() / 2>::second_type>::second_type;
    using mid_state = pop_first<split<size() / 2>::second_type>::first_type;
    
    // 二分法检查组里是否含有指定状态（利用了三目运算符的短路）
    template <class State>
    static constexpr bool has_state = size() == 0 ? false :
        std::is_same_v<State, mid_state>  ? true : 
        state_compare_v<State, mid_state> ? first_half::has_state<State> : second_half::has_state<State>;

    // 二分法插入非重新状态（利用了SFINAE作模式匹配）
    template <class State, class = void>
    struct _insert_proc;

    template <class State> // 空集合直接插入新状态
    struct _insert_proc<State, std::enable_if_t<size() == 0>> { using result = state_group<State>; };

    template <class State> // 新状态在左半部分
    struct _insert_proc<State, std::enable_if_t<state_compare_v<State, mid_state>>> { 
        using result = first_half::insert<State>::concat<mid_state, second_half>; 
    };

    template <class State> // 新状态在右半部分
    struct _insert_proc<State, std::enable_if_t<state_compare_v<mid_state, State>>> { 
        using result = first_half::concat<mid_state, second_half::insert<State>>; 
    };

    template <class State> // 一般来说最后一定会落在这里
    struct _insert_proc<State, std::enable_if_t<std::is_same_v<mid_state, State>>> { 
        using result = state_group<States...>; // 不插入
    };
    
    template <class State>
    using insert = _insert_proc<State>::result;
};

/* ---------------------group_list实现--------------------- */

template <>
struct group_list<> {
    template <class State>
    using find = empty_group;

    using front = void;

    //template <class NewGroup>
    //using insert = group_list<NewGroup>;
};

template <class Group, class... Rest>
struct group_list<Group, Rest...> {
    // 检索第一个包含指定状态的组
    template <class State>
    using find = std::conditional_t<
        Group::has_state<State>, Group, group_list<Rest...>::template find<State>
    >;

    // 获取第一个组
    using front = Group;

    //// 插入新组，并将新组放置在等价组旁边
    //template <class NewGroup>
    //using insert = std::conditional_t<
    //    std::is_same_v<Group, NewGroup>,
    //    group_list<NewGroup, Group, Rest...>,
    //    concat<group_list<Group>, group_list<Rest...>::insert<NewGroup>>
    //>;
};

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
            new_groups, _min_dfa_proc<new_groups>::_division_list
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

#endif // !REGEX_DFA_H_
