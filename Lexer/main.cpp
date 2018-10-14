#include <iostream>
#include <typeinfo>
#include "regex_def.hpp"

using namespace std;

template <class... Symbols>
void func(cp::detail::symbol_sequence<Symbols...> s) {
    cout << typeid(s).name() << endl;
}

int main() {
    auto a = cp::detail::make_symbol_range<'0', char, '9'>{};
    func(a);

    return 0;
}