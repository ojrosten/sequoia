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
  template<class T>
  struct is_suite : std::false_type {};

  template<class T>
  inline constexpr bool is_suite_v{is_suite<T>::value};

  template<class... Ts>
  //  requires ((is_suite_v<Ts> && ...) || ((!is_suite_v<Ts>) && ...))
  class suite;




  template<class... Ts>
  struct extract_leaves;

  template<class... Ts>
  struct extract_leaves
  {
    using type = suite<typename extract_leaves<Ts>::type...>;
  };

  template<class... Ts>
  using extract_leaves_t = typename extract_leaves<Ts...>::type;

  template<class T>
  struct extract_leaves<T>
  {
    using type = T;
  };

  template<class... Ts>
  struct extract_leaves<suite<Ts...>>
  {
    using type = extract_leaves_t<Ts...>;
  };

  template<class T, class U>
  struct extract_leaves<T, U>
  {
    using type = suite<extract_leaves_t<T>, extract_leaves_t<U>>;
  };

  template<class T, class U, class... Ts>
  struct extract_leaves<T, U, Ts...>
  {
    using type = extract_leaves_t<extract_leaves_t<T, U>, Ts...>;
  };

  template<class... Ts, class... Us>
  struct extract_leaves<suite<Ts...>, suite<Us...>>
  {
    using type = suite<Ts..., Us...>;
  };

  template<class T, class...Ts>
  struct extract_leaves<T, suite<Ts...>>
  {
    using type = extract_leaves_t<suite<extract_leaves_t<T>, extract_leaves_t<Ts>...>>;
  };

  template<class T, class...Ts>
  struct extract_leaves<suite<Ts...>, T>
  {
    using type = suite<extract_leaves_t<Ts>..., extract_leaves_t<T>>;
  };

  template<class... Ts>
  struct is_suite<suite<Ts...>> : std::true_type {};

  template<class T>
  struct to_variant;

  template<class T>
  using to_variant_t = typename to_variant<T>::type;

  template<class... Ts>
    requires ((!is_suite_v<Ts>) && ...)
  struct to_variant<suite<Ts...>>
  {
    using type = std::variant<Ts...>;
  };

  template<class... Ts>
  struct to_variant<suite<Ts...>>
  {
    using type = typename to_variant<extract_leaves_t<suite<Ts...>>>::type;
  };
  
  template<int I>
  struct foo
  {
    explicit foo(std::string) {}
  };

  template<int I>
  struct bar
  {
    explicit bar(std::string) {}
  };

  template<int I>
  struct baz
  {
    explicit baz(std::string) {}
  };

  template<class... Ts>
  //  requires ((is_suite_v<Ts> && ...) || ((!is_suite_v<Ts>) && ...))
  class suite
  {
  public:
    std::string name;
    std::tuple<Ts...> values;

    suite(std::string name, Ts&&... ts)
      : name{std::move(name)}
      , values{std::forward<Ts>(ts)...} {}
  };


  [[nodiscard]]
  std::string_view experimental_test::source_file() const noexcept
  {
    return __FILE__;
  }


  template<class Suite>
    requires is_suite_v<Suite>
  class extractor
  {
  public:
    using container_type = std::vector<to_variant_t<Suite>>;

    explicit extractor(Suite s) : m_Suite{std::move(s)} {}

    [[nodiscard]]
    container_type get()
    {
      container_type c{};

      get(m_Suite, c);

      return c;
    }
  private:
    Suite m_Suite;

    template<class... Us>
      requires ((!is_suite_v<Us>) && ...)
    static void get(suite<Us...>& s, container_type& c)
    {
      [&] <std::size_t... Is> (std::index_sequence<Is...>) {
        (c.emplace_back(std::move(std::get<Is>(s.values))), ...);
      }(std::make_index_sequence<sizeof...(Us)>{});
    }

    template<class... Us>
    static void get(suite<Us...>& s, container_type& c)
    {
      [&] <std::size_t... Is> (std::index_sequence<Is...>) {
        (get(std::get<Is>(s.values), c), ...);
      }(std::make_index_sequence<sizeof...(Us)>{});
    }
  };

  void experimental_test::run_tests()
  {
    static_assert(std::is_same_v<to_variant_t<suite<suite<foo<0>>, suite<bar<0>, baz<0>>>>, std::variant<foo<0>, bar<0>, baz<0>>>);

    extractor e{
                suite{"root",
                      suite{"suite_0", foo<0>{"foo"}},
                      suite{"suite_1", bar<0>{"bar"}, baz<0>{"baz"}},
                      suite{"suite_2",
                            suite{"suite_2_0", foo<1>{"foo1"}},
                            suite{"suite_2_1",
                                  suite{"suite_2_1_0", bar<1>{"bar2"}}
                            }
                      }
                }
              };

    auto v{e.get()};

  }
}
