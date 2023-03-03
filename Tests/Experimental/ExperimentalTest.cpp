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

  template<int I>
  struct foo
  {
    std::string name;

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

    baz(const baz&)     = delete;
    baz(baz&&) noexcept = default;

    baz& operator=(const baz&)     = delete;
    baz& operator=(baz&&) noexcept = default;

    [[nodiscard]]
    friend bool operator==(const baz&, const baz&) noexcept = default;

    template<class Stream>
    friend Stream& operator<<(Stream& s, const baz& b)
    {
      return (s << b.name);
    }
  };

  [[nodiscard]]
  std::string_view experimental_test::source_file() const noexcept
  {
    return __FILE__;
  }

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
    
    static_assert(std::is_same_v<to_variant_t<suite<suite<foo<0>>, suite<bar<0>, baz<0>>>>, std::variant<foo<0>, bar<0>, baz<0>>>);

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<0>{"foo"}, bar<0>{"bar"}, baz<0>{"baz"}, foo<1>{"foo1"}, bar<1>{"bar1"}};
      check(equality, LINE(""), extract(make_test_suite(), [](auto&&) { return true; }), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<0>{"foo"}, baz<0>{"baz"}, foo<1>{"foo1"}, bar<1>{"bar1"}};

      variant_visitor filter{
        [] <class T> requires is_suite_v<T>(T&&) { return true; },
        [](const auto& val) { return val.name != "bar"; }
      };
      check(equality, LINE(""), extract(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<0>{"foo"}};

      variant_visitor filter{
        [] <class T> requires is_suite_v<T>(const T&) { return true; },
        [] <class T> (const T&) { return std::is_same_v<foo<0>, T>; }
      };
      check(equality, LINE(""), extract(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }
  }
}
