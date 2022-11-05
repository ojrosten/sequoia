////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ExperimentalTest.hpp"

namespace sequoia::testing
{
  template<class... Ts>
  struct flatten;

  template<class... Ts>
  using flatten_t = typename flatten<Ts...>::type;


  template<class... Ts>
  struct flatten
  {
    using type = type_list<flatten_t<Ts>...>;
  };

  template<class T>
  struct flatten<T>
  {
    using type = T;
  };

  template<class... Ts>
  struct flatten<type_list<Ts...>>
  {
    using type = flatten_t<Ts...>;
  };

  template<class T, class U>
  struct flatten<T, U>
  {
    using type = type_list<flatten_t<T>, flatten_t<U>>;
  };

  template<class T, class U, class... Ts>
  struct flatten<T, U, Ts...>
  {
    using type = flatten_t<flatten_t<T, U>, Ts...>;
  };

  template<class... Ts, class... Us>
  struct flatten<type_list<Ts...>, type_list<Us...>>
  {
    using type = type_list<Ts..., Us...>;
  };

  template<class T, class...Ts>
  struct flatten<T, type_list<Ts...>>
  {
    using type = flatten_t<type_list<flatten_t<T>, flatten_t<Ts>...>>;
  };

  template<class T, class...Ts>
  struct flatten<type_list<Ts...>, T>
  {
    using type = type_list<flatten_t<Ts>..., flatten_t<T>>;
  };



  [[nodiscard]]
  std::string_view experimental_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void experimental_test::run_tests()
  {
    static_assert(std::is_same_v<flatten_t<int>, int>);
    static_assert(std::is_same_v<flatten_t<type_list<int>>, int>);
    static_assert(std::is_same_v<flatten_t<type_list<int, char>>, type_list<int, char>>);

    static_assert(std::is_same_v<flatten_t<int, type_list<char, float>>, type_list<int, char, float>>);
    static_assert(std::is_same_v<flatten_t<type_list<char, float>, int>, type_list<char, float, int>>);

    static_assert(std::is_same_v<flatten_t<int, double, type_list<char, float>> , type_list<int, double, char, float>> );
    static_assert(std::is_same_v<flatten_t<type_list<char, float>, int, double>, type_list<char, float, int, double>>);


    static_assert(std::is_same_v<flatten_t<type_list<int, type_list<char,float>>>, type_list<int, char, float>>);

    static_assert(std::is_same_v<flatten_t<type_list<int, type_list<char, type_list<float, double>>>>,
      type_list<int, char, float, double>>);

    static_assert(std::is_same_v<flatten_t<type_list<int, type_list<type_list<char, unsigned>, type_list<float, double>>>>,
      type_list<int, char, unsigned, float, double>>);

    static_assert(std::is_same_v<flatten_t<type_list<int, type_list<type_list<char, unsigned>, type_list<float, type_list<double, long>>>>>,
      type_list<int, char, unsigned, float, double, long>>);

  }
}
