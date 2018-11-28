#include "../src/dfa/dfa_state.hpp"

using namespace cp;
using namespace std;

// 以下A, B, C, D是有序的
using A = state<std::index_sequence<0, 1, 2>>;
using B = state<std::index_sequence<1, 2, 3>>;
using C = state<std::index_sequence<1, 2, 3, 4, 5>>;
using D = state<std::index_sequence<4, 5, 6>>;
using G1 = state_group<A, B>;
using G2 = state_group<C, D>;
using G3 = state_group<A, B, C, D>;
using G4 = state_group<A, B, C>;

// test state compare
static_assert(!lex_compare<int, integer_sequence<int>, integer_sequence<int>>::value);
static_assert(!lex_compare<int, integer_sequence<int, 1>, integer_sequence<int>>::value);
static_assert(lex_compare<int, integer_sequence<int>, integer_sequence<int, 1>>::value);
static_assert(lex_compare<int, integer_sequence<int, 1, 2, 3>, integer_sequence<int, 1, 4, 5>>::value);
static_assert(state_compare<A, B>::value);
static_assert(state_compare<B, C>::value);
static_assert(!state_compare<D, C>::value);
static_assert(comparator_trait<state_compare>::origin<A, B>::value);
static_assert(comparator_trait<state_compare>::reversed<D, C>::value);
static_assert(comparator_trait<state_compare>::equal<C, C>::value);

// test reverse
static_assert(is_same_v<std::tuple<A, B>, tuple_reverse<std::tuple<B, A>>>);
static_assert(is_same_v<ordered_group<std::tuple<B, A>, comparator_trait<state_compare>::reversed, null_state>, G1::reverse>);

// test concat
static_assert(is_same_v<G1::tuple, as_tuple<G1>>);
static_assert(is_same_v<state_group<A, B, C>, G1::concat<C>>);
static_assert(is_same_v<state_group<A, B, C>, empty_group::concat<G1, C>>);
static_assert(is_same_v<G3, G1::concat<G2>>);

// test diff
using t0 = std::tuple<>;
using t1 = std::tuple<A>;
using t2 = std::tuple<B>;
using t3 = std::tuple<A, B>;
static_assert(is_same_v<G3::tuple, decltype(std::tuple_cat(G1::tuple{}, G2::tuple{}))>);
static_assert(is_same_v<G3::tuple, decltype(std::tuple_cat<G1::tuple, G2::tuple>({}, {}))>);
static_assert(is_base_of_v<G3::tuple, tuple_concat<G1::tuple, G2::tuple>>);
static_assert(is_same_v<t0, tuple_diff<t0, t1, state_compare>>);
static_assert(is_same_v<t1, tuple_diff<t1, t0, state_compare>>);
static_assert(is_base_of_v<t1, tuple_diff<t1, t2, state_compare>>);
static_assert(is_base_of_v<t1, tuple_diff<t3, t2, state_compare>>);
static_assert(is_base_of_v<G1::tuple, tuple_diff<G3::tuple, G2::tuple, state_compare>>);
static_assert(is_same_v<G1, G3::diff<G2>>);
static_assert(is_same_v<G1, G4::diff<G2>>);
static_assert(is_same_v<state_group<D>, G2::diff<G4>>);

// test split
static_assert(is_same_v<type_pair<empty_group, G1>, G1::split<0>>);
static_assert(is_same_v<type_pair<state_group<A>, state_group<B>>, G1::split<1>>);
static_assert(is_same_v<type_pair<G1, G2>, G3::split<2>>);

// test halves
static_assert(is_same_v<G3::first_half, G1>);
static_assert(is_same_v<G3::second_half, state_group<D>>);
static_assert(is_same_v<G3::mid_elem, C>);

// test has_state
static_assert(state_group<A, B>::has<A>::value);
static_assert(!state_group<A, B>::has<C>::value);
static_assert(G3::has<B>::value);

// test insert
static_assert(is_same_v<state_group<D>, empty_group::insert<D>>);
static_assert(is_same_v<G2, empty_group::insert<C>::insert<D>>);
static_assert(is_same_v<G2, state_group<D>::insert<C>::insert<D>>);
static_assert(is_same_v<state_group<C>::insert<D>, state_group<D>::insert<C>>);
static_assert(is_same_v<G3, G3::insert<A>::insert<B>::insert<C>::insert<D>>);

template <class... Groups>
using list = group_list<std::tuple<Groups...>, std::is_same, empty_group>;

// test group list concat
static_assert(is_same_v<list<>::concat<list<G1>>, list<G1>>);
static_assert(is_same_v<list<>::concat<list<G1, G2>>, list<G1, G2>>);
static_assert(is_same_v<list<G1>::concat<list<G2>>, list<G1, G2>>);
static_assert(is_same_v<list<G1, G2>::concat<list<>>, list<G1, G2>>);

// test group list insert
static_assert(is_same_v<list<>::insert<G1>, list<G1>>);
static_assert(is_same_v<list<G1>::insert<G2>, list<G1, G2>>);
static_assert(is_same_v<list<G1, G2>::insert<G3>, list<G1, G2, G3>>);
static_assert(is_same_v<list<G1, G2, G3>::insert<G2>, list<G1, G2, G2, G3>>);
