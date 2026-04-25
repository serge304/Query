/*
 * Numeric.h
 *
 *  Created on: Nov 11, 2025
 *      Author: sergey
 */

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
typename InputIterator::value_type average(InputIterator first, InputIterator last)
{
  return (first != last) ?
	std::accumulate(first, last, 0) / std::distance(first, last) : 0;
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
    sum += SQ(*first - av);
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
typename InputIterator::value_type median(InputIterator first, InputIterator last)
{
  if (first == last)
    return 0.0;

  std::vector<typename InputIterator::value_type> vec(first, last);
  size_t n = vec.size();

  std::sort(vec.begin(), vec.end());
  if (n % 2 == 1)
    return vec[n / 2];
  else
    return 0.5*(vec[n/2 - 1] + vec[n/2]);
}

#if __cplusplus >= 202002L
inline auto median(const auto& c) { return median(std::begin(c), std::end(c)); }
#endif

#endif /* NUMERIC_H_ */
