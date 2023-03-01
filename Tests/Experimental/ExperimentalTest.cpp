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
  struct foo
  {
    explicit foo(std::string) {}
  };

  struct bar
  {
    explicit bar(std::string) {}
  };

  template<class... Ts>
    requires ((is_suite_v<Ts> && ...) || ((!is_suite_v<Ts>) && ...))
  class suite;

  template<class T>
  struct is_suite : std::false_type {};

  template<class... Ts>
  struct is_suite<suite<Ts...>> : std::true_type {};

  template<class T>
  inline constexpr bool is_suite_v{is_suite<T>::value};

  template<class... Ts>
    requires ((is_suite_v<Ts> && ...) || ((!is_suite_v<Ts>) && ...))
  class suite
  {
    std::string m_Name;
    std::tuple<Ts...> m_Vals;
  public:
    template<class... Args>
    suite(std::string name, Args&&... args)
      : m_Name{std::move(name)}
      , m_Vals{std::forward<Args>(args)...} {}
  };


  [[nodiscard]]
  std::string_view experimental_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void experimental_test::run_tests()
  {
    suite<suite<foo>, suite<bar>> s{"root", suite<foo>{"suite_0", "foo"}, suite<bar>{"suite_1", "bar"}};
  }
}
