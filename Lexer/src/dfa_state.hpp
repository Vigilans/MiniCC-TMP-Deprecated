#ifndef DFA_STATE_H_
#define DFA_STATE_H_
#include "utility.hpp"
#include <tuple>

namespace cp {

/* ---------------------前置声明--------------------- */

template <size_t... I> 
struct state; // 由Regex AST的结点位置进行编码的状态。

template <template <class Left, class Right> class Compare, class... Elems> 
struct ordered_group; // 状态的二分有序不重复集合。

template <template <class Left, class Right> class Equal, class... Groups>
struct group_list; // 状态组的列表，列表里的组不保证有序且可重复。


/* ---------------------state实现--------------------- */

template <size_t... I>
struct state : public std::index_sequence<I...> {
    using code = std::index_sequence<I...>;
    static_assert(is_ascending<size_t, code>::value);
};

// 按编码的字典序对集合进行比较。
template <class Left, class Right>
using state_compare = lex_compare<size_t, Left::code, Right::code>;

/* ---------------------ordered_group实现--------------------- */

template <template <class, class> class Compare, class... Elems>
struct ordered_group : public std::tuple<Elems...> {
    // 定义基础的type_traits
    template <class... Elems>
    using group = ordered_group<Compare, Elems...>;
    using this_type = group<Elems...>;
    using super = std::tuple<Elems...>;
    using front = std::tuple_element<0, super>;

    template <class Left, class Right>
    constexpr static bool compare_v = Compare<Left, Right>::value;

    // 获取元素数量
    constexpr static auto size() { return sizeof...(Elems); }

    // 合并多个元素或元素集合
    template <class... GroupsOrElems>
    struct _concat_proc;

    template <>
    struct _concat_proc<> { using result = group<Elems...>; };

    template <class... TheseElems, class... Rest> // 合并一个元素集合
    struct _concat_proc<group<TheseElems...>, Rest...> {
        using result = group<Elems..., TheseElems...>::concat<Rest...>;
    };

    template <class ThisElem, class... Rest> // 合并一个元素
    struct _concat_proc<ThisElem, Rest...> {
        using result = group<Elems..., ThisElem>::concat<Rest...>;
    };

    template <class... GroupsOrElems>
    using concat = _concat_proc<GroupsOrElems...>::result;

    // 分解出第一个元素，返回 { 第一个元素，剩下的元素集合 }
    template <class Group = group<Elems...>>
    struct pop_first;

    template <>
    struct pop_first<group<>> : public std::pair<void, group<>> {};

    template <class Elem, class... Rest>
    struct pop_first<group<Elem, Rest...>> : public std::pair<Elem, group<Rest...>> {};

    // 二分法在I处将Sequence切分
    template <size_t I>
    struct _split_proc;

    template <>
    struct _split_proc<0> { using result = std::pair<group<>, group<Elems...>>; };

    template <>
    struct _split_proc<1> {
        using poped = pop_first<group<Elems...>>;
        using result = std::pair<group<poped::first_type>, poped::second_type>;
    };

    template <size_t I>
    struct _split_proc {
        using a = _split_proc<I / 2>::result;
        using b = a::second_type::_split_proc<(I + 1) / 2>::result;
        using result = std::pair<a::first_type::concat<b::first_type>, b::second_type>;
    };

    template <size_t I>
    using split = _split_proc<I>::result;

    // 获取有序元素集的前半与后半部分
    using first_half = split<size() / 2>::first_type;
    using second_half = pop_first<split<size() / 2>::second_type>::second_type;
    using mid_elem = pop_first<split<size() / 2>::second_type>::first_type;

    // 二分法查找指定元素（利用了SFINAE作模式匹配）
    template <class Elem, class = void>
    struct _find_proc;

    template <class Elem> // 空集合则查找失败
    struct _find_proc<Elem, std::enable_if_t<size() == 0>> { using result = void; };

    template <class Elem> // 在左半部分查找
    struct _find_proc<Elem, std::enable_if_t<compare_v<Elem, mid_elem>>> {
        constexpr static int order = 1;
        using result = first_half::_find_proc<Elem>::result;
    };

    template <class Elem> // 在右半部分查找
    struct _find_proc<Elem, std::enable_if_t<compare_v<mid_elem, Elem>>> {
        constexpr static int order = 2;
        using result = second_half::_find_proc<Elem>::result;
    };

    template <class Elem> // 落在这里即有Elem == mid_elem
    struct _find_proc<Elem> {
        constexpr static int order = 3;
        using result = mid_elem; // 中间元素即为查找结果
    };

    // 二分法插入非重的新元素（利用了SFINAE作模式匹配）
    template <class Elem, class = void>
    struct _insert_proc;

    template <class Elem> // 空集合直接插入新元素
    struct _insert_proc<Elem, std::enable_if_t<size() == 0>> { using result = group<Elem>; };

    template <class Elem> // 新元素在左半部分
    struct _insert_proc<Elem, std::enable_if_t<compare_v<Elem, mid_elem>>> {
        constexpr static int order = 1;
        using result = first_half::insert<Elem>::concat<mid_elem, second_half>;
    };

    template <class Elem> // 新元素在右半部分
    struct _insert_proc<Elem, std::enable_if_t<compare_v<mid_elem, Elem>>> {
        constexpr static int order = 2;
        using result = first_half::concat<mid_elem, second_half::insert<Elem>>;
    };

    template <class Elem> // 落在这里即有Elem == mid_elem
    struct _insert_proc<Elem> {
        constexpr static int order = 3;
        using result = group<Elems...>; // 不插入
    };

    template <class Elem> using insert = _insert_proc<Elem>::result;
    template <class Elem> using find = _find_proc<Elem>::result;
    template <class Elem> constexpr static bool has = !std::is_void_v<_find_proc<Elem>::result>;
};

/* ---------------------group_list实现--------------------- */

template <template <class, class> class Equal>
struct group_list<Equal> {
    template <class OtherList>
    using concat = OtherList;

    template <class State>
    using find = void;

    template <class OtherGroup>
    using insert = group_list<Equal, OtherGroup>;
};

template <template <class, class> class Equal, class Group, class... Rest>
struct group_list<Equal, Group, Rest...> {
    template <class... Groups>
    using list = group_list<Equal, Groups...>;

    template <class Left, class Right>
    constexpr static bool equal_v = Equal<Left, Right>;

    template <class OtherList> struct _concat_proc;
    template <class... Groups> struct _concat_proc<list<Groups...>> { using result = list<Group, Rest..., Groups...>; };

    template <class OtherList>
    using concat = _concat_proc<OtherList>::result;

    // 检索第一个包含指定元素的组
    template <class State>
    using first_group_of = std::conditional_t<Group::has<State>, Group, list<Rest...>::first_group_of<State>>;

    // 插入新组，并将新组放置在等价组旁边
    template <class OtherGroup>
    using insert = std::conditional_t<
        equal_v<Group, OtherGroup>,
        list<Group, OtherGroup, Rest...>,
        list<Group>::concat<list<Rest...>::insert<OtherGroup>>>
    >;
};

template <class... States>
using state_group = ordered_group<state_compare, States...>;

using null_state = state<>;
using empty_group = state_group<>;

}

#endif // !DFA_STATE_H_