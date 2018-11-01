#ifndef DFA_STATE_H_
#define DFA_STATE_H_
#include "utility.hpp"

namespace cp {

/* ---------------------ǰ������--------------------- */

template <size_t... I> 
struct state; // ��Regex AST�Ľ��λ�ý��б����״̬��

template <class ElemTuple, template <class Left, class Right> class Compare, class NullElem = void>
struct ordered_group; // ״̬�Ķ��������ظ����ϡ�

template <class GroupTuple = std::tuple<>, template <class Left, class Right> class Equal = std::is_same, class EmptyGroup = void>
struct group_list; // ״̬����б��б�����鲻��֤�����ҿ��ظ���Ԫ�ر������has������


/* ---------------------stateʵ��--------------------- */

template <size_t... I>
struct state : public std::index_sequence<I...> {
    using code = std::index_sequence<I...>;
};

// ��������ֵ���Լ��Ͻ��бȽϡ�
template <class Left, class Right>
struct state_compare : lex_compare<size_t, typename Left::code, typename Right::code> {};

/* ---------------------ordered_groupʵ��--------------------- */

template <class This, class... Rest, template <class Left, class Right> class Compare, class NullElem>
struct ordered_group<std::tuple<This, Rest...>, Compare, NullElem> {
    // type_traits
    template <class ElemTuple = std::tuple<>>
    using ctor = ordered_group<ElemTuple, Compare, NullElem>;
    using null_elem = NullElem;
    using tuple = std::tuple<This, Rest...>;
    using front = This;
    using rest  = ctor<std::tuple<Rest...>>;
    using type  = ctor<std::tuple<This, Rest...>>;

    // ��ȡԪ������
    constexpr static auto size = sizeof...(Rest) + 1;

    // ��ȡ�������򼯺�
    using reverse = ordered_group<tuple_reverse<tuple>, typename comparator_trait<Compare>::reversed, null_elem>;

    // �ϲ����Ԫ�ػ�Ԫ�ؼ���
    template <class... GroupsOrElems>
    using concat = tuplelike_concat<ctor, as_tuple, tuple, GroupsOrElems...>;

    template <class Other>
    using diff = ctor<tuple_diff<tuple, typename Other::tuple, Compare>>;

    // ���ַ���I����Sequence�з�
    template <size_t I> struct _split_impl;
    template <> struct _split_impl<0> { using result = type_pair<ordered_group<std::tuple<>, Compare, NullElem>, type>; };
    template <> struct _split_impl<1> { using result = type_pair<ordered_group<std::tuple<front>, Compare, NullElem>, rest>; };
    template <size_t I> struct _split_impl {
        using a = typename _split_impl<I / 2>::result;
        using b = typename a::second::template split<(I + 1) / 2>;
        using result = type_pair<
            typename a::first::template concat<typename b::first>, 
            typename b::second
        >;
    };
    template <size_t I>
    using split = typename _split_impl<I>::result;

    // ��ȡ����Ԫ�ؼ���ǰ�����벿��
    using first_half  = typename split<size / 2>::first;
    using second_half = typename split<size / 2>::second::rest;
    using mid_elem    = typename split<size / 2>::second::front;

    // ���ַ�����ָ��Ԫ�أ�������SFINAE��ģʽƥ�䣩
    template <class Elem, class Enable = void> struct _find_impl { // �������Ｔ��Elem == mid_elem
        using result = mid_elem; // �м�Ԫ�ؼ�Ϊ���ҽ��
    };
    template <class Elem> struct _find_impl<Elem, std::enable_if_t<Compare<Elem, mid_elem>::value>> { // ����벿�ֲ���
        using result = typename first_half::template find<Elem>;
    };
    template <class Elem> struct _find_impl<Elem, std::enable_if_t<Compare<mid_elem, Elem>::value>> { // ���Ұ벿�ֲ���
        using result = typename second_half::template find<Elem>;
    };


    // ���ַ�������ص���Ԫ�أ�������SFINAE��ģʽƥ�䣩
    template <class Elem, class Enable = void> struct _insert_impl { // �������Ｔ��Elem == mid_elem
        using result = type; // ������
    };
    template <class Elem> struct _insert_impl<Elem, std::enable_if_t<Compare<Elem, mid_elem>::value>> { // ��Ԫ������벿��
        using result = typename first_half::template insert<Elem>::template concat<mid_elem, second_half>;
    };
    template <class Elem> struct _insert_impl<Elem, std::enable_if_t<Compare<mid_elem, Elem>::value>> { // ��Ԫ�����Ұ벿��
        using result = typename first_half::template concat<mid_elem, typename second_half::template insert<Elem>>;
    };


    template <class Elem> using insert = typename _insert_impl<Elem>::result;
    template <class Elem> using find   = typename _find_impl<Elem>::result;
    template <class Elem> using has    = std::negation<std::is_same<find<Elem>, null_elem>>;
};

// base case�ػ�
template <template <class Left, class Right> class Compare, class NullElem>
struct ordered_group<std::tuple<>, Compare, NullElem> {
    // type_traits
    template <class ElemTuple = std::tuple<>>
    using ctor = ordered_group<ElemTuple, Compare, NullElem>;
    using null_elem = NullElem;
    using tuple = std::tuple<>;
    using front = null_elem;
    using rest = ordered_group<std::tuple<>, Compare, NullElem>;
    using type = ordered_group<std::tuple<>, Compare, NullElem>;

    constexpr static auto size = 0;

    template <class... GroupsOrElems> using concat = tuplelike_concat<ctor, as_tuple, GroupsOrElems...>;
    template <class Other> using diff = type;
    template <size_t I>    using split = type_pair<type, type>;

    using first_half = type;
    using second_half = type;
    using mid_elem = null_elem;

    template <class Elem> using insert = ctor<std::tuple<Elem>>;
    template <class Elem> using find = null_elem;
    template <class Elem> using has = std::false_type;
};

/* ---------------------group_listʵ��--------------------- */

template <class This, class... Rest, template <class, class> class Equal, class EmptyGroup>
struct group_list<std::tuple<This, Rest...>, Equal, EmptyGroup> {
    template <class GroupTuple>
    using ctor = group_list<GroupTuple, Equal, EmptyGroup>;
    using empty_group = EmptyGroup;
    using tuple = std::tuple<This, Rest...>;
    using front = This;
    using rest  = ctor<std::tuple<Rest...>>;
    using type  = ctor<std::tuple<This, Rest...>>;

    template <class... OtherLists>
    using concat = tuplelike_concat<ctor, as_tuple, tuple, OtherLists...>;

    // ������һ������ָ��Ԫ�ص���
    template <class State, class Enable = void> struct _first_group_of_impl { 
        using result = front; // �ҵ�������£����ص�һ����
    }; 
    template <class State> struct _first_group_of_impl<State, std::enable_if_t<!front::template has<State>::value>> { 
        using result = typename rest::template first_group_of<State>; // ��һ���鲻ƥ�������£��ݹ��������
    };
    template <class State>
    using first_group_of = typename _first_group_of_impl<State>::result;

    // �������飬������������ڵȼ����Ա�
    template <class Group, class Enable = void> struct _insert_impl {
        using result = ctor<std::tuple<Group, This, Rest...>>;
    };
    template <class Group> struct _insert_impl<Group, std::enable_if_t<!Equal<This, Group>::value>> {
        using this_group  = typename ctor<std::tuple<This>>;
        using rest_groups = typename rest::template insert<Group>;
        using result = typename this_group::template concat<rest_groups>;
    };
    template <class Group>
    using insert = typename _insert_impl<Group>::result;
};

template <template <class, class> class Equal, class EmptyGroup>
struct group_list<std::tuple<>, Equal, EmptyGroup> {
    template <class GroupTuple>
    using ctor = group_list<GroupTuple, Equal, EmptyGroup>;
    using empty_group = EmptyGroup;
    using tuple = std::tuple<>;
    using front = empty_group;
    using rest = group_list<std::tuple<>, Equal, EmptyGroup>;
    using type = group_list<std::tuple<>, Equal, EmptyGroup>;

    template <class... OtherLists>
    using concat = tuplelike_concat<ctor, as_tuple, OtherLists...>;

    template <class State>
    using fist_group_of = empty_group;

    template <class OtherGroup>
    using insert = group_list<std::tuple<OtherGroup>, Equal, EmptyGroup>;
};

/* ---------------------state���������װ--------------------- */

using null_state = state<>;

template <class... States>
using state_group = ordered_group<std::tuple<States...>, state_compare, null_state>;
using empty_group = state_group<>;

}

#endif // !DFA_STATE_H_