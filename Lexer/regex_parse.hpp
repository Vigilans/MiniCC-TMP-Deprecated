#ifndef REGEX_PARSE_H_
#define REGEX_PARSE_H_
#include "utility.hpp"
#include "regex_def.hpp"
#include <pybind11/common.h>

namespace cp {

template <class Sequence, size_t i, class Result>
struct parse_result {
    constexpr static decltype(auto) sequence() {
        return Sequence{};
    }

    constexpr static decltype(auto) getch() {
        return Sequence::template get<i>();
    }

    constexpr static decltype(auto) peekch() {
        return Sequence::template get<i + 1>();
    }

    constexpr static decltype(auto) forward() {
        return parse_result<Sequence, i + 1, Result>{};
    }

    constexpr static decltype(auto) result() {
        return Result{};
    }

    template <class NewResult>
    constexpr static decltype(auto) set(NewResult ret) {
        return parse_result<Sequence, i, NewResult>{};
    }
};

class regex_parser {
public:
    template <class Sequence>
    constexpr static decltype(auto) parse(Sequence s) {
        // 根节点为括号操作，从索引0开始解析，初始ParseResult为void
        return parse_bracket(parse_result<Sequence, 0, void>{});
    }

private:
    template <class ParseResult>
    constexpr static decltype(auto) parse_bracket(ParseResult r) {
        
    }

    template <class ParseResult>
    constexpr static decltype(auto) parse_unite(ParseResult r) {
        return op_loop(
            parse_concat(r),
            [](auto ret) { return ret.getch() != ch<'\0'>; },
            [](auto ret) {
                auto e = parse_concat(ret.forward());
                return e.set(make_unite(ret.result, e.result));
            }
        );
    }

    template <class ParseResult>
    constexpr static decltype(auto) parse_concat(ParseResult r) {
        return op_loop(
            parse_kleene(r),
            [](auto ret) { return (ret.getch() != ch<'\0'>) && (ret.getch() != ch<'|'>;) }
            
        );
    }
};

}


#endif // !REGEX_PARSE_H_
