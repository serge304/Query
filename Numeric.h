#ifndef NUMERIC_H_
#define NUMERIC_H_

#include <utility>
#include <numeric>
#include <vector>
#include <algorithm>
#include <cmath>

//! Шаблонный алгоритм для вычисления среднего значения элементов контейнера
/**
 * @param first Первый итератор
 * @param last  Последний итератор
 * @return Среднее значение
 */
template<class InputIterator>
auto average(InputIterator first, InputIterator last)
  -> typename std::iterator_traits<InputIterator>::value_type
{
  using value_type = typename std::iterator_traits<InputIterator>::value_type;

  return (first != last) ?
	std::accumulate(first, last, value_type()) / std::distance(first, last) : value_type();
}

//! Шаблонный алгоритм для вычисления среднего квадратического отклонения элементов контейнера
/**
 * @param first Первый итератор
 * @param last  Последний итератор
 * @return Среднее квадратическое отклонение
 */
template<class InputIterator>
double standard_deviation(InputIterator first, InputIterator last)
{
  int n = std::distance(first, last);
  if (n < 2)
    return 0.0;

  double av = average(first, last);
  double sum = 0.0;
  while (first != last) {
    double delta = *first - av;
    sum += delta*delta;
    ++first;
  }

  return std::sqrt(sum/(n - 1));
}

//! Шаблонный алгоритм для вычисления медианы элементов контейнера
/**
 * @note При выполнении функции элементы копируются во временный вектор для сортировки
 * @param first Первый итератор
 * @param last  Последний итератор
 * @return Среднее квадратическое отклонение
 */
template<class InputIterator>
auto median(InputIterator first, InputIterator last)
  -> typename std::iterator_traits<InputIterator>::value_type
{
  using value_type = typename std::iterator_traits<InputIterator>::value_type;

  if (first == last)
    return value_type();

  std::vector<value_type> vec(first, last);
  size_t n = vec.size();

  std::sort(vec.begin(), vec.end());
  if (n % 2 == 1)
    return vec[n / 2];
  else
    return (vec[n/2 - 1] + vec[n/2]) / 2;
}

#endif /* NUMERIC_H_ */
