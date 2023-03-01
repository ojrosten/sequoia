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
    requires ((is_suite_v<Ts> && ...) || ((!is_suite_v<Ts>) && ...))
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
  

  struct foo
  {
    explicit foo(std::string) {}
  };

  struct bar
  {
    explicit bar(std::string) {}
  };

  struct baz
  {
    explicit baz(std::string) {}
  };

  template<class... Ts>
    requires ((is_suite_v<Ts> && ...) || ((!is_suite_v<Ts>) && ...))
  class suite
  {
  public:
    std::string m_Name;
    std::tuple<Ts...> m_Vals;

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

  template<class>
  class extractor;

  template<class... Ts>
  class extractor<suite<Ts...>>
  {
  public:
    using container_type = std::vector<to_variant_t<suite<Ts...>>>;

    explicit extractor(suite<Ts...> s) : m_Suite{std::move(s)} {}

    [[nodiscard]]
    container_type get()
    {
      container_type c{};

      get(m_Suite, c);

      return c;
    }
  private:
    suite<Ts...> m_Suite;

    template<class... Us>
      requires ((!is_suite_v<Us>) && ...)
    static void get(suite<Us...>& s, container_type& c)
    {
      [&] <std::size_t... Is> (std::index_sequence<Is...>) {
        (c.emplace_back(std::move(std::get<Is>(s.m_Vals))), ...);
      }(std::make_index_sequence<sizeof...(Us)>{});
    }

    template<class... Us>
    static void get(suite<Us...>& s, container_type& c)
    {
      [&] <std::size_t... Is> (std::index_sequence<Is...>) {
        (get(std::get<Is>(s.m_Vals), c), ...);
      }(std::make_index_sequence<sizeof...(Us)>{});
    }
  };

  void experimental_test::run_tests()
  {
    static_assert(std::is_same_v<to_variant_t<suite<suite<foo>, suite<bar, baz>>>, std::variant<foo, bar, baz>>);

    extractor<suite<suite<foo>, suite<bar, baz>>> e{{"root", suite<foo>{"suite_0", "foo"}, suite<bar, baz>{"suite_1", "bar", "baz"}}};

    auto v{e.get()};

  }
}
