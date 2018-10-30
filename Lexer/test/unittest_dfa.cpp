#include "../src/dfa.hpp"

using namespace cp;
using namespace std;

// 紫龙书图3-36，为保序与图3-63编码不一致
using A = state<1, 2, 3>;
using B = state<1, 2, 3, 4>;
using C = state<1, 2, 3, 5>;
using D = state<1, 2, 3, 6>;
using E = state<1, 2, 3, 7>;

using tAa = transition<A, 'a', B>;
using tAb = transition<A, 'b', C>;
using tBa = transition<B, 'a', B>;
using tBb = transition<B, 'b', D>;
using tCa = transition<C, 'a', B>;
using tCb = transition<C, 'b', C>;
using tDa = transition<D, 'a', B>;
using tDb = transition<D, 'b', E>;
using tEa = transition<E, 'a', B>;
using tEb = transition<E, 'b', C>;

//// test transition compare
//static_assert(trans_compare<tAa, tAb>::value);
//static_assert(!trans_compare<tAb, tAa>::value);
//static_assert(!trans_compare<tAa, tAa>::value);
//static_assert(trans_compare<tAb, tBa>::value);
//static_assert(trans_compare<tBb, tEa>::value);
//
//// test trans_table
//using table  = init_trans_table<tAa, tAb, tBa, tBb, tCa, tCb, tDa, tDb, tEa, tEb>;
//using table_ = init_trans_table<tAb, tBb, tBa, tAa, tEb, tCa, tEa, tCb, tDa, tDb>;
//static_assert(std::is_same_v<state_group<A, B>, trans_table<>::insert<tAa>::states>);
//static_assert(std::is_same_v<char_set<'a', 'b'>, trans_table<>::insert<tAa>::insert<tAb>::charset>);
//static_assert(std::is_same_v<table, table_>);
//static_assert(std::is_same_v<table::states, state_group<A, B, C, D, E>>);
//static_assert(std::is_same_v<table::charset, char_set<'a', 'b'>>);
//static_assert(std::is_same_v<table::trans<B, 'b'>, D>);
//static_assert(std::is_same_v<table::trans<E, 'b'>, C>);
//
//// test dfa
//using DFA = dfa<table, A, state_group<E>>;
//static_assert(std::is_same_v<DFA::initial_state, A>);
//static_assert(std::is_same_v<DFA::accept_states, state_group<E>>);
//static_assert(std::is_same_v<DFA::normal_states, state_group<A, B, C, D>>);
//
//// test min-dfa - preparing
//using p_split = DFA::_min_dfa_impl_split<DFA::normal_states, DFA::accept_states>;
//using p_groups = p_split::_new_groups_impl<DFA::normal_states, DFA::accept_states>;
//
//// test min-dfa::split::pairs process
//template <class Group> using p_pair  = p_groups::pairs_impl<Group>;
//template <class Group> using sg_pair = typename p_pair<Group>::state_group_pair;
//
//static_assert(std::is_same_v<sg_pair<state_group<A, B, C, D>>, pair<A, state_group<B, C>>>);
//static_assert(std::is_same_v<sg_pair<state_group<B, C, D>>, pair<B, state_group<B, D>>>);
//static_assert(std::is_same_v<sg_pair<state_group<C, D>>, pair<C, state_group<B, C>>>);
//static_assert(std::is_same_v<sg_pair<state_group<D>>, pair<D, state_group<B, E>>>);
//static_assert(std::is_same_v<
//    p_pair<state_group<A, B, C, D>>::result, 
//    p_groups::pair_list< // 是逆序插入的，且对于相等的值，新值插在前面
//        pair<D, state_group<B, E>>,
//        pair<A, state_group<B, C>>,
//        pair<C, state_group<B, C>>,
//        pair<B, state_group<B, D>>, 
//    >
//>);
//static_assert(std::is_same_v<p_groups::mapped_pairs, p_pair<state_group<A, B, C, D>>::result>);
//
//// test min-dfa::split::compress process
//template <class PairList> using p_cpre = p_groups::compress_impl<PairList>;
//template <class... Pairs> using pairs = p_groups::pair_list<Pairs...>;
//
//using mapD = pair<D, state_group<B, E>>;
//using mapA = pair<A, state_group<B, C>>;
//using mapC = pair<C, state_group<B, C>>;
//using mapB = pair<B, state_group<B, D>>;
//
//#define group state_group
//static_assert(std::is_same_v<p_cpre<pairs<>>::result, DFA::division_list<>>);
//static_assert(std::is_same_v<p_cpre<pairs<mapB>>::result, DFA::division_list<group<B>>>);
//static_assert(std::is_same_v<p_cpre<pairs<mapC, mapB>>::result, DFA::division_list<group<C>, group<B>>>);
//static_assert(std::is_same_v<p_cpre<pairs<mapA, mapC, mapB>>::result, DFA::division_list<group<A, C>, group<B>>>); // 发生合并
//static_assert(std::is_same_v<p_cpre<pairs<mapD, mapA, mapC, mapB>>::result, DFA::division_list<group<D>, group<A, C>, group<B>>>);
//#undef group
//
//DFA::_division_list;