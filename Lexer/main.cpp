#include <iostream>
#include <typeinfo>
#include "regex_def.hpp"
#include "regex_dfa.hpp"

using namespace std;
using namespace cp;

template <class... Symbols>
void func(cp::detail::symbol_sequence<Symbols...> s) {
    cout << typeid(s).name() << endl;
}

int main() {
    auto a = cp::detail::make_symbol_range<'0', char, '9'>{};
    func(a);

    using A = state<0, 1, 2>;
    using B = state<1, 2, 3, 4, 5>;
    using t = transition<A, B, '1'>;
    std::is_same_v<trans_table<t>::get<A, '1'>, B>;

    using C = state<1, 2, 3>;
    state_group<A, B>::has_state<A>;
    state_group<A, B>::has_state<C>;
    
    using G1 = state_group<A, B>;
    using G2 = state_group<C>;
    static_assert(std::is_same_v<group_set<G1, G2>::find<C>, G2>);
    group_set<G2>::find<A>;

    return 0;
}