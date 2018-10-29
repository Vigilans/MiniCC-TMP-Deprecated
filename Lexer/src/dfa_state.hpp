#ifndef DFA_STATE_H_
#define DFA_STATE_H_
#include "utility.hpp"
#include <tuple>

namespace cp {

/* ---------------------ǰ������--------------------- */

template <size_t... I> 
struct state; // ��Regex AST�Ľ��λ�ý��б����״̬��

template <class ElemTuple, template <class Left, class Right> class Compare, class NullElem = void>
struct ordered_group; // ״̬�Ķ��������ظ����ϡ�

template <class GroupTuple = std::tuple<>, template <class Left, class Right> class Equal = std::is_same, class EmptyGroup = void>
struct group_list; // ״̬����б��б�����鲻��֤�����ҿ��ظ���


/* ---------------------stateʵ��--------------------- */

template <size_t... I>
struct state : public std::index_sequence<I...> {
    using code = std::index_sequence<I...>;
    static_assert(is_ascending<size_t, code>::value);
};

// ��������ֵ���Լ��Ͻ��бȽϡ�
template <class Left, class Right>
using state_compare = lex_compare<size_t, typename Left::code, typename Right::code>;

/* ---------------------ordered_groupʵ��--------------------- */

template <template <class, class> class Compare, class NullElem>
struct ordered_group<std::tuple<>, Compare, NullElem> {
    using null_elem = NullElem;
    using tuple = std::tuple<>;
    using front = null_elem;
    using rest  = ordered_group<std::tuple<>, Compare, NullElem>;
    using type  = ordered_group<std::tuple<>, Compare, NullElem>;
};

template <class This, class... Rest, template <class, class> class Compare, class NullElem>
struct ordered_group<std::tuple<This, Rest...>, Compare, NullElem> {
    // ���������type_traits
    template <class... Elems>
    using group = ordered_group<std::tuple<Elems...>, Compare, NullElem>;
    using null_elem = NullElem;
    using tuple = std::tuple<>;
    using front = This;
    using rest  = group<Rest...>;
    using type  = group<This, Rest...>;

    template <class Left, class Right>
    constexpr static bool compare_v = Compare<Left, Right>::value;

    // ��ȡԪ������
    constexpr static auto size = sizeof...(Elems);

    // �ϲ����Ԫ�ػ�Ԫ�ؼ���
    template <class... GroupsOrElems>
    struct _concat_impl;
    template <>
    struct _concat_impl<> { using result = group<Elems...>; };
    template <class... TheseElems, class... Rest> struct _concat_impl<group<TheseElems...>, Rest...> { // �ϲ�һ��Ԫ�ؼ���
        using result = typename group<Elems..., TheseElems...>::template _concat_impl<Rest...>::result;
    };
    template <class ThisElem, class... Rest> struct _concat_impl<ThisElem, Rest...> { // �ϲ�һ��Ԫ��
        using result = typename group<Elems..., ThisElem>::template _concat_impl<Rest...>::result;
    };
    template <class... GroupsOrElems>
    using concat = typename _concat_impl<GroupsOrElems...>::result;

    template <class Other>
    using diff = ordered_group<tuple_diff<tuple, typename Other::tuple, Compare>, Compare, NullElem>;

    // �ֽ����һ��Ԫ�أ����� { ��һ��Ԫ�أ�ʣ�µ�Ԫ�ؼ��� }
    template <class Group = group<Elems...>>
    struct pop_first;
    template <>
    struct pop_first<group<>> : std::pair<null_elem, group<>> {};
    template <class Elem, class... Rest>
    struct pop_first<group<Elem, Rest...>> : std::pair<Elem, group<Rest...>> {};

    // ���ַ���I����Sequence�з�
    template <size_t I> struct _split_impl;
    template <> struct _split_impl<0> { using result = std::pair<group<>, group<Elems...>>; };
    template <> struct _split_impl<1> {
        using poped = pop_first<group<Elems...>>;
        using result = std::pair<group<typename poped::first_type>, typename poped::second_type>;
    };
    template <size_t I> struct _split_impl {
        using a = typename _split_impl<I / 2>::result;
        using b = typename a::second_type::template _split_impl<(I + 1) / 2>::result;
        using result = std::pair<typename a::first_type::template concat<typename b::first_type>, typename b::second_type>;
    };
    template <size_t I>
    using split = typename _split_impl<I>::result;

    // ��ȡ����Ԫ�ؼ���ǰ�����벿��
    using first_half  = typename split<size / 2>::first_type;
    using second_half = typename pop_first<typename split<size / 2>::second_type>::second_type;
    using mid_elem    = typename pop_first<typename split<size / 2>::second_type>::first_type;

    // ���ַ�����ָ��Ԫ�أ�������SFINAE��ģʽƥ�䣩
    template <class Elem, class = void> struct _find_impl;
    template <class Elem> struct _find_impl<Elem, std::enable_if_t<size == 0>> { // �ռ���
        using result = null_elem; // ����ʧ��
    }; 
    template <class Elem> struct _find_impl<Elem, std::enable_if_t<compare_v<Elem, mid_elem>>> { // ����벿�ֲ���
        using result = typename first_half::template _find_impl<Elem>::result;
    };
    template <class Elem> struct _find_impl<Elem, std::enable_if_t<compare_v<mid_elem, Elem>>> { // ���Ұ벿�ֲ���
        using result = typename second_half::template _find_impl<Elem>::result;
    };
    template <class Elem> struct _find_impl<Elem> { // �������Ｔ��Elem == mid_elem
        using result = mid_elem; // �м�Ԫ�ؼ�Ϊ���ҽ��
    };

    // ���ַ�������ص���Ԫ�أ�������SFINAE��ģʽƥ�䣩
    template <class Elem, class = void> struct _insert_impl;
    template <class Elem> struct _insert_impl<Elem, std::enable_if_t<size == 0>> { // �ռ���
        using result = group<Elem>; // ֱ�Ӳ�����Ԫ��
    }; 
    template <class Elem> struct _insert_impl<Elem, std::enable_if_t<compare_v<Elem, mid_elem>>> { // ��Ԫ������벿��
        using result = typename first_half::template insert<Elem>::template concat<mid_elem, second_half>;
    };
    template <class Elem> struct _insert_impl<Elem, std::enable_if_t<compare_v<mid_elem, Elem>>> { // ��Ԫ�����Ұ벿��
        using result = typename first_half::template concat<mid_elem, typename second_half::template insert<Elem>>;
    };
    template <class Elem> struct _insert_impl<Elem> { // �������Ｔ��Elem == mid_elem
        using result = group<Elems...>; // ������
    };

    template <class Elem> using insert = typename _insert_impl<Elem>::result;
    template <class Elem> using find   = typename _find_impl<Elem>::result;
    template <class Elem> constexpr static bool has = !std::is_same_v<typename _find_impl<Elem>::result, null_elem>;
};

/* ---------------------group_listʵ��--------------------- */

template <template <class, class> class Equal, class EmptyGroup>
struct group_list<std::tuple<>, Equal, EmptyGroup> {
    using type = group_list<std::tuple<>, Equal, EmptyGroup>;
    using empty_group = EmptyGroup;
    using tuple = std::tuple<>;
    using front = empty_group;
    using rest  = type;
    
    template <class OtherList>
    using concat = std::conditional_t<std::is_void_v<OtherList>, type, OtherList>;

    template <class State>
    using fist_group_of = empty_group;

    template <class OtherGroup>
    using insert = group_list<std::tuple<OtherGroup>, Equal, EmptyGroup>;
};

template <class Group, class... Rest, template <class, class> class Equal, class EmptyGroup>
struct group_list<std::tuple<Group, Rest...>, Equal, EmptyGroup> {
    template <class... Groups>
    using list = group_list<std::tuple<Groups...>, Equal, EmptyGroup>;
    using empty_group = EmptyGroup;
    using tuple = std::tuple<Group, Rest...>;
    using front = Group;
    using rest  = list<Rest...>;

    template <class Left, class Right>
    constexpr static bool equal_v = Equal<Left, Right>::value;

    //template <class OtherList> struct _concat_impl;
    //template <>                struct _concat_impl<void> { using result = list<Group, Rest...>; };
    //template <class... Groups> struct _concat_impl<list<Groups...>> { using result = list<Group, Rest..., Groups...>; };
    template <class... OtherLists>
    using concat = group_list<tuple_concat<tuple, typename OtherLists::tuple...>, Equal, EmptyGroup>;

    // ������һ������ָ��Ԫ�ص���
    template <class State>
    using first_group_of = std::conditional_t<Group::template has<State>, Group, typename list<Rest...>::template first_group_of<State>>;

    // �������飬������������ڵȼ����Ա�
    template <class OtherGroup>
    using insert = std::conditional_t<
        equal_v<Group, OtherGroup>,
        list<OtherGroup, Group, Rest...>,
        typename list<Group>::template concat<typename list<Rest...>::template insert<OtherGroup>>
    >;
};

/* ---------------------state���������װ--------------------- */

using null_state = state<>;

template <class... States>
using state_group = ordered_group<std::tuple<States...>, state_compare, null_state>;
using empty_group = state_group<>;

}

#endif // !DFA_STATE_H_