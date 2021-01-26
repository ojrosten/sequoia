////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "FactoryTest.hpp"

#include <complex>

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view factory_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void factory_test::run_tests()
  {
    using namespace runtime;

    {
      using prediction_type = std::array<std::pair<std::string, std::variant<int, double>>, 2>;

      factory<int, double> f{{"int", "double"}}, g{{"bar", "foo"}};

      check_equivalence(LINE(""), f, prediction_type{{{"int", 0}, {"double", 0.0}}});
      check_equivalence(LINE(""), g, prediction_type{{{"bar", 0}, {"foo", 0.0}}});

      check_semantics(LINE(""), f, g);

      check_exception_thrown<std::runtime_error>(LINE(""), [&f](){ return f.create("plurgh"); });
    }

    {
      using prediction_type = std::array<std::pair<std::string, std::variant<std::vector<int>, int, std::complex<float>, double>>, 4>;
      factory<std::vector<int>, int, std::complex<float>, double>
        f{{"vec", "int", "complex", "double"}}, g{{"baz", "foo", "bar", "huh"}};

      check_equivalence(LINE(""), f,
                        prediction_type{{{"vec", std::vector<int>{}}, {"int", 0}, {"complex", std::complex<float>{}}, {"double", 0.0}}});

      check_equivalence(LINE(""), g,
                        prediction_type{{{"baz", std::vector<int>{}}, {"foo", 0}, {"bar", std::complex<float>{}}, {"huh", 0.0}}});

      check_semantics(LINE(""), f, g);
    }
  }
}
