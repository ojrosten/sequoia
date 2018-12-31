////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file Sample.hpp
    \brief Tools for statistical analysis.
*/

#include "TypeTraits.hpp"

#include <cmath>
#include <vector>
#include <map>
#include <limits>
#include <iostream>
#include <type_traits>
#include <mutex>
#include <atomic>
#include <numeric>

namespace sequoia::maths::statistics
{
  namespace bias
  {
    template<class Data>
    class trivial_modifier
    {
    public:
      using value_type = typename Data::value_type;
        
      trivial_modifier(const Data& data) : m_Data{data} {}

      [[nodiscard]]
      value_type operator()(const value_type sampleStandardDeviation) { return sampleStandardDeviation; }
    private:
      const Data& m_Data;
    };

    template<class Data>
    class gaussian_approx_modifier
    {
    public:
      using value_type = typename Data::value_type;
        
      gaussian_approx_modifier(const Data& data) : m_Data{data} {}

      [[nodiscard]]
      value_type operator()(const value_type sampleStandardDeviation)
      {
        return sampleStandardDeviation*sqrt((m_Data.size() - 1) / (m_Data.size() - 1.5));
      }
    private:
      const Data& m_Data;
    };
  }
      
  template<typename T, std::size_t N=0>
  class sample
  {
  public:
    using value_type = T;

    sample()
    {
      m_Data.resize(N + 1);
    }

    [[nodiscard]]
    std::size_t size() const { return m_Data.empty() ? 0 : m_Data.front().size(); }

    template<class... Args>
    void add_data(Args&&... args)
    {
      static_assert(sizeof...(args) == N + 1, "sample::add_data - Wrong number of arguments!\n");
      add_datum(args...);
    }

    template<class Arg, class... Args>
    void add_datum(Arg&& arg, Args&&... args)
    {
      constexpr std::size_t i{sizeof...(args)};
      m_Data[i].add_datum(arg);
      add_datum(args...);
    }

    void add_datum() { m_Covariance.clear(); }

    T mean(const std::size_t i) const
    {
      if(i > N)
        throw std::out_of_range("Statistical Data: mean request out of range!");

      return m_Data[i].mean();
    }

    T covariance(std::size_t i, std::size_t j) const
    {
      if(i > N || j > N)
        throw std::out_of_range("Statistical Data: covariance request out of range!");

      T cov{};
      if(i == j)
      {
        cov = m_Data[i].variance();
      }
      else
      {
        if(i > j) std::swap(i, j);
        std::pair<std::size_t, std::size_t> index{i, j};
        auto iterator = m_Covariance.find(index);
        if(iterator == m_Covariance.end())
        {
          if(std::size_t numPoints = size())
          {
            for(std::size_t k=0; k < numPoints; ++k)
            {
              cov += (m_Data[i][k] - m_Data[i].mean())*(m_Data[j][k] - m_Data[j].mean());
            }

            cov /= numPoints;

            std::lock_guard<std::mutex> lock(m_CovMutex);
            m_Covariance.insert(std::make_pair(index, cov));
          }
        }
        else
        {
          cov = iterator->second;
        }
      }

      return cov;
    }

    [[nodiscard]]
    T range(const std::size_t i) const
    {
      if(i > N)
        throw std::runtime_error("Statistical Data: range request out of range!");

      return m_Data[i].range();
    }

    [[nodiscard]]
    const sample<T, 0>& operator[](const std::size_t index) const
    {
      if(index > N)
        throw std::runtime_error("Statistical Tools: [] index out of range!");

      return m_Data[index];
    }

  private:
    std::vector<sample<T, 0>> m_Data;

    mutable std::map<std::pair<std::size_t, std::size_t>, T> m_Covariance;
    mutable std::mutex m_CovMutex;
  };

  template<typename T>
  class sample<T, 0>
  {
  public:
    using value_type = T;

    template<class... Args, class=typename std::enable_if<!utilities::same_decay<sample, Args...>()>::type>
    explicit sample(Args&&... args) : m_Data(std::forward<Args>(args)...) {}

    constexpr static T invalid() { return std::numeric_limits<T>::signaling_NaN(); }

    [[nodiscard]]
    std::size_t size() const { return m_Data.size(); }

    void add_datum(const T datum)
    {
      m_Data.push_back(datum);
      clear_cache();
    }

    [[nodiscard]]
    T operator[](const std::size_t index) const { return m_Data[index]; }

    friend std::ostream& operator<<(std::ostream& os, const sample& data)
    {
      for(auto datum : data.m_Data)
      {
        os << datum;
        os << ' ';
      }
      return os;
    }

    T mean() const
    {
      if(size() && !valid(m_Mean))
      {
        m_Mean = std::accumulate(std::cbegin(m_Data), std::cend(m_Data), T{}) / size();
      }

      return m_Mean;
    }

    T variance() const
    {
      if(size() && !valid(m_Variance))
      {
        const T sumOfSquares{std::accumulate(std::cbegin(m_Data), std::cend(m_Data), T{},
          [](const T& sum, const T& data){
            return sum + data * data;  
          }
        )};

        m_Variance = std::max(T{}, sumOfSquares / size() - mean() * mean());
      }

      return m_Variance;
    }

    [[nodiscard]]
    T sample_variance() const
    {
      return size() * variance() / (size() - 1);
    }

    [[nodiscard]]
    T standard_deviation() const
    {
      return sqrt(variance());
    }

    template<template<class> class Modifier=bias::trivial_modifier>
    [[nodiscard]]
    T sample_standard_deviation() const
    {
      return Modifier<sample<T,0>>{*this}(sqrt(sample_variance()));
    }

    [[nodiscard]]
    T maximum() const
    {
      calculate_minmax();

      return m_Max;
    }

    [[nodiscard]]
    T minimum() const
    {
      calculate_minmax();

      return m_Min;
    }

    [[nodiscard]]
    T range() const
    {
      return maximum() - minimum();
    }

    void clear()
    {
      m_Data.clear();
      clear_cache();
    }
  private:
    void calculate_minmax() const
    {      
      if(size() && !valid(m_Min))
      {

        auto min{std::numeric_limits<T>::max()};
        auto max{-std::numeric_limits<T>::max()};

        for(const auto datum : m_Data)
        {
          if(datum > max) max = datum;
          if(datum < min) min = datum;
        }

        m_Min = min;
        m_Max = max;
      }
    }


    void clear_cache()
    {
      m_Mean     = invalid();
      m_Variance = invalid();
      m_Max      = invalid();
      m_Min      = invalid();
    }

    constexpr static bool valid(const T in) { return !std::isnan(in); }

    std::vector<T> m_Data;

    mutable std::atomic<T>
      m_Mean{invalid()},
      m_Variance{invalid()},
      m_Max{invalid()},
      m_Min{invalid()};

  };
}
