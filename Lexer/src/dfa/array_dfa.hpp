#ifndef ARRAY_DFA_H_
#define ARRAY_DFA_H_
#include "dfa.hpp"
#include "dfa_utility.hpp"
#include <array>


namespace cp {

template <class DFA>
struct array_dfa_2d { // ʹ���ַ���ѹ����Ķ�ά�����ʾת�����״̬��
    // type-traits
    using origin = index_dfa<DFA>;

    constexpr static auto charset_size = origin::charset::size;
    
    constexpr static auto states_size = origin::states::size;

    constexpr static auto table_size = origin::trans_table::size;

public:
    template <char ch, char... rest> // sizeof...(rest)Ϊ0ʱ��ch������
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

    // ��Ч�����1��ʼ������0������Чת��
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
            table[from + 1][cond] = to + 1; // ��Ч״̬��1��ʼ��״̬0�����״̬
            return table;
        }
    };

    // ��ĳ��Ԫ��Ϊ0�����Ԫ�ض�Ӧ��ת�������˿�״̬��
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

    // ��Ч״̬�ǽ���״̬�ı�ǩΪ0�������Ϊ����״̬��
    constexpr static auto tag_list = make_tag_list<states_size - 1>();

public:
    // ��״̬����
    constexpr static auto null_state = 0;

    // �����ַ���
    constexpr static auto encode(char c) {
        return charset_encoder[c];
    }

    // ��״̬Ϊ0��������ת�����ǿ�״̬�����е���Чת�����ִ��״̬
    constexpr static auto trans(int state, char cond) {
        return trans_table[state][encode(cond)];
    }

    // ��ȡ״̬��Ӧ�ı�ǩ
    constexpr static auto get_tag(int state) {
        return tag_list[state];
    }
};

template <class DFA>
using array_dfa = array_dfa_2d<DFA>;

}

#endif // !ARRAY_DFA_H_