////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ExperimentalTest.hpp"
#include "TypeList.hpp"

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
  using extract_leaves_t = typename extract_leaves<Ts...>::type;

  template<class... Ts>
  struct extract_leaves<type_list<Ts...>>
  {
    using type = type_list<Ts...>;
  };


  template<class... Ts>
    requires ((!is_suite_v<Ts>) && ...)
  struct extract_leaves<suite<Ts...>>
  {
    using type = type_list<Ts...>;
  };

  template<class... Ts>
  struct extract_leaves<suite<Ts...>>
  {
    using type = extract_leaves_t<extract_leaves_t<Ts>...>;
  };

  template<class... Ts, class... Us>
  struct extract_leaves<type_list<Ts...>, type_list<Us...>>
  {
    using type = type_list<Ts..., Us...>;
  };

  template<class... Ts, class... Us>
  struct extract_leaves<suite<Ts...>, type_list<Us...>>
  {
    using type = type_list<extract_leaves_t<suite<Ts...>>, Us...>;
  };

  template<class... Ts, class... Us>
  struct extract_leaves<type_list<Ts...>, suite<Us...>>
  {
    using type = type_list<Ts..., extract_leaves_t<suite<Us...>>>;
  };

  template<class T, class U, class... Vs>
  struct extract_leaves<T, U, Vs...>
  {
    using type = extract_leaves_t<extract_leaves_t<T, U>, Vs...>;
  };

  template<class... Ts>
  struct is_suite<suite<Ts...>> : std::true_type {};

  template<class T>
  struct to_variant;

  template<class T>
  using to_variant_t = typename to_variant<T>::type;

  template<class... Ts>
  struct to_variant<type_list<Ts...>>
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

  template<class... Ts>
    requires ((is_suite_v<Ts> && ...) || ((!is_suite_v<Ts>) && ...))
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
    using suite_type     = Suite;
    using container_type = std::vector<to_variant_t<Suite>>;

    explicit extractor(Suite s) : m_Suite{std::move(s)} {}

    template<class Filter>
    [[nodiscard]]
    container_type get(Filter&& filter)
    {
      container_type c{};

      get(std::forward<Filter>(filter), m_Suite, c);

      return c;
    }
  private:
    Suite m_Suite;

    template<class Filter, class... Us>
      requires ((!is_suite_v<Us>) && ...)
    static void get(Filter&& filter, suite<Us...>& s, container_type& c)
    {
      auto emplacer{
        [&filter, &c] <class V> (V&& val) {
          if(filter(val)) c.emplace_back(std::forward<V>(val));
        }
      };

      [emplacer, &s] <std::size_t... Is> (std::index_sequence<Is...>) {
        (emplacer(std::move(std::get<Is>(s.values))), ...);
      }(std::make_index_sequence<sizeof...(Us)>{});
    }

    template<class Filter, class... Us>
    static void get(Filter&& filter, suite<Us...>& s, container_type& c)
    {
      [&] <std::size_t... Is> (std::index_sequence<Is...>) {
        (get(std::forward<Filter>(filter), std::get<Is>(s.values), c), ...);
      }(std::make_index_sequence<sizeof...(Us)>{});
    }
  };

  void experimental_test::run_tests()
  {
    static_assert(std::is_same_v<extract_leaves_t<suite<foo<0>>>, type_list<foo<0>>>);
    static_assert(std::is_same_v<extract_leaves_t<suite<foo<0>, bar<0>>>, type_list<foo<0>, bar<0>>>);
    static_assert(std::is_same_v<extract_leaves_t<suite<foo<0>, bar<0>, baz<0>>>, type_list<foo<0>, bar<0>, baz<0>>>);

    static_assert(std::is_same_v<extract_leaves_t<suite<suite<foo<0>>>>, type_list<foo<0>>>);
    static_assert(std::is_same_v<extract_leaves_t<suite<suite<foo<0>, bar<0>>>>, type_list<foo<0>, bar<0>>>);
    static_assert(std::is_same_v<extract_leaves_t<suite<suite<foo<0>>, suite<bar<0>>>>, type_list<foo<0>, bar<0>>>);
    static_assert(std::is_same_v<extract_leaves_t<suite<suite<foo<0>>, suite<bar<0>>, suite<baz<0>>>>, type_list<foo<0>, bar<0>, baz<0>>>);

    static_assert(std::is_same_v<extract_leaves_t<suite<suite<foo<0>>, suite<suite<bar<0>>>>>, type_list<foo<0>, bar<0>>>);
    
    static_assert(std::is_same_v<to_variant_t<suite<suite<foo<0>>, suite<bar<0>, baz<0>>>>, std::variant<foo<0>, bar<0>, baz<0>>>);

    extractor e{
                suite{"root",
                      suite{"suite_0", foo<0>{"foo"}},
                      suite{"suite_1", bar<0>{"bar"}, baz<0>{"baz"}},
                      suite{"suite_2",
                            suite{"suite_2_0", foo<1>{"foo1"}},
                            suite{"suite_2_1",
                                  suite{"suite_2_1_0", bar<1>{"bar1"}}
                            }
                      }
                }
              };

    using suite_t = decltype(e)::suite_type;
    using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
    static_assert(std::is_same_v<to_variant_t<suite_t>, variant_t>);

    auto v{e.get([](auto&&) { return true; })};
    variant_t init[]{foo<0>{"foo"}, bar<0>{"bar"}, baz<0>{"baz"}, foo<1>{"foo1"}, bar<1>{"bar1"}};

    check(equality, LINE(""), v, std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
  }
}
