#ifndef UTILITY_H_
#define UTILITY_H_
#include <utility>

namespace cp {

template <size_t N>
using index_constant = std::integral_constant<size_t, N>;

template <char c>
using char_constant = std::integral_constant<char, c>;

template <class T, T c1, T c2>
constexpr std::bool_constant<c1 == c2> operator==(std::integral_constant<T, c1>, std::integral_constant<T, c2>) { return {}; }

template <class T, T c1, T c2>
constexpr std::bool_constant<c1 != c2> operator!=(std::integral_constant<T, c1>, std::integral_constant<T, c2>) { return {}; }

template <char c>
constexpr static auto ch = char_constant<c>{};

// �ַ����У��ַ���λ����ʵ������
template <char... chars>
struct char_sequence : std::integer_sequence<char, chars...> {
    template <size_t i>
    constexpr static decltype(auto) get() {
        static_assert(i < sizeof...(chars));
        return char_constant<std::get<i>(std::make_tuple(chars...))>{};
    }
};

// �ַ����ϣ��ַ���λ��������
// ��ʱû����Ҫ�Ƚ��ַ�������ȵĲ������ʲ���֤����Ҳ����
template <char... chars>
struct char_set {
    template <char ch>
    constexpr static bool has = ((chars == ch) || ...);

    template <char ch>
    using insert = std::conditional_t<has<ch>, char_set<chars...>, char_set<ch, chars...>>;
};

// ����less-than�ֵ�����бȽ�
template <class T, class I1, class I2>
struct lex_compare;

template <class T> // ���������бȽϷ���false
struct lex_compare<T, std::integer_sequence<T>, std::integer_sequence<T>> : std::false_type {};

template <class T, T v, T... rest> // �ǿ����бȿ����д󣬷���false
struct lex_compare<T, std::integer_sequence<T, v, rest...>, std::integer_sequence<T>> : std::false_type {};

template <class T, T v, T... rest> // �����бȷǿ�����С������true
struct lex_compare<T, std::integer_sequence<T>, std::integer_sequence<T, v, rest...>> : std::true_type {};

template <class T, T v1, T v2> // ��Ԫ������ֱ�ӱȽ�ֵ
struct lex_compare<T, std::integer_sequence<T, v1>, std::integer_sequence<T, v2>> : std::bool_constant<(v1 < v2)> {};

template <class T, T v1, T... I1, T v2, T... I2> // ���߾��ǿ�ʱ������Ĭ���ֵ���Ƚ�
struct lex_compare<T, std::integer_sequence<T, v1, I1...>, std::integer_sequence<T, v2, I2...>>
    : std::conditional_t<(v1 < v2), std::true_type,
      std::conditional_t<(v1 > v2), std::false_type,
      lex_compare<T, std::integer_sequence<T, I1...>, std::integer_sequence<T, I2...>>>> {};

// �ж�һ�����������Ƿ�Ϊ����
template <class T, class I>
struct is_ascending;

template <class T>
struct is_ascending<T, std::integer_sequence<T>> : std::true_type {};

template <class T, T v>
struct is_ascending<T, std::integer_sequence<T, v>> : std::true_type {};

template <class T, T v1, T v2, T... I>
struct is_ascending<T, std::integer_sequence<T, v1, v2, I...>> :
    std::bool_constant<(v1 < v2) && is_ascending<T, std::integer_sequence<T, v2, I...>>::value> {};

template <bool v, class T1, class T2>
constexpr decltype(auto) op_cond(std::bool_constant<v>, T1 lhs, T2 rhs) {
    if constexpr (v) return lhs;
    else return rhs;
}

template <class Variable, class Condition, class Transfer>
constexpr decltype(auto) op_loop(Variable state, Condition cond, Transfer trans) {
    if constexpr (cond(state).value) {
        return op_loop(trans(state), cond, trans);
    } else {
        return state;
    }
}

}


#endif // !UTILITY_H_