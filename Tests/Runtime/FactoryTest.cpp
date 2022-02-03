////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "FactoryTest.hpp"

#include <complex>

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view factory_test::source_file() const noexcept
  {
    return __FILE__;
  }

  struct x
  {
    x(int j) : i{j} {}

    [[nodiscard]]
    friend auto operator<=>(const x&, const x&) = default;

    template<class Stream>
    friend Stream& operator<<(Stream& s, const x& val)
    {
      s << val.i;
      return s;
    }

    int i{};
  };

  struct y
  {
    y(int j) : i{j} {}

    [[nodiscard]]
    friend auto operator<=>(const y&, const y&) = default;

    template<class Stream>
    friend Stream& operator<<(Stream& s, const y& val)
    {
      s << val.i;
      return s;
    }

    int i{};
  };

  void factory_test::run_tests()
  {
    using namespace runtime;

    {
      using prediction_type = std::array<std::pair<std::string, std::variant<int, double>>, 2>;

      check_exception_thrown<std::logic_error>(LINE("Empty string"), [](){ factory<int, double> f{{"int", ""}}; });
      check_exception_thrown<std::logic_error>(LINE("Empty string"), [](){ factory<int, double> f{{"", "bar"}}; });
      check_exception_thrown<std::logic_error>(LINE("Duplicated names"), [](){ factory<int, double> f{{"bar", "bar"}}; });

      factory<int, double> f{{"int", "double"}}, g{{"bar", std::string{"foo"}}};

      check(equivalence, LINE(""), f, prediction_type{{{"int", 0}, {"double", 0.0}}});
      check(equivalence, LINE(""), g, prediction_type{{{"bar", 0}, {"foo", 0.0}}});

      check_semantics(LINE(""), f, g);

      check_exception_thrown<std::runtime_error>(LINE(""), [&f](){ return f.create("plurgh"); });

      const auto created{f.create_or<int>("plurgh")};
      check(equality, LINE(""), created, std::variant<int, double>{0});
    }

    {
      using prediction_type = std::array<std::pair<std::string, std::variant<std::vector<int>, int, std::complex<float>, double>>, 4>;

      using factory_type = factory<std::vector<int>, int, std::complex<float>, double>;

      check_exception_thrown<std::logic_error>(LINE("Duplicated names"),
                                               [](){ factory_type f{{"baz", "foo", "baz", "huh"}}; });

      factory_type f{{"vec", "int", "complex", "double"}}, g{{"baz", "foo", "bar", "huh"}};

      check(equivalence, LINE(""), f,
                        prediction_type{{{"vec", std::vector<int>{}}, {"int", 0}, {"complex", std::complex<float>{}}, {"double", 0.0}}});

      check(equivalence, LINE(""), g,
                        prediction_type{{{"baz", std::vector<int>{}}, {"foo", 0}, {"bar", std::complex<float>{}}, {"huh", 0.0}}});

      check_semantics(LINE(""), f, g);
    }

    {
      using prediction_type = std::array<std::pair<std::string, std::variant<x, y>>, 2>;

      factory<x, y> f{{"x", "y"}, 1}, g{{"x", "y"}, 2};

      check(equivalence, LINE(""), f, prediction_type{{{"x", x{1}}, {"y", y{1}}}});
      check(equivalence, LINE(""), g, prediction_type{{{"x", x{2}}, {"y", y{2}}}});

      check_semantics(LINE(""), f, g);
    }
  }
}
