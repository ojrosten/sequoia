////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "FactoryTest.hpp"

#include <complex>
#include <numeric>

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


  template<class Factory, std::size_t N, class... Args>
  void factory_test::check_bulk_creation(std::string_view description,
                                         const Factory& f,
                                         const std::array<std::pair<std::string, typename Factory::vessel>, N>& prediction,
                                         const Args&... args)
  {
    // A factory reports in name order; a prediction is written in declaration order. Indices are
    // sorted rather than the prediction itself, which would require copying products that may be
    // move-only.
    std::array<std::size_t, N> byName{};
    std::iota(byName.begin(), byName.end(), std::size_t{});
    std::ranges::sort(byName, {}, [&prediction](std::size_t i){ return prediction[i].first; });

    auto checkSelection{
      [&](std::string_view mess, const std::vector<typename Factory::vessel>& actual, std::span<const std::size_t> expected){
        if(check(equality, append_lines(description, mess, "Number created"), actual.size(), expected.size()))
        {
          for(auto [product, index] : std::views::zip(actual, expected))
          {
            check(equality, append_lines(description, mess, prediction[index].first), product, prediction[index].second);
          }
        }
      }
    };

    checkSelection("make_all", f.make_all(args...), byName);
    checkSelection("make_if, admitting everything", f.make_if([](std::string_view){ return true; }, args...), byName);
    checkSelection("make_if, admitting nothing", f.make_if([](std::string_view){ return false; }, args...), {});

    const auto first{std::span{byName}.first(1)};
    checkSelection("make_if, admitting the first name",
                   f.make_if([&](std::string_view name){ return name == prediction[byName.front()].first; }, args...),
                   first);

    // Everything ordered before the midpoint, which is a genuine subset for N > 1 and exercises
    // the claim that the results come back in the factory's order rather than the prediction's.
    const auto lowerHalf{std::span{byName}.first(N / 2)};
    checkSelection("make_if, admitting the names before the midpoint",
                   f.make_if([&](std::string_view name){ return name < prediction[byName[N / 2]].first; }, args...),
                   lowerHalf);
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

      check_bulk_creation("Fundamental products", f, prediction_type{{{"int", 0}, {"double", 0.0}}});

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

      check_bulk_creation("Four products",
                          f,
                          prediction_type{{{"vec", std::vector<int>{}}, {"int", 0}, {"complex", std::complex<float>{}}, {"double", 0.0}}});
    }

    {
      using prediction_type = std::array<std::pair<std::string, std::variant<regular_type, move_only_type>>, 2>;

      factory<regular_type, move_only_type> f{"x", "y"}, g{"make_x", "make_y"};   
      using check_type = value_tester<factory<regular_type, move_only_type>>::factory_check_type<int>;

      check(check_type{1}, "", f, prediction_type{{{"x", regular_type{1}}, {"y", move_only_type{1}}}});
      check(check_type{2}, "", g, prediction_type{{{"make_x", regular_type{2}}, {"make_y", move_only_type{2}}}});

      check_semantics("", f, g);

      check_bulk_creation("Products taking an argument, one of them move-only",
                          f,
                          prediction_type{{{"x", regular_type{1}}, {"y", move_only_type{1}}}},
                          1);
    }

    {
      using prediction_type = std::array<std::pair<std::string, std::variant<foo<int>, foo<double>>>, 2>;

      factory<foo<int>, foo<double>> f{}, g{"int", "double"};
      using check_type = value_tester<factory<foo<int>, foo<double>>>::factory_check_type<int>;
      
      check(equivalence, "", f, prediction_type{ {{"foo-int", foo<int>{}}, {"foo-double", foo<double>{}}} });
      check(check_type{42}, "", g, prediction_type{ {{"int", foo<int>{42}}, {"double", foo<double>{42}}} });

      check_semantics("", f, g);

      check_bulk_creation("Products named by nomenclator", f, prediction_type{ {{"foo-int", foo<int>{}}, {"foo-double", foo<double>{}}} });
    }

    test_erasing_factory();
  }

  void factory_test::test_erasing_factory()
  {
    {
      using vessel          = std::variant<int, double>;
      using prediction_type = std::array<std::pair<std::string, vessel>, 2>;
      using factory_type    = erasing_factory<vessel>;

      factory_type f{}, g{};
      f.register_product<int>("int");
      f.register_product<double>("double");
      g.register_product<int>("bar");
      g.register_product<double>("foo");

      check(equality, "Number of products", f.size(), std::size_t{2});

      // The prediction the typed factory is held to, and the helper written for it.
      check_bulk_creation("Erased over the same products", f, prediction_type{{{"int", 0}, {"double", 0.0}}});
      check_bulk_creation("Erased, named differently", g, prediction_type{{{"bar", 0}, {"foo", 0.0}}});

      check_semantics("", f, g);

      check_exception_thrown<std::runtime_error>("Unknown name", [&f](){ return f.make("plurgh"); });
      check_exception_thrown<std::logic_error>("Empty name", [&f](){ f.register_product<int>(""); });
      check_exception_thrown<std::logic_error>("Duplicated name", [&f](){ f.register_product<int>("int"); });

      // None of those took effect.
      check(equality, "Number of products after the refusals", f.size(), std::size_t{2});
    }

    {
      using vessel          = std::variant<regular_type, move_only_type>;
      using prediction_type = std::array<std::pair<std::string, vessel>, 2>;

      erasing_factory<vessel, int> f{};
      f.register_product<regular_type>("x");
      f.register_product<move_only_type>("y");

      check_bulk_creation("Erased, taking an argument, one product move-only",
                          f,
                          prediction_type{{{"x", regular_type{1}}, {"y", move_only_type{1}}}},
                          1);
    }

    {
      // Registered out of order, and with the two product types alternating once sorted, so that
      // an ordering error changes which alternative each element holds. Five identical products
      // would make this check unfalsifiable.
      using vessel          = std::variant<int, double>;
      using prediction_type = std::array<std::pair<std::string, vessel>, 5>;

      erasing_factory<vessel> f{};
      f.register_product<double>("epsilon");
      f.register_product<double>("beta");
      f.register_product<int>("delta");
      f.register_product<int>("alpha");
      f.register_product<int>("gamma");

      check_bulk_creation("Erased, registered out of order",
                          f,
                          prediction_type{{{"epsilon", 0.0}, {"beta", 0.0}, {"delta", 0}, {"alpha", 0}, {"gamma", 0}}});
    }
  }
}
