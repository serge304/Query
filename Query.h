#ifndef QUERIES_H
#define QUERIES_H

#include <Numeric.h>

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

//!
//! Итератор, выбирающий элементы контейнера, используя заданное условие.
//!

template<class Operation, class It,
         class = typename std::enable_if<
                              std::is_convertible<
                                  decltype(std::declval<const Operation&>()(*std::declval<const It&>())),
                                  bool
                                  >::value
                              >::type>
class skip_iterator
{
    It first;
    It last;
    Operation op;

    void advance()
    {
        while (first != last && !detail::invoke(op, *first))
            ++first;
    }

public:
    // Типы, требуемые для совместимости с STL-алгоритмами
    using iterator_category = std::forward_iterator_tag;
    using value_type        = typename std::iterator_traits<It>::value_type;
    using difference_type   = typename std::iterator_traits<It>::difference_type;
    using pointer           = typename std::iterator_traits<It>::pointer;
    using reference         = typename std::iterator_traits<It>::reference;

    // Основной конструктор. Для создания end-итератора передавайте end, end, op
    skip_iterator(It _first, It _last, Operation _op)
        : first(std::move(_first)), last(std::move(_last)), op(std::move(_op))
    {
        advance();
    }

    reference operator*() const { return *first; }
    pointer   operator->() const { return &*first; }

    skip_iterator& operator++()
    {
        if (first != last)
        {
            ++first;
            advance();
        }
        return *this;
    }

    skip_iterator operator++(int)
    {
        skip_iterator tmp(*this);
        ++(*this);
        return tmp;
    }

    bool operator==(const skip_iterator& other) const { return first == other.first; }
    bool operator!=(const skip_iterator& other) const { return !(*this == other); }

    // Доступ к обёрнутому итератору (полезно для отладки и специфичных алгоритмов)
    const It& base() const { return first; }
};

// Вспомогательная функция для вывода типов
template<class Operation, class It>
skip_iterator<Operation, It> make_skip_iterator(It begin, It end, Operation op) {
    return {std::move(begin), std::move(end), std::move(op)};
}

//!
//! Итератор, преобразующий элементы контейнера используя заданное условие
//!

template<class Operation, class It>
class transform_iterator
{
    It first;
    Operation op;

public:
    using raw_result = decltype(detail::invoke(std::declval<const Operation&>(), *std::declval<const It&>()));
    using iterator_category = typename std::conditional<std::is_reference<raw_result>::value, std::forward_iterator_tag, std::input_iterator_tag>::type;
    using value_type = typename std::decay<raw_result>::type;
    using reference  = raw_result;
    using pointer    = typename std::conditional<std::is_reference<reference>::value, typename std::add_pointer<reference>::type, void>::type;
    using difference_type = typename std::iterator_traits<It>::difference_type;

    transform_iterator(It _first, Operation _op) : first(std::move(_first)), op(std::move(_op)) {}

    reference operator*() const { return detail::invoke(op, *first); }

    transform_iterator& operator++()
    {
        ++first;
        return *this;
    }

    transform_iterator operator++(int)
    {
        transform_iterator tmp(*this);
        ++(*this);
        return tmp;
    }

    bool operator==(const transform_iterator& other) const { return first == other.first; }
    bool operator!=(const transform_iterator& other) const { return !(*this == other); }

    // Доступ к обёрнутому итератору (полезно для отладки и специфичных алгоритмов)
    const It& base() const { return first; }
};

// Вспомогательная функция для вывода типов (в C++14 нет CTAD)
template<class Operation, class It>
transform_iterator<Operation, It> make_transform_iterator(It it, Operation op) {
    return {std::move(it), std::move(op)};
}

//!
//! Сравнение заданного значения и результата функтора
//!

template<class OP, typename T>
auto Less(OP op, T val) {
    return [val = std::move(val), op = std::move(op)](auto&& elem) mutable {
        return detail::invoke(op, std::forward<decltype(elem)>(elem)) < val;
    };
}

template<class OP, typename T>
auto Greater(OP op, T val) {
    return [val = std::move(val), op = std::move(op)](auto&& elem) mutable {
        return detail::invoke(op, std::forward<decltype(elem)>(elem)) > val;
    };
}

template<class OP, typename T>
auto Is(OP op, T val) {
    return [val = std::move(val), op = std::move(op)](auto&& elem) mutable {
        return detail::invoke(op, std::forward<decltype(elem)>(elem)) == val;
    };
}

template<class OP, typename T>
auto IsNot(OP op, T val) {
    return [val = std::move(val), op = std::move(op)](auto&& elem) mutable {
        return detail::invoke(op, std::forward<decltype(elem)>(elem)) != val;
    };
}

//!
//! Простые операции
//!

template<typename T>
auto minus_const(T c) { return [val = std::move(c)](auto&& x) { return std::forward<decltype(x)>(x) - val; }; }

template<typename T>
auto plus_const(T c) { return [val = std::move(c)](auto&& x) { return std::forward<decltype(x)>(x) + val; }; }

//!
//! Функторы
//!

template<class P>
auto Not(P&& p) { return detail::not_fn(std::forward<P>(p)); }

struct NotNull {
    template<class Ptr>
    bool operator()(const Ptr& ptr) const noexcept {
        return static_cast<bool>(ptr);  // работает для shared_ptr, unique_ptr, raw ptr
    }
};

//!
//! Сравнения
//!

template<typename T>
auto is_equal_to(T value) { return [val = std::move(value)](auto&& x) { return std::forward<decltype(x)>(x) == val; }; }

template<typename T>
auto is_not_equal_to(T value) { return [val = std::move(value)](auto&& x) { return std::forward<decltype(x)>(x) != val; }; }

template<typename T>
auto is_less(T value) { return [val = std::move(value)](auto&& x) { return std::forward<decltype(x)>(x) < val; }; }

template<typename T>
auto is_greater(T value) { return [val = std::move(value)](auto&& x) { return std::forward<decltype(x)>(x) > val; }; }

template<typename T>
auto is_less_equal(T value) { return [val = std::move(value)](auto&& x) { return std::forward<decltype(x)>(x) <= val; }; }

template<typename T>
auto is_greater_equal(T value) { return [val = std::move(value)](auto&& x) { return std::forward<decltype(x)>(x) >= val; }; }

//!
//! Преобразования контейнера по цепочке
//!

namespace Query
{

template<class ForwardIterator>
class QueryContainer
{
public:
    using value_type = typename std::iterator_traits<ForwardIterator>::value_type;

    QueryContainer(ForwardIterator f, ForwardIterator l)
        : first(std::move(f)), last(std::move(l)) {}

    template<class A> auto Apply(A&& Op) const 
    {
        using IterT = transform_iterator<typename std::decay<A>::type, ForwardIterator>;
        return QueryContainer<IterT>(
            transform_iterator(first, Op), transform_iterator(last, std::forward<A>(Op)));    
    }

    template<class A> auto Where(A&& Op) const 
    { 
        using IterT = skip_iterator<typename std::decay<A>::type, ForwardIterator>;
        return QueryContainer<IterT>(
            skip_iterator(first, last, Op), skip_iterator(last, last, std::forward<A>(Op)));
    }

    auto ExcludeLastElement() const                            { return QueryContainer(first, first != last ? std::prev(last) : last); }

    auto Accumulate() const                                    { return std::accumulate(first, last, 0);          }
    template<class T = value_type> auto Average() const        { return average<ForwardIterator, T>(first, last); }
    auto Median() const                                        { return median(first, last);                      }

    template<class A> auto Max(A&& Op) const                   { return first != last ? *std::max_element(first, last, std::forward<A>(Op)) : value_type(); }
    template<class A> auto Min(A&& Op) const                   { return first != last ? *std::min_element(first, last, std::forward<A>(Op)) : value_type(); }
    auto Max() const                                           { return first != last ? *std::max_element(first, last)                      : value_type(); }
    auto Min() const                                           { return first != last ? *std::min_element(first, last)                      : value_type(); }
    auto Spread() const                                        { return first != last ? Max() - Min()                                       : value_type(); }

    size_t Count() const                                       { return first == last ? 0 : std::distance(first, last);  }
    size_t Count(const value_type& val) const                  { return std::count(first, last, val);                    }
    template<class A> size_t CountIf(A&& Op) const             { return std::count_if(first, last, std::forward<A>(Op)); }

    template<class A> void Do(A&& Op) const                    { std::for_each(first, last, std::forward<A>(Op));           }
    template<class Container> void Insert(Container& C) const  { std::copy(first, last, back_inserter(C));                  }
    template<class A> bool Any(A&& Op) const                   { return std::any_of(first, last, std::forward<A>(Op));      }
    template<class A> bool None(A&& Op) const                  { return std::none_of(first, last, std::forward<A>(Op));     }
    template<class A> bool All(A&& Op) const                   { return std::all_of(first, last, std::forward<A>(Op));      }

    template<class A, class... Args> void Call(A&& Op, Args&&... args) const {
        std::for_each(first, last, std::bind(std::forward<A>(Op), _1, std::forward<Args>(args)...)); }

private:
    ForwardIterator first, last;
};

//
// For
//

template<class ForwardIterator> auto For(ForwardIterator first, ForwardIterator last) { return QueryContainer(first, last); }
template<class Container>       auto For(const Container& container)                  { return QueryContainer(std::begin(container), std::end(container));     }

}  //namespace Query

#endif // QUERIES_H
