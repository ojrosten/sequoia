////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ExperimentalTest.hpp"
#include "sequoia/Core/Object/Suite.hpp"

namespace sequoia::testing
{
  using namespace object;

  namespace
  {
    template<int I, template<int> class T>
    [[nodiscard]]
    std::string make_next_name(const T<I>& t)
    {
      return std::string{t.name}.append("->").append(std::to_string(I + 1));
    }

    template<int I>
    struct foo
    {
      std::string name;

      [[nodiscard]]
      foo<I + 1> next() const { return {make_next_name(*this)}; }

      [[nodiscard]]
      friend bool operator==(const foo&, const foo&) noexcept = default;

      template<class Stream>
      friend Stream& operator<<(Stream& s, const foo& f)
      {
        return (s << f.name);
      }
    };

    template<int I>
    struct bar
    {
      std::string name;

      [[nodiscard]]
      friend bool operator==(const bar&, const bar&) noexcept = default;

      [[nodiscard]]
      bar<I + 1> next() const { return {make_next_name(*this)}; }

      template<class Stream>
      friend Stream& operator<<(Stream& s, const bar& b)
      {
        return (s << b.name);
      }
    };

    template<int I>
    struct baz
    {
      std::string name;

      baz(std::string s) : name{std::move(s)} {}

      baz(const baz&) = delete;
      baz(baz&&) noexcept = default;

      baz& operator=(const baz&) = delete;
      baz& operator=(baz&&) noexcept = default;

      [[nodiscard]]
      baz<I + 1> next() const { return {make_next_name(*this)}; }

      [[nodiscard]]
      friend bool operator==(const baz&, const baz&) noexcept = default;

      template<class Stream>
      friend Stream& operator<<(Stream& s, const baz& b)
      {
        return (s << b.name);
      }
    };

    auto make_test_suite()
    {
      return suite{"root",
                    suite{"suite_0", foo<0>{"foo"}},
                    suite{"suite_1", bar<0>{"bar"}, baz<0>{"baz"}},
                    suite{"suite_2",
                          suite{"suite_2_0", foo<1>{"foo1"}},
                          suite{"suite_2_1",
                                suite{"suite_2_1_0", bar<1>{"bar1"}}
                          }
                    }
      };
    }
  }

  [[nodiscard]]
  std::string_view experimental_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void experimental_test::run_tests()
  {
    static_assert(std::is_same_v<extract_leaves_t<suite<foo<0>>>, std::tuple<foo<0>>>);
    static_assert(std::is_same_v<extract_leaves_t<suite<foo<0>, bar<0>>>, std::tuple<foo<0>, bar<0>>>);
    static_assert(std::is_same_v<extract_leaves_t<suite<foo<0>, bar<0>, baz<0>>>, std::tuple<foo<0>, bar<0>, baz<0>>>);

    static_assert(std::is_same_v<extract_leaves_t<suite<suite<foo<0>>>>, std::tuple<foo<0>>>);
    static_assert(std::is_same_v<extract_leaves_t<suite<suite<foo<0>, bar<0>>>>, std::tuple<foo<0>, bar<0>>>);
    static_assert(std::is_same_v<extract_leaves_t<suite<suite<foo<0>>, suite<bar<0>>>>, std::tuple<foo<0>, bar<0>>>);
    static_assert(std::is_same_v<extract_leaves_t<suite<suite<foo<0>>, suite<bar<0>>, suite<baz<0>>>>, std::tuple<foo<0>, bar<0>, baz<0>>>);

    static_assert(std::is_same_v<extract_leaves_t<suite<suite<foo<0>>, suite<suite<bar<0>>>>>, std::tuple<foo<0>, bar<0>>>);
    
    static_assert(std::is_same_v<to_variant_t<suite<suite<foo<0>>, suite<bar<0>, baz<0>>>, std::identity>, std::variant<foo<0>, bar<0>, baz<0>>>);

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<0>{"foo"}, bar<0>{"bar"}, baz<0>{"baz"}, foo<1>{"foo1"}, bar<1>{"bar1"}};
      check(equality, LINE(""), extract(make_test_suite(), [](auto&&...) { return true; }), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<0>{"foo"}, baz<0>{"baz"}, foo<1>{"foo1"}, bar<1>{"bar1"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto& val, const Suites&...) { return val.name != "bar"; },
      };
      check(equality, LINE(""), extract(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<0>{"foo"}};

      auto filter{
        [] <class T, class... Suites> requires (is_suite_v<Suites> && ...) (const T&, const Suites&...) { return std::is_same_v<foo<0>, T>; }
      };
      check(equality, LINE(""), extract(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<0>{"foo"}, bar<0>{"bar"}, baz<0>{"baz"}, foo<1>{"foo1"}, bar<1>{"bar1"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto&, const Suites&... s) { return ((s.name == "root") || ...); }
      };

      check(equality, LINE(""), extract(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<0>{"foo"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto&, const Suites&... s) { return ((s.name == "suite_0") || ...); }
      };

      check(equality, LINE(""), extract(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{bar<0>{"bar"}, baz<0>{"baz"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto&, const Suites&... s) { return ((s.name == "suite_1") || ...); }
      };

      check(equality, LINE(""), extract(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<1>{"foo1"}, bar<1>{"bar1"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto&, const Suites&... s) { return ((s.name == "suite_2") || ...); },
      };

      check(equality, LINE(""), extract(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<1>{"foo1"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto&, const Suites&... s) { return ((s.name == "suite_2_0") || ...); }
      };

      check(equality, LINE(""), extract(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{bar<1>{"bar1"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto&, const Suites&... s) { return ((s.name == "suite_2_1") || ...); }
      };

      check(equality, LINE(""), extract(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{bar<1>{"bar1"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto&, const Suites&... s) { return ((s.name == "suite_2_1_0") || ...); }
      };

      check(equality, LINE(""), extract(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<1>, bar<1>, baz<1>, foo<2>, bar<2>>;
      variant_t init[]{foo<1>{"foo->1"}, bar<1>{"bar->1"}, baz<1>{"baz->1"}, foo<2>{"foo1->2"}, bar<2>{"bar1->2"}};
      check(equality, LINE(""), extract(make_test_suite(), [](auto&&...) { return true; }, [](const auto& s) { return s.next(); }), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<1>, bar<1>, baz<1>, foo<2>, bar<2>>;
      variant_t init[]{baz<1>{"baz->1"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto & val, const Suites&...) { return val.name == "baz"; },
      };
      check(equality, LINE(""), extract(make_test_suite(), filter, [](const auto& s) { return s.next(); }), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<1>, bar<1>, baz<1>, foo<2>, bar<2>>;
      variant_t init[]{foo<2>{"foo1->2"}, bar<2>{"bar1->2"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto&, const Suites&... s) { return ((s.name == "suite_2") || ...); },
      };

      check(equality, LINE(""), extract(make_test_suite(), filter, [](const auto& s) { return s.next(); }), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }
  }
}
