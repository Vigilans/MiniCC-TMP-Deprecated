#include "../src/dfa.hpp"

using namespace cp;
using namespace std;

void test() {
    // 下列A, B, C, D是严格升序表示的
    using A = state<0, 1, 2>;
    using B = state<1, 2, 3>;
    using C = state<1, 2, 3, 4, 5>;
    using D = state<4, 5, 6>;
    using G1 = state_group<A, B>;
    using G2 = state_group<C, D>;
    using G3 = state_group<A, B, C, D>;

    // test state compare
    static_assert(!lex_compare<int, integer_sequence<int>, integer_sequence<int>>::value);
    static_assert(!lex_compare<int, integer_sequence<int, 1>, integer_sequence<int>>::value);
    static_assert(lex_compare<int, integer_sequence<int>, integer_sequence<int, 1>>::value);
    static_assert(lex_compare<int, integer_sequence<int, 1, 2, 3>, integer_sequence<int, 1, 4, 5>>::value);
    static_assert(state_compare<A, B>::value);
    static_assert(state_compare<B, C>::value);
    static_assert(!state_compare<D, C>::value);

    // test concat
    static_assert(is_same_v<state_group<A, B, C>, G1::concat<C>>);
    static_assert(is_same_v<state_group<A, B, C>, empty_group::concat<G1, C>>);
    static_assert(is_same_v<G3, G1::concat<G2>>);

    // test pop_first
    static_assert(is_base_of_v<pair<A, state_group<B>>, G1::pop_first<>>);
    
    // test split
    static_assert(is_same_v<pair<empty_group, G1>, G1::split<0>>);
    static_assert(is_same_v<pair<state_group<A>, state_group<B>>, G1::split<1>>);
    static_assert(is_same_v<pair<G1, G2>, G3::split<2>>);

    // test halves
    static_assert(is_same_v<G3::first_half, G1>);
    static_assert(is_same_v<G3::second_half, state_group<D>>);
    static_assert(is_same_v<G3::mid_elem, C>);
   
    // test has_state
    static_assert(state_group<A, B>::has<A>);
    static_assert(!state_group<A, B>::has<C>);
    static_assert(G3::has<B>);

    // test insert
    static_assert(std::is_same_v<state_group<D>, empty_group::insert<D>>);
    static_assert(std::is_same_v<G2, empty_group::insert<C>::insert<D>>);
    static_assert(std::is_same_v<G2, state_group<D>::insert<C>::insert<D>>);
    static_assert(std::is_same_v<state_group<C>::insert<D>, state_group<D>::insert<C>>);
    static_assert(std::is_same_v<G3, G3::insert<A>::insert<B>::insert<C>::insert<D>>);
}