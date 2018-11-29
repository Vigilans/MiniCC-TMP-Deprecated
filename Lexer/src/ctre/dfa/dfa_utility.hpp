#ifndef DFA_UTILITY_H_
#define DFA_UTILITY_H_
#include "dfa.hpp"

namespace cp {

template <class DFA, size_t offset = 0>
struct _index_dfa_impl {

    template <class State, class States = typename DFA::states> struct index_of {
        constexpr static auto result_v = -1;
    };
    template <class State, class Current, class... Rest>
    struct index_of<State, state_group<Current, Rest...>> {
        constexpr static auto result_v = std::is_same_v<State, Current>
            ? DFA::states::size - sizeof...(Rest) - 1 + offset
            : index_of<State, state_group<Rest...>>::result_v;
    };

    template <class State>
    using map_state = state<std::index_sequence<index_of<State>::result_v>, State::tag>;

    template <class TransTable> struct map_trans_table {
        using result = trans_table<>;
    };
    template <class This, class... Rest>
    struct map_trans_table<trans_table<std::tuple<This, Rest...>>> {
        using this_transition = transition<
            map_state<typename This::from_state>, This::cond, map_state<typename This::to_state>
        >;
        using rest_table = typename map_trans_table<trans_table<std::tuple<Rest...>>>::result;
        using result = typename rest_table::template insert<this_transition>;
    };

    using result = dfa <
        typename map_trans_table<typename DFA::trans_table>::result,
        map_state<typename DFA::initial_state>,
        typename DFA::accept_states::template map<map_state>
    >;
};

// ��DFA������״̬�ı���������
template <class DFA, size_t offset = 0>
using index_dfa = typename _index_dfa_impl<DFA, offset>::result;


template <class A, class B>
struct _union_state_impl;
template <size_t... I1, std::uint32_t t1, size_t... I2, std::uint32_t t2>
struct _union_state_impl<state<std::index_sequence<I1...>, t1>, state<std::index_sequence<I2...>, t2>> {
    using result = state<std::index_sequence<I1..., I2...>, t1 | t2>;
};

// �ϲ�����״̬��״̬��tagΪ��λ�����
template <class A, class B>
using union_state = typename _union_state_impl<A, B>::result;

template <class Lhs, class Rhs>
struct _union_dfa_impl {
    // �����������״̬��������
    using a = index_dfa<Lhs>;
    using b = index_dfa<Rhs, Lhs::states::size>; // ��֤���е�״̬�ı��벻ͬ

    // ѭ���ַ�����ÿ����״̬��һ���ַ�ӳ�䣬ӳ���������ת�����С�
    template <class char_set> struct map_charset { using result = trans_table<>; };
    template <char ch, char... rest> struct map_charset<char_set<char_sequence<ch, rest...>>> {
        // ѭ����һ��DFA��״̬
        template <class AStates> struct map_a { using result = trans_table<>; };
        template <class AThis, class... ARest> struct map_a<state_group<AThis, ARest...>> {
            using a_src = AThis;
            using a_des = typename a::template trans<a_src, ch>;
            // ѭ���ڶ���DFA��״̬
            template <class BStates> struct map_b { using result = trans_table<>; };
            template <class BThis, class... BRest> struct map_b<state_group<BThis, BRest...>> {
                using b_src = BThis;
                using b_des = typename b::template trans<b_src, ch>;
                using union_trans = transition<union_state<a_src, b_src>, ch, union_state<a_des, b_des>>; // ��ȡ�ϲ���ӳ��״̬
                using result = typename map_b<state_group<BRest...>>::result::template insert<union_trans>;
            };
            using rest_table = typename map_a<state_group<ARest...>>::result;
            using result = typename rest_table::template concat<typename map_b<typename b::states>::result>;
        };
        using rest_table = typename map_charset<char_set<char_sequence<rest...>>>::result;
        using result = typename rest_table::template concat<typename map_a<typename a::states>::result>;
    };

    // ��������״̬��������µĽ���״̬����
    // ѭ����һ��DFA��״̬
    template <class AStates> struct map_accepts_a { using result = state_group<>; };
    template <class AThis, class... ARest> struct map_accepts_a<state_group<AThis, ARest...>> {
        // ѭ���ڶ���DFA��״̬
        template <class BStates> struct map_accepts_b { using result = state_group<>; };
        template <class BThis, class... BRest> struct map_accepts_b<state_group<BThis, BRest...>> {
            using rest_group = typename map_accepts_b<state_group<BRest...>>::result;
            using result = std::conditional_t<
                std::disjunction<typename a::accept_states::template has<AThis>, typename b::accept_states::template has<BThis>>::value,
                typename rest_group::template insert<union_state<AThis, BThis>>, // ���AThis��BThis������һ���ǽ���״̬���򲢳��µĽ���״̬
                typename rest_group::type
            >;
        };
        using rest_group = typename map_accepts_a<state_group<ARest...>>::result;
        using result = typename rest_group::template concat<typename map_accepts_b<typename b::states>::result>;
    };

    using union_trans_table = typename map_charset<typename a::charset::template union_of<typename b::charset>>::result;
    using union_initial_state = union_state<typename a::initial_state, typename b::initial_state>;
    using union_accept_states = map_accepts_a<typename a::states>;

    // ����������������󷵻�
    using result = index_dfa<dfa<union_trans_table, union_initial_state, union_accept_states>>;
};

// �ϲ�����DFA
template <class Lhs, class Rhs>
using union_dfa = typename _union_dfa_impl<Lhs, Rhs>::result;

}

#endif // !DFA_UTILITY_H_
