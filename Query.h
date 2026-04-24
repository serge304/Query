#ifndef QUERIES_H
#define QUERIES_H

namespace detail {

// C++14 polyfill для std::void_t
template<class...> struct voider { using type = void; };
template<class... Ts> using void_t = typename voider<Ts...>::type;

// =========================================================================
// Единый механизм вызова: работает с лямбдами, функторами, PMF + smart_ptr
// =========================================================================

// 1. Обычный вызываемый объект
template<class Fn, class Arg>
auto invoke(Fn&& fn, Arg&& arg) -> decltype(std::forward<Fn>(fn)(std::forward<Arg>(arg))) {
    return std::forward<Fn>(fn)(std::forward<Arg>(arg));
}

// 2. Указатель на const-метод
template<class R, class C, class Arg>
auto invoke(R (C::*fn)() const, Arg&& arg) -> R {
    return std::mem_fn(fn)(std::forward<Arg>(arg)); // mem_fn знает про shared_ptr
}

// 3. Указатель на non-const метод
template<class R, class C, class Arg>
auto invoke(R (C::*fn)(), Arg&& arg) -> R {
    return std::mem_fn(fn)(std::forward<Arg>(arg));
}

// =========================================================================
// SFINAE-traits, использующие detail::invoke
// =========================================================================

template<class F, class T, class = void>
struct is_predicate : std::false_type {};

template<class F, class T>
struct is_predicate<F, T, void_t<decltype(detail::invoke(std::declval<const F&>(), std::declval<T>()))>>
    : std::integral_constant<bool,
                             std::is_convertible<decltype(detail::invoke(std::declval<const F&>(), std::declval<T>())), bool>::value> {};

template<class F, class T, class = void>
struct is_unary_op : std::false_type {};

template<class F, class T>
struct is_unary_op<F, T, void_t<decltype(detail::invoke(std::declval<const F&>(), std::declval<T>()))>>
    : std::integral_constant<bool,
                             !std::is_void<decltype(detail::invoke(std::declval<const F&>(), std::declval<T>()))>::value> {};

template<class F, class T, class R = void>
using enable_if_predicate = typename std::enable_if<is_predicate<F, T>::value, R>::type;

template<class F, class T, class R = void>
using enable_if_unary_op = typename std::enable_if<is_unary_op<F, T>::value, R>::type;

// =========================================================================
// not_fn (C++17 std::not_fn обратно портирован на C++14)
// =========================================================================

template<class Predicate>
auto not_fn(Predicate&& p) {
    using DecayedP = typename std::decay<Predicate>::type;
    return [pred = DecayedP(std::forward<Predicate>(p))](auto&&... args) -> bool {
        return !detail::invoke(pred, std::forward<decltype(args)>(args)...);
    };
}

} // namespace detail


#endif // QUERIES_H
