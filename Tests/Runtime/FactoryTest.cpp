////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "FactoryTest.hpp"

#include <complex>

namespace
{
  struct regular_type
  {
    regular_type(int j) : i{ j } {}

    [[nodiscard]]
    friend auto operator<=>(const regular_type&, const regular_type&) = default;

    template<class Stream>
    friend Stream& operator<<(Stream& s, const regular_type& val)
    {
      s << val.i;
      return s;
    }

    int i{};
  };

  struct move_only_type
  {
    move_only_type(int j) : i{ j } {}

    move_only_type(const move_only_type&) = delete;
    move_only_type(move_only_type&&) noexcept = default;

    move_only_type& operator=(const move_only_type&) = delete;
    move_only_type& operator=(move_only_type&&) noexcept = default;

    [[nodiscard]]
    friend auto operator<=>(const move_only_type&, const move_only_type&) = default;

    template<class Stream>
    friend Stream& operator<<(Stream& s, const move_only_type& val)
    {
      s << val.i;
      return s;
    }

    int i{};
  };

  template<std::regular T>
  struct foo
  {
    foo() = default;

    template<class U>
    foo(U u) : t(u) {}

    [[nodiscard]]
    friend auto operator<=>(const foo&, const foo&) = default;

    template<class Stream>
    friend Stream& operator<<(Stream& s, const foo& val)
    {
      s << val.t;
      return s;
    }

    T t{};
  };
}

namespace sequoia::object
{
  template<>
  struct nomenclator<foo<int>>
  {
    [[nodiscard]]
    std::string operator()() const { return "foo-int"; }
  };

  template<>
  struct nomenclator<foo<double>>
  {
    [[nodiscard]]
    std::string operator()() const { return "foo-double"; }
  };
}

namespace sequoia::testing
{
  using namespace object;

  [[nodiscard]]
  std::filesystem::path factory_test::source_file() const
  {
    return std::source_location::current().file_name();
  }


  void factory_test::run_tests()
  {
    {
      using prediction_type = std::array<std::pair<std::string, std::variant<int, double>>, 2>;

      check_exception_thrown<std::logic_error>("Empty string", [](){ factory<int, double> f{"int", ""}; });
      check_exception_thrown<std::logic_error>("Empty string", [](){ factory<int, double> f{"", "bar"}; });
      check_exception_thrown<std::logic_error>("Duplicated names", [](){ factory<int, double> f{"bar", "bar"}; });

      factory<int, double> f{"int", "double"}, g{"bar", std::string{"foo"}};

      check(equivalence, "", f, prediction_type{{{"int", 0}, {"double", 0.0}}});
      check(equivalence, "", g, prediction_type{{{"bar", 0}, {"foo", 0.0}}});

      check_semantics("", f, g);

      check_exception_thrown<std::runtime_error>("", [&f](){ return f.make("plurgh"); });

      const auto created{f.make_or<int>("plurgh")};
      check(equality, "", created, std::variant<int, double>{0});
    }

    {
      using prediction_type = std::array<std::pair<std::string, std::variant<std::vector<int>, int, std::complex<float>, double>>, 4>;

      using factory_type = factory<std::vector<int>, int, std::complex<float>, double>;

      check_exception_thrown<std::logic_error>("Duplicated names",
                                               [](){ factory_type f{"baz", "foo", "baz", "huh"}; });

      factory_type f{"vec", "int", "complex", "double"}, g{"baz", "foo", "bar", "huh"};

      check(equivalence, "", f,
                        prediction_type{{{"vec", std::vector<int>{}}, {"int", 0}, {"complex", std::complex<float>{}}, {"double", 0.0}}});

      check(equivalence, "", g,
                        prediction_type{{{"baz", std::vector<int>{}}, {"foo", 0}, {"bar", std::complex<float>{}}, {"huh", 0.0}}});

      check_semantics("", f, g);
    }

    {
      using prediction_type = std::array<std::pair<std::string, std::variant<regular_type, move_only_type>>, 2>;

      factory<regular_type, move_only_type> f{"x", "y"}, g{"make_x", "make_y"};   
      using check_type = value_tester<factory<regular_type, move_only_type>>::factory_check_type<int>;

      check(check_type{1}, "", f, prediction_type{{{"x", regular_type{1}}, {"y", move_only_type{1}}}});
      check(check_type{2}, "", g, prediction_type{{{"make_x", regular_type{2}}, {"make_y", move_only_type{2}}}});

      check_semantics("", f, g);
    }

    {
      using prediction_type = std::array<std::pair<std::string, std::variant<foo<int>, foo<double>>>, 2>;

      factory<foo<int>, foo<double>> f{}, g{"int", "double"};
      using check_type = value_tester<factory<foo<int>, foo<double>>>::factory_check_type<int>;
      
      check(equivalence, "", f, prediction_type{ {{"foo-int", foo<int>{}}, {"foo-double", foo<double>{}}} });
      check(check_type{42}, "", g, prediction_type{ {{"int", foo<int>{42}}, {"double", foo<double>{42}}} });

      check_semantics("", f, g);
    }
  }
}
