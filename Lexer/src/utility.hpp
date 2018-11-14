#ifndef UTILITY_H_
#define UTILITY_H_
#include <utility>
#include <tuple>
#include <cstdint>

namespace cp {

// 传入参数并实例化，便可在编译错误中看到参数的类型
template <typename...> struct which_type;

template <class T, T c1, T c2>
constexpr std::bool_constant<c1 == c2> operator==(std::integral_constant<T, c1>, std::integral_constant<T, c2>) { return {}; }
template <class T, T c1, T c2>
constexpr std::bool_constant<c1 != c2> operator!=(std::integral_constant<T, c1>, std::integral_constant<T, c2>) { return {}; }

template <size_t N>
using index_constant = std::integral_constant<size_t, N>;

template <char c>
using char_constant = std::integral_constant<char, c>;

template <char c>
constexpr static auto ch = char_constant<c>{};

// 字符序列，字符的位置有实际意义
template <char... chars>
struct char_sequence : std::integer_sequence<char, chars...> {
    template <size_t i>
    constexpr static decltype(auto) get() {
        static_assert(i < sizeof...(chars));
        return char_constant<std::get<i>(std::make_tuple(chars...))>{};
    }
};

// 字符集合，字符的位置无意义
// 暂时没有需要比较字符集合相等的操作，故不保证有序也可以
template <char... chars>
struct char_set {
    template <char ch>
    constexpr static bool has_v = ((chars == ch) || ...);

    template <char ch>
    using has = std::bool_constant<has_v<ch>>;

    template <char ch>
    using insert = std::conditional_t<has_v<ch>, char_set<chars...>, char_set<ch, chars...>>;
};

// 按照less-than字典序进行比较
template <class T, class I1, class I2>
struct lex_compare;
template <class T> // 两个空序列比较返回false
struct lex_compare<T, std::integer_sequence<T>, std::integer_sequence<T>> : std::false_type {};
template <class T, T v, T... rest> // 非空序列比空序列大，返回false
struct lex_compare<T, std::integer_sequence<T, v, rest...>, std::integer_sequence<T>> : std::false_type {};
template <class T, T v, T... rest> // 空序列比非空序列小，返回true
struct lex_compare<T, std::integer_sequence<T>, std::integer_sequence<T, v, rest...>> : std::true_type {};
template <class T, T v1, T v2> // 单元素序列直接比较值
struct lex_compare<T, std::integer_sequence<T, v1>, std::integer_sequence<T, v2>> : std::bool_constant<(v1 < v2)> {};
template <class T, T v1, T... I1, T v2, T... I2> // 两者均非空时，进行默认字典序比较
struct lex_compare<T, std::integer_sequence<T, v1, I1...>, std::integer_sequence<T, v2, I2...>>
    : std::conditional_t<(v1 < v2), std::true_type,
      std::conditional_t<(v1 > v2), std::false_type,
      lex_compare<T, std::integer_sequence<T, I1...>, std::integer_sequence<T, I2...>>>> {};

// 比较相关trait
template <template <class Left, class Right> class Compare>
struct comparator_trait {
    template <class Left, class Right> using origin   = Compare<Left, Right>;
    template <class Left, class Right> using reversed = Compare<Right, Left>;
    template <class Left, class Right> using equal = std::bool_constant<(!origin<Left, Right>::value && !reversed<Left, Right>::value)>;
};

// 判断一个整数序列是否为升序
template <class T, class I>
struct is_ascending;
template <class T>
struct is_ascending<T, std::integer_sequence<T>> : std::true_type {};
template <class T, T v>
struct is_ascending<T, std::integer_sequence<T, v>> : std::true_type {};
template <class T, T v1, T v2, T... I>
struct is_ascending<T, std::integer_sequence<T, v1, v2, I...>> :
    std::bool_constant<(v1 < v2) && is_ascending<T, std::integer_sequence<T, v2, I...>>::value> {};

/* ---------------------tuple类型方法补充--------------------- */

// 重新定义一个pair是因为std::pair有数据成员，会导致各种未定义错误
template <class First, class Second>
struct type_pair {
    using first  = First;
    using second = Second;
    
    template <class Type>
    using has = std::disjunction<std::is_same<Type, First>, std::is_same<Type, Second>>;
};

template <class... Tuples>
struct _tuple_concat_impl;
template <>
struct _tuple_concat_impl<> { using result = std::tuple<>; };
template <class Left, class... Right>
struct _tuple_concat_impl<Left, std::tuple<Right...>> { using result = std::tuple<Left, Right...>; };
template <class... Left, class... Right>
struct _tuple_concat_impl<std::tuple<Left...>, std::tuple<Right...>> { using result = std::tuple<Left..., Right...>; };
template <class This, class... Rest>
struct _tuple_concat_impl<This, Rest...> { using result = typename _tuple_concat_impl<This, typename _tuple_concat_impl<Rest...>::result>::result; };
template <class... Tuples>
using tuple_concat = typename _tuple_concat_impl<Tuples...>::result;

template <class T, class = void> 
struct _as_tuple_impl { using type = T; };
template <class T>
struct _as_tuple_impl<T, std::void_t<typename T::tuple>> { using type = typename T::tuple; };
template <class T>
using as_tuple = typename _as_tuple_impl<T>::type;

template <template <class Tuple> class Constructor, template <class T> class TupleMapper, class... Types>
using tuplelike_concat = Constructor<tuple_concat<TupleMapper<Types>...>>;

// 反转tuple
template <class Tuple>
struct _tuple_reverse_impl;
template <>
struct _tuple_reverse_impl<std::tuple<>> { using result = std::tuple<>; };
template <class This, class... Rest>
struct _tuple_reverse_impl<std::tuple<This, Rest...>> {
    using result = tuple_concat<typename _tuple_reverse_impl<std::tuple<Rest...>>::result, std::tuple<This>>;
};
template <class Tuple>
using tuple_reverse = typename _tuple_reverse_impl<Tuple>::result;

// 基于tuple内类型有序的前提下进行A-B集合操作
template <class A, class B, template <class, class> class Compare>
struct _tuple_diff_impl;
template <class... B, template <class, class> class Compare>
struct _tuple_diff_impl<std::tuple<>, std::tuple<B...>, Compare> { using result = std::tuple<>; };
template <class a, class... A, template <class, class> class Compare>
struct _tuple_diff_impl<std::tuple<a, A...>, std::tuple<>, Compare> { using result = std::tuple<a, A...>; };
template <class a, class... A, class b, class... B, template <class, class> class Compare>
struct _tuple_diff_impl<std::tuple<a, A...>, std::tuple<b, B...>, Compare> {
    using less    = tuple_concat<a, typename _tuple_diff_impl<std::tuple<A...>, std::tuple<b, B...>, Compare>::result>;
    using greater = typename _tuple_diff_impl<std::tuple<a, A...>, std::tuple<B...>, Compare>::result;
    using equal   = typename _tuple_diff_impl<std::tuple<A...>,    std::tuple<B...>, Compare>::result;
    using result = std::conditional_t<Compare<a, b>::value, less,      
                   std::conditional_t<Compare<b, a>::value, greater, equal>>;
};
template <class A, class B, template <class, class> class Compare>
using tuple_diff = typename _tuple_diff_impl<A, B, Compare>::result;

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
