#ifndef REGEX_DEF_H_
#define REGEX_DEF_H_
#include "../dfa/dfa_state.hpp"

namespace cp {

/* --------------------正则表达式主运算符定义---------------------- */

template <class Derived>
struct regex {};

template <char c>
struct symbol : regex<symbol<c>> {};

template <class Regex>
struct kleene : regex<kleene<Regex>> {};

template <class... Regexes>
struct unite : regex<unite<Regexes...>> {};

template <class... Regexes>
struct concat : regex<concat<Regexes...>> {};

namespace detail {

template <class... Symbols>
struct symbol_sequence { using type = symbol_sequence; };

template <char... chs>
struct make_symbol_sequence : symbol_sequence<symbol<chs>...> {};

template <char first, class IndexSeq>
struct index_char_mapper;

template <char first, size_t... I>
struct index_char_mapper<first, std::index_sequence<I...>> : make_symbol_sequence<static_cast<char>(first + I)...> {};

template <char first, class T, T constraint>
struct make_symbol_range;

template <char first, size_t length>
struct make_symbol_range<first, size_t, length> : index_char_mapper<first, std::make_index_sequence<length>> {};

template <char first, char last>
struct make_symbol_range<first, char, last> : make_symbol_range<first, size_t, last - first + 1> {
    static_assert(first <= last, "first should be no greater than last");
};

template <class SymbolSeq>
struct make_united_symbols { using type = make_united_symbols; };

template <class... Symbols>
struct make_united_symbols<symbol_sequence<Symbols...>> : unite<Symbols...> {};

template <char first, char last>
struct make_united_symbol_range : detail::make_united_symbols<detail::make_symbol_range<first, char, last>> {};

}

template <char ch> constexpr decltype(auto) make_symbol(char_constant<ch>) { return symbol<ch>{}; }
template <class T> constexpr decltype(auto) make_kleene(regex<T>) { return kleene<T>{}; }
template <class... Ts> constexpr decltype(auto) make_unite (regex<Ts>...) { return unite<Ts...>{}; }
template <class... Ts> constexpr decltype(auto) make_concat(regex<Ts>...) { return concat<Ts...>{}; }
template <class... Ts> constexpr decltype(auto) make_unite (detail::symbol_sequence<Ts...>) { return unite<Ts...>{}; }
template <class... Ts> constexpr decltype(auto) make_concat(detail::symbol_sequence<Ts...>) { return concat<Ts...>{}; }
template <class... Rs, class... Ts> constexpr decltype(auto) make_unite (unite<Rs>..., regex<Ts>...) { return unite<Rs..., Ts...>{}; }
template <class... Rs, class... Ts> constexpr decltype(auto) make_concat(concat<Rs>..., regex<Ts>...) { return concat<Rs..., Ts...>{}; }


/* --------------------正则表达式辅助运算符定义---------------------- */

struct escape_digit : detail::make_united_symbol_range<'0', '9'> {};
//struct escape_word : decltype(make_unite(
//    detail::make_symbol_range<'0', char, '9'>{},
//    detail::make_united_symbol_range<'a', 'z'>{},
//    detail::make_united_symbol_range<'A', 'Z'>{},
//    make_symbol(ch<'_'>)
//)) {};

}

#endif // !REGEX_DFA_H_
