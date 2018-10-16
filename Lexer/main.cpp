#include <iostream>
#include <typeinfo>
#include "regex_def.hpp"
#include "regex_dfa.hpp"

using namespace std;
using namespace cp::dfa;

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
    std::is_same_v<dfa<t>::trans<A, '1'>, B>;

    using C = state<1, 2, 3>;
    state_group<A, B>::has_state<A>;
    state_group<A, B>::has_state<C>;

    return 0;
}