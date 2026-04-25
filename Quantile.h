/*
 * Quantile.h
 *
 *  Created on: Oct 27, 2025
 *      Author: sergey
 */

#ifndef QUANTILE_H_
#define QUANTILE_H_

#include <vector>

//
//  Квантили
//

//! Шаблонный алгоритм для вычисления квантилей элементов контейнера
/**
 * @note При выполнении функции элементы копируются во временный вектор для сортировки
 * @param first Первый итератор
 * @param last  Последний итератор
 * @return Вектор квантилей из пяти элементов [0%, 25%, 50%, 75%, 100%]
 */
template<class It>
std::vector<typename It::value_type> Quantile(It first, It last, typename It::value_type shift = 1.0/3.0)
{
  typedef typename It::value_type T;

  std::vector<T> data, quantiles;
  if (first != last)
  {
    data.assign(first, last);
    std::sort(data.begin(), data.end());

    if (data.size() == 1)
      quantiles.insert(quantiles.begin(), 5, data[0]);
    else
    {
      int n = data.size();
      T m = n + shift;
      T k = 0.5*(shift + 1);

      quantiles.push_back(data.front());
      T prob = 0.25;
      for (size_t i = 0; i < 3; ++i)
      {
        T ndx = Lerp<T>(0, m, prob) - k;
        T leftData  = data[positive_filter(std::max((int)std::floor(ndx), 0))];
        T rightData = data[positive_filter(std::min((int)std::ceil(ndx), n - 1))];
        T Q = Lerp<T>(leftData, rightData, ndx - std::floor(ndx));
        quantiles.push_back(Q);

        prob += 0.25;
      }
      quantiles.push_back(data.back());
    }
  }
  return quantiles;
}

//! Шаблонный алгоритм для получения заданного квантиля элементов контейнера
/**
 * @note При выполнении функции элементы копируются во временный вектор для сортировки
 * @param first Первый итератор
 * @param last  Последний итератор
 * @param ndx   Индекс квантиля
 * @return Один из пяти квантилей [0%, 25%, 50%, 75%, 100%] или ноль
 */
template<class It>
typename It::value_type GetQuantile(It first, It last, size_t ndx)
{
  std::vector<typename It::value_type> quantiles = Quantile(first, last);
  return !quantiles.empty() && ndx < quantiles.size() ? quantiles[ndx] : 0.0f;
}

//! Шаблонный алгоритм для получения медианного интервала значений элементов контейнера. Элементы вне интервала считаются выбросами.
/**
 * @note При выполнении функции элементы копируются во временный вектор для сортировки
 * @param first Первый итератор
 * @param last  Последний итератор
 * @param k     Параметр ширины интервала
 * @return Интервал
 */
template<class It>
auto SequenceRange(It first, It last, typename It::value_type k = 4.5)
{
  typedef typename It::value_type T;
  std::vector<T> quantiles = Quantile(first, last);
  if (!quantiles.empty())
  {
    double interval = quantiles[3] - quantiles[1];
    return std::make_pair(quantiles[1] - k*interval, quantiles[3] + k*interval);
  }
  return {};
}

template<typename Q>
auto SequenceRange(const Q& v, typename Q::value_type k = 4.5)
{
  return SequenceRange(v.begin(), v.end(), k);
}

// удаление отклонений из контейнера

//! Шаблонный алгоритм для перемещения выбросов в конец контейнера
/**
 * @note При выполнении функции элементы копируются во временный вектор для сортировки
 * @param first Первый итератор
 * @param last  Последний итератор
 * @return Итератор на конец последовательности элементов без выбросов
 */
template <class It>
It RemoveOutliers(It first, It last)
{
  return std::remove_if(first, last, 
    [](const auto& x, const auto& r) { return x < r.first || x > r.second;};
  );
}

//! Шаблонный алгоритм для удаления выбросов из контейнера
/**
 * @note При выполнении функции элементы копируются во временный вектор для сортировки
 * @param first Первый итератор
 * @param last  Последний итератор
 * @return Итератор на конец последовательности элементов без выбросов
 */
template<typename Q>
void EraseOutliers(Q& v)
{
  v.erase(RemoveOutliers(v.begin(), v.end()), v.end());
}

#endif /* QUANTILE_H_ */
