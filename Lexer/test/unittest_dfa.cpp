#include "../src/dfa.hpp"
#include <iostream>
#include <typeinfo>

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

// test transition compare
static_assert(trans_compare<tAa, tAb>::value);
static_assert(!trans_compare<tAb, tAa>::value);
static_assert(!trans_compare<tAa, tAa>::value);
static_assert(trans_compare<tAb, tBa>::value);
static_assert(trans_compare<tBb, tEa>::value);

// test trans_table
using table  = init_trans_table<tAa, tAb, tBa, tBb, tCa, tCb, tDa, tDb, tEa, tEb>;
using table_ = init_trans_table<tAb, tBb, tBa, tAa, tEb, tCa, tEa, tCb, tDa, tDb>;
static_assert(is_same_v<state_group<A, B>, trans_table<>::insert<tAa>::states>);
static_assert(is_same_v<char_set<'a', 'b'>, trans_table<>::insert<tAa>::insert<tAb>::charset>);
static_assert(is_same_v<table, table_>);
static_assert(is_same_v<table::states, state_group<A, B, C, D, E>>);
static_assert(is_same_v<table::charset, char_set<'a', 'b'>>);
static_assert(is_same_v<table::trans<B, 'b'>, D>);
static_assert(is_same_v<table::trans<E, 'b'>, C>);

// test dfa
using DFA = dfa<table, A, state_group<E>>;
static_assert(is_same_v<DFA::initial_state, A>);
static_assert(is_same_v<DFA::accept_states, state_group<E>>);
static_assert(is_same_v<DFA::normal_states, state_group<A, B, C, D>>);

// test min-dfa - preparing
template <class... Groups> using division = _min_dfa_impl<DFA>::division<Groups...>;
using split_to_atom = _min_dfa_impl<DFA>::split_to_atom<division<DFA::normal_states, DFA::accept_states>, true>;
using split_division = split_to_atom::split_division<DFA::normal_states, DFA::accept_states>;
template <class Group> using map = split_division::map<typename Group::tuple>;
template <class Group> using mapped_pair = typename map<Group>::this_mapped_pair;
template <class PairList> using reduce = split_division::reduce<PairList>;
template <class... Pairs> using pairs  = split_division::pair_list<Pairs...>;

#define group state_group
// test min-dfa::split::map 
static_assert(is_same_v<mapped_pair<group<A, B, C, D>>, type_pair<A, group<B, C>>>);
static_assert(is_same_v<mapped_pair<group<B, C, D>>,    type_pair<B, group<B, D>>>);
static_assert(is_same_v<mapped_pair<group<C, D>>,       type_pair<C, group<B, C>>>);
static_assert(is_same_v<mapped_pair<group<D>>,          type_pair<D, group<B, E>>>);
static_assert(is_same_v<
    map<group<A, B, C, D>::reverse>::result, 
    pairs<  // 对于相等的值，新值插在前面
        type_pair<C, group<B, C>>,
        type_pair<A, group<B, C>>,
        type_pair<B, group<B, D>>,
        type_pair<D, group<B, E>>
    >
>);

// test min-dfa::split::reduce
using mapC = type_pair<C, state_group<B, C>>;
using mapA = type_pair<A, state_group<B, C>>;
using mapB = type_pair<B, state_group<B, D>>;
using mapD = type_pair<D, state_group<B, E>>;
static_assert(is_same_v<reduce<pairs<>>::result,                       division<>>);
static_assert(is_same_v<reduce<pairs<mapD>>::result,                   division<group<D>>>);
static_assert(is_same_v<reduce<pairs<mapB, mapD>>::result,             division<group<B>, group<D>>>);
static_assert(is_same_v<reduce<pairs<mapA, mapB, mapD>>::result,       division<group<A>, group<B>, group<D>>>); 
static_assert(is_same_v<reduce<pairs<mapC, mapA, mapB, mapD>>::result, division<group<A, C>, group<B>, group<D>>>); // 发生合并

// test min-dfa::split_division result
static_assert(is_same_v<split_division::new_this_group, division<group<A, C>, group<B>, group<D>>>);
static_assert(is_same_v<split_division::new_rest_groups, division<group<E>>>);
static_assert(is_same_v<split_to_atom::new_division, division<group<A, C>, group<B>, group<D>, group<E>>>);

// test min-dfa::split_to_atom and final_division
using new_division = split_to_atom::new_division;
using next_split = _min_dfa_impl<DFA>::split_to_atom<new_division, true>;
static_assert(is_same_v<next_split::new_division, new_division>);
static_assert(is_same_v<split_to_atom::result, new_division>);
static_assert(is_same_v<_min_dfa_impl<DFA>::final_division, new_division>);

// test min-dfa::map_representative & new_transtable & build
using final_division = _min_dfa_impl<DFA>::final_division;
using build = _min_dfa_impl<DFA>::build;
using new_trans_table = init_trans_table<
    transition<A, 'a', B>, transition<A, 'b', A>,
    transition<B, 'a', B>, transition<B, 'b', D>,
    transition<D, 'a', B>, transition<D, 'b', E>,
    transition<E, 'a', B>, transition<E, 'b', A>
>;
static_assert(is_same_v<build::rep_initial, A>);
static_assert(is_same_v<build::rep_accepts, group<E>>);
static_assert(is_same_v<build::rep_states,  group<A, B, D, E>>);
static_assert(is_same_v<build::trans_table, new_trans_table>);

#undef group

// test min-dfa
using MinDFA = min_dfa<DFA>;
static_assert(is_same_v<MinDFA::initial_state, A>);
static_assert(is_same_v<MinDFA::accept_states, state_group<E>>);
static_assert(is_same_v<MinDFA::normal_states, state_group<A, B, D>>);
static_assert(is_same_v<MinDFA::trans<A, 'b'>, A>);
static_assert(is_same_v<MinDFA::trans<E, 'b'>, A>);