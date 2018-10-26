#ifndef DFA_STATE_H_
#define DFA_STATE_H_
#include "utility.hpp"
#include <tuple>

namespace cp {

/* ---------------------ǰ������--------------------- */

template <size_t... I> 
struct state; // ��Regex AST�Ľ��λ�ý��б����״̬��

template <template <class Left, class Right> class Compare, class... Elems> 
struct ordered_group; // ״̬�Ķ��������ظ����ϡ�

template <template <class Left, class Right> class Equal, class... Groups>
struct group_list; // ״̬����б��б�����鲻��֤�����ҿ��ظ���


/* ---------------------stateʵ��--------------------- */

template <size_t... I>
struct state : public std::index_sequence<I...> {
    using code = std::index_sequence<I...>;
    static_assert(is_ascending<size_t, code>::value);
};

// ��������ֵ���Լ��Ͻ��бȽϡ�
template <class Left, class Right>
using state_compare = lex_compare<size_t, Left::code, Right::code>;

/* ---------------------ordered_groupʵ��--------------------- */

template <template <class, class> class Compare, class... Elems>
struct ordered_group : public std::tuple<Elems...> {
    // ���������type_traits
    template <class... Elems>
    using group = ordered_group<Compare, Elems...>;
    using this_type = group<Elems...>;
    using super = std::tuple<Elems...>;
    using front = std::tuple_element<0, super>;

    template <class Left, class Right>
    constexpr static bool compare_v = Compare<Left, Right>::value;

    // ��ȡԪ������
    constexpr static auto size() { return sizeof...(Elems); }

    // �ϲ����Ԫ�ػ�Ԫ�ؼ���
    template <class... GroupsOrElems>
    struct _concat_proc;

    template <>
    struct _concat_proc<> { using result = group<Elems...>; };

    template <class... TheseElems, class... Rest> // �ϲ�һ��Ԫ�ؼ���
    struct _concat_proc<group<TheseElems...>, Rest...> {
        using result = group<Elems..., TheseElems...>::concat<Rest...>;
    };

    template <class ThisElem, class... Rest> // �ϲ�һ��Ԫ��
    struct _concat_proc<ThisElem, Rest...> {
        using result = group<Elems..., ThisElem>::concat<Rest...>;
    };

    template <class... GroupsOrElems>
    using concat = _concat_proc<GroupsOrElems...>::result;

    // �ֽ����һ��Ԫ�أ����� { ��һ��Ԫ�أ�ʣ�µ�Ԫ�ؼ��� }
    template <class Group = group<Elems...>>
    struct pop_first;

    template <>
    struct pop_first<group<>> : public std::pair<void, group<>> {};

    template <class Elem, class... Rest>
    struct pop_first<group<Elem, Rest...>> : public std::pair<Elem, group<Rest...>> {};

    // ���ַ���I����Sequence�з�
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

    // ��ȡ����Ԫ�ؼ���ǰ�����벿��
    using first_half = split<size() / 2>::first_type;
    using second_half = pop_first<split<size() / 2>::second_type>::second_type;
    using mid_elem = pop_first<split<size() / 2>::second_type>::first_type;

    // ���ַ�����ָ��Ԫ�أ�������SFINAE��ģʽƥ�䣩
    template <class Elem, class = void>
    struct _find_proc;

    template <class Elem> // �ռ��������ʧ��
    struct _find_proc<Elem, std::enable_if_t<size() == 0>> { using result = void; };

    template <class Elem> // ����벿�ֲ���
    struct _find_proc<Elem, std::enable_if_t<compare_v<Elem, mid_elem>>> {
        constexpr static int order = 1;
        using result = first_half::_find_proc<Elem>::result;
    };

    template <class Elem> // ���Ұ벿�ֲ���
    struct _find_proc<Elem, std::enable_if_t<compare_v<mid_elem, Elem>>> {
        constexpr static int order = 2;
        using result = second_half::_find_proc<Elem>::result;
    };

    template <class Elem> // �������Ｔ��Elem == mid_elem
    struct _find_proc<Elem> {
        constexpr static int order = 3;
        using result = mid_elem; // �м�Ԫ�ؼ�Ϊ���ҽ��
    };

    // ���ַ�������ص���Ԫ�أ�������SFINAE��ģʽƥ�䣩
    template <class Elem, class = void>
    struct _insert_proc;

    template <class Elem> // �ռ���ֱ�Ӳ�����Ԫ��
    struct _insert_proc<Elem, std::enable_if_t<size() == 0>> { using result = group<Elem>; };

    template <class Elem> // ��Ԫ������벿��
    struct _insert_proc<Elem, std::enable_if_t<compare_v<Elem, mid_elem>>> {
        constexpr static int order = 1;
        using result = first_half::insert<Elem>::concat<mid_elem, second_half>;
    };

    template <class Elem> // ��Ԫ�����Ұ벿��
    struct _insert_proc<Elem, std::enable_if_t<compare_v<mid_elem, Elem>>> {
        constexpr static int order = 2;
        using result = first_half::concat<mid_elem, second_half::insert<Elem>>;
    };

    template <class Elem> // �������Ｔ��Elem == mid_elem
    struct _insert_proc<Elem> {
        constexpr static int order = 3;
        using result = group<Elems...>; // ������
    };

    template <class Elem> using insert = _insert_proc<Elem>::result;
    template <class Elem> using find = _find_proc<Elem>::result;
    template <class Elem> constexpr static bool has = !std::is_void_v<_find_proc<Elem>::result>;
};

/* ---------------------group_listʵ��--------------------- */

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

    // ������һ������ָ��Ԫ�ص���
    template <class State>
    using first_group_of = std::conditional_t<Group::has<State>, Group, list<Rest...>::first_group_of<State>>;

    // �������飬������������ڵȼ����Ա�
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