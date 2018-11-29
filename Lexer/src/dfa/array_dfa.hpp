#ifndef ARRAY_DFA_H_
#define ARRAY_DFA_H_
#include "dfa.hpp"
#include "dfa_utility.hpp"
#include <array>


namespace cp {

template <class DFA>
struct array_dfa_2d { // 使用字符集压缩后的二维数组表示转换表的状态机
    // type-traits
    using origin = index_dfa<DFA>;

    constexpr static auto charset_size = origin::charset::size;
    
    constexpr static auto states_size = origin::states::size;

    constexpr static auto table_size = origin::trans_table::size;

public:
    template <char ch, char... rest> // sizeof...(rest)为0时的ch是最大的
    constexpr static auto make_encoder(char_set<char_sequence<ch, rest...>>) {
        if constexpr (sizeof...(rest) == 0) {
            std::array<int, CHAR_MAX> encoder{};
            encoder[ch] = charset_size;
            return encoder;
        } else {
            auto encoder = make_encoder(char_set<char_sequence<rest...>>{});
            encoder[ch] = charset_size - sizeof...(rest);
            return encoder;
        }
    }

    // 有效编码从1开始，编码0代表无效转换
    constexpr static auto charset_encoder = make_encoder(typename origin::charset{});

    template <size_t i>
    constexpr static auto make_trans_table() {
        if constexpr (i == 0) {
            return std::array<std::array<int, charset_size + 1>, states_size + 1> {};
        } else {
            using transition = std::tuple_element_t<i, typename origin::trans_table::tuple>;
            using from_state = typename transition::from_state;
            using to_state = typename transition::to_state
            constexpr auto from = std::get<0>(typename from_state::code{});
            constexpr auto to = std::is_same_v<to_state, null_state> ? -1 : std::get<0>(typename to_state::code{});
            constexpr auto cond = charset_encoder[transition::cond];
            auto table = make_trans_table<i - 1>();
            table[from + 1][cond] = to + 1; // 有效状态从1开始，状态0代表空状态
            return table;
        }
    };

    // 若某个元素为0，则该元素对应的转换到达了空状态。
    constexpr static auto trans_table = make_trans_table<table_size - 1>();

    template <size_t i>
    constexpr static auto make_tag_list() {
        if constexpr (i == 0) {
            return std::array<std::uint32_t, states_size + 1> {};
        } else {
            using state = std::tuple_element_t<i, typename origin::states::tuple>;
            constexpr auto code = std::get<0>(typename state::code{});
            constexpr auto tag = state::tag;
            auto list = make_tag_list<i - 1>();
            list[code] = tag;
            return list;
        }
    }

    // 无效状态非接受状态的标签为0，其余均为接受状态。
    constexpr static auto tag_list = make_tag_list<states_size - 1>();

public:
    // 空状态索引
    constexpr static auto null_state = 0;

    // 编码字符集
    constexpr static auto encode(char c) {
        return charset_encoder[c];
    }

    // 空状态为0，其所有转换都是空状态，所有的无效转换都抵达空状态
    constexpr static auto trans(int state, char cond) {
        return trans_table[state][encode(cond)];
    }

    // 获取状态对应的标签
    constexpr static auto get_tag(int state) {
        return tag_list[state];
    }
};

template <class DFA>
using array_dfa = array_dfa_2d<DFA>;

}

#endif // !ARRAY_DFA_H_