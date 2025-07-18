////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "SuiteFreeTest.hpp"
#include "SuiteTestingUtilities.hpp"
#include "Maths/Graph/Tree/TreeTestingUtilities.hpp"

#include "sequoia/Maths/Graph/DynamicTree.hpp"

namespace fs = std::filesystem;

namespace
{
  using namespace sequoia;
  using namespace sequoia::object;

  template<int I, template<int> class T>
  [[nodiscard]]
  std::string make_next_name(const T<I>& t)
  {
    return std::format("{}->{}", t.name, I + 1);
  }

  template<int I>
  struct foo
  {
    std::string name;

    fs::path to_path() const { return {name}; }

    [[nodiscard]]
    foo<I + 1> next() const { return {make_next_name(*this)}; }

    [[nodiscard]]
    friend bool operator==(const foo&, const foo&) noexcept = default;
  };

  template<int I>
  struct bar
  {
    std::string name;

    fs::path to_path() const { return {name}; }

    [[nodiscard]]
    friend bool operator==(const bar&, const bar&) noexcept = default;

    [[nodiscard]]
    bar<I + 1> next() const { return {make_next_name(*this)}; }
  };

  template<int I>
  struct baz
  {
    std::string name;

    fs::path to_path() const { return {name}; }

    baz(std::string s) : name{std::move(s)} {}

    baz(const baz&) = delete;
    baz(baz&&) noexcept = default;

    baz& operator=(const baz&) = delete;
    baz& operator=(baz&&) noexcept = default;

    [[nodiscard]]
    baz<I + 1> next() const { return {make_next_name(*this)}; }

    [[nodiscard]]
    friend bool operator==(const baz&, const baz&) noexcept = default;
  };

  struct to_path
  {
    template<class T>
    [[nodiscard]]
    fs::path operator()(const T& t) const { return t.to_path(); }
  };

  struct path_equivalence
  {
    [[nodiscard]]
    bool operator()(const fs::path& lhs, const fs::path& rhs) const
    {
      if(lhs.empty() && rhs.empty()) return true;

      if(!lhs.empty() && !rhs.empty())
        {
          return (lhs == rhs) || (back(lhs) == back(rhs));
        }

      return false;
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

  template<class T>
  [[nodiscard]]
  foo<-1> make_common(const T& t)
  {
    return foo<-1>{t.name};
  }

  struct stringify
  {
    template<class... Ts>
    [[nodiscard]]
    std::string operator()(const suite<Ts...>& s) const
    {
      return s.name;
    }

    template<class T>
      requires std::is_arithmetic_v<T>
    [[nodiscard]]
    std::string operator()(T t) const { return std::to_string(t);}
  };

  using filter_by_names = granular_filter<std::string, std::ranges::equal_to, item_to_name>;
  using filter_by_paths = granular_filter<fs::path, path_equivalence, to_path>;

  template<class T>
  struct format_helper
  {
    constexpr auto parse(auto& ctx) { return ctx.begin(); }

    auto format(const T& t, auto& ctx) const
    {
      return std::format_to(ctx.out(), "{}", t.name);
    }
  };
}

namespace std {
  template<int I>
  struct formatter<foo<I>> : format_helper<foo<I>> {};

  template<int I>
  struct formatter<bar<I>> : format_helper<bar<I>> {};

  template<int I>
  struct formatter<baz<I>> : format_helper<baz<I>> {};
}

namespace sequoia::testing
{
  using namespace object;
  using namespace std::string_literals;

  [[nodiscard]]
  std::filesystem::path suite_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void suite_free_test::run_tests()
  {
    test_flat_suite();
    test_nested_suite();
    test_name_filter();
    test_flat_to_tree();
    test_nested_to_tree();
  }

  void suite_free_test::test_flat_suite()
  {
    {
      using variant_t = std::variant<int, double>;
      check(equality, "", extract_leaves(suite{"root", int{42}, double{3.14}}, [](auto&&...) { return true; }), std::vector<variant_t>{ {int{42}}, {double{3.14}}});
    }

    {
      check(equality, "", extract_leaves(suite{"root", int{42}}, [](auto&&...) { return true; }), std::vector<int>{ 42 });
    }

    {
      using variant_t = std::variant<foo<0>, bar<1>>;
      variant_t init[]{bar<1>{"bar1"}};

      auto filter{filter_by_paths{{}, {{{"foo/bar1"}}}}};

      check(equality,
            "Projection from items to paths, which are compared using an equivalence relationship, rather than equality",
            extract_leaves(suite{"flat", foo<0>{"foo"}, bar<1>{"bar1"}}, filter),
            std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));

      using suites_map_t = std::optional<filter_by_paths::suites_map_type>;
      using items_map_t  = std::optional<filter_by_paths::items_map_type>;
      check(equivalence, "", filter, std::pair{suites_map_t{}, items_map_t{{{"foo/bar1", true}}}});
    }
  }

  void suite_free_test::test_nested_suite()
  {
    static_assert(std::is_same_v<leaf_extractor_t<suite<foo<0>>>, std::tuple<foo<0>>>);
    static_assert(std::is_same_v<leaf_extractor_t<suite<foo<0>, bar<0>>>, std::tuple<foo<0>, bar<0>>>);
    static_assert(std::is_same_v<leaf_extractor_t<suite<foo<0>, bar<0>, baz<0>>>, std::tuple<foo<0>, bar<0>, baz<0>>>);

    static_assert(std::is_same_v<leaf_extractor_t<suite<suite<foo<0>>>>, std::tuple<foo<0>>>);
    static_assert(std::is_same_v<leaf_extractor_t<suite<suite<foo<0>, bar<0>>>>, std::tuple<foo<0>, bar<0>>>);
    static_assert(std::is_same_v<leaf_extractor_t<suite<suite<foo<0>>, suite<bar<0>>>>, std::tuple<foo<0>, bar<0>>>);
    static_assert(std::is_same_v<leaf_extractor_t<suite<suite<foo<0>>, suite<bar<0>>, suite<baz<0>>>>, std::tuple<foo<0>, bar<0>, baz<0>>>);

    static_assert(std::is_same_v<leaf_extractor_t<suite<suite<foo<0>>, suite<suite<bar<0>>>>>, std::tuple<foo<0>, bar<0>>>);

    static_assert(std::is_same_v<leaves_to_variant_or_unique_type_t<suite<suite<foo<0>>, suite<bar<0>, baz<0>>>, std::identity>, std::variant<foo<0>, bar<0>, baz<0>>>);

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<0>{"foo"}, bar<0>{"bar"}, baz<0>{"baz"}, foo<1>{"foo1"}, bar<1>{"bar1"}};
      check(equality, "", extract_leaves(make_test_suite(), [](auto&&...) { return true; }), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<0>{"foo"}, baz<0>{"baz"}, foo<1>{"foo1"}, bar<1>{"bar1"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto & val, const Suites&...) { return val.name != "bar"; },
      };
      check(equality, "", extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<0>{"foo"}};

      auto filter{
        [] <class T, class... Suites> requires (is_suite_v<Suites> && ...) (const T&, const Suites&...) { return std::is_same_v<foo<0>, T>; }
      };
      check(equality, "", extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<0>{"foo"}, bar<0>{"bar"}, baz<0>{"baz"}, foo<1>{"foo1"}, bar<1>{"bar1"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto&, const Suites&... s) { return ((s.name == "root") || ...); }
      };

      check(equality, "", extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<0>{"foo"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto&, const Suites&... s) { return ((s.name == "suite_0") || ...); }
      };

      check(equality, "", extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{bar<0>{"bar"}, baz<0>{"baz"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto&, const Suites&... s) { return ((s.name == "suite_1") || ...); }
      };

      check(equality, "", extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<1>{"foo1"}, bar<1>{"bar1"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto&, const Suites&... s) { return ((s.name == "suite_2") || ...); },
      };

      check(equality, "", extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<1>{"foo1"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto&, const Suites&... s) { return ((s.name == "suite_2_0") || ...); }
      };

      check(equality, "", extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{bar<1>{"bar1"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto&, const Suites&... s) { return ((s.name == "suite_2_1") || ...); }
      };

      check(equality, "", extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{bar<1>{"bar1"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto&, const Suites&... s) { return ((s.name == "suite_2_1_0") || ...); }
      };

      check(equality, "", extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<1>, bar<1>, baz<1>, foo<2>, bar<2>>;
      variant_t init[]{foo<1>{"foo->1"}, bar<1>{"bar->1"}, baz<1>{"baz->1"}, foo<2>{"foo1->2"}, bar<2>{"bar1->2"}};
      check(equality, "", extract_leaves(make_test_suite(), [](auto&&...) { return true; }, [](const auto& val) { return val.next(); }), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<1>, bar<1>, baz<1>, foo<2>, bar<2>>;
      variant_t init[]{baz<1>{"baz->1"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto & val, const Suites&...) { return val.name == "baz"; },
      };
      check(equality, "", extract_leaves(make_test_suite(), filter, [](const auto& val) { return val.next(); }), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<1>, bar<1>, baz<1>, foo<2>, bar<2>>;
      variant_t init[]{foo<2>{"foo1->2"}, bar<2>{"bar1->2"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto&, const Suites&... s) { return ((s.name == "suite_2") || ...); },
      };

      check(equality, "", extract_leaves(make_test_suite(), filter, [](const auto& val) { return val.next(); }), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      check(equality, "", extract_leaves(make_test_suite(), [](auto&&...) {return true; }, [](const auto& val) -> foo<-1> { return make_common(val); }), std::vector<foo<-1>>{ {"foo"}, {"bar"}, {"baz"}, {"foo1"}, {"bar1"}});
    }
  }

  void suite_free_test::test_name_filter()
  {

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;

      {
        variant_t init[]{foo<0>{"foo"}, bar<0>{"bar"}, baz<0>{"baz"}, foo<1>{"foo1"}, bar<1>{"bar1"}};

        filter_by_names filter{std::nullopt, std::nullopt};

        check(equality, "", extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));

        using opt_suites_map_t = std::optional<filter_by_names::suites_map_type>;
        using opt_items_map_t  = std::optional<filter_by_names::items_map_type>;
        check(equivalence, "", filter, std::pair{opt_suites_map_t{}, opt_items_map_t{}});
      }

      {
        filter_by_names filter{{std::vector<std::string>{}}, std::nullopt};

        check(equality, "", extract_leaves(make_test_suite(), filter), std::vector<variant_t>{});

        using opt_suites_map_t = std::optional<filter_by_names::suites_map_type>;
        using opt_items_map_t = std::optional<filter_by_names::items_map_type>;
        check(equivalence, "", filter, std::pair{opt_suites_map_t{opt_suites_map_t::value_type{}}, opt_items_map_t{}});
      }

      {
        filter_by_names filter{std::nullopt, {std::vector<std::string>{}}};

        check(equality, "", extract_leaves(make_test_suite(), filter), std::vector<variant_t>{});

        using opt_suites_map_t = std::optional<filter_by_names::suites_map_type>;
        using opt_items_map_t  = std::optional<filter_by_names::items_map_type>;
        check(equivalence, "", filter, std::pair{opt_suites_map_t{}, opt_items_map_t{opt_items_map_t::value_type{}}});
      }

      {
        variant_t init[]{foo<1>{"foo1"}, bar<1>{"bar1"}};
   
        filter_by_names filter{{{"suite_2"s}}, {}};

        check(equality, "", extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));

        using opt_suites_map_t = std::optional<filter_by_names::suites_map_type>;
        using opt_items_map_t  = std::optional<filter_by_names::items_map_type>;
        check(equivalence, "", filter, std::pair{opt_suites_map_t{{{"suite_2"s, true}}}, opt_items_map_t{}});
      }
    }

    {

      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<1>{"foo1"}, bar<1>{"bar1"}};

      filter_by_names filter{{{"suite_2"s, "plurgh"s}}, {}};

      check(equality, "", extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));

      using opt_suites_map_t = std::optional<filter_by_names::suites_map_type>;
      using opt_items_map_t  = std::optional<filter_by_names::items_map_type>;
      check(equivalence, "", filter, std::pair{opt_suites_map_t{{{"suite_2"s, true}, {"plurgh"s, false}}}, opt_items_map_t{}});
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{bar<1>{"bar1"}};

      filter_by_names filter{{}, {{"bar1"s}}};

      check(equality, "", extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));

      using opt_suites_map_t = std::optional<filter_by_names::suites_map_type>;
      using opt_items_map_t  = std::optional<filter_by_names::items_map_type>;
      check(equivalence, "", filter, std::pair{opt_suites_map_t{}, opt_items_map_t{{{"bar1"s, true}}}});
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{bar<1>{"bar1"}};

      using filter_t = granular_filter<fs::path, std::ranges::equal_to, to_path>;
      filter_t filter{{}, {{"bar1"s}}};

      check(equality,
           "Projection from items to paths",
           extract_leaves(make_test_suite(), filter),
           std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));

      using opt_suites_map_t = std::optional<filter_t::suites_map_type>;
      using opt_items_map_t  = std::optional<filter_t::items_map_type>;
      check(equivalence, "", filter, std::pair{opt_suites_map_t{}, opt_items_map_t{{{"bar1"s, true}}}});
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{ bar<1>{"bar1"} };

      filter_by_paths filter{{}, {{"foo/bar1"s}}};

      check(equality,
            "Projection from items to paths, which are compared using an equivalence relationship, rather than equality",
            extract_leaves(make_test_suite(), filter),
            std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));

      using opt_suites_map_t = std::optional<filter_by_paths::suites_map_type>;
      using opt_items_map_t  = std::optional<filter_by_paths::items_map_type>;
      check(equivalence, "", filter, std::pair{opt_suites_map_t{}, opt_items_map_t{{{"foo/bar1"s, true}}}});
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{bar<1>{"bar1"}};

      filter_by_names filter{{}, {{"bar1"s, "far"s}}};

      check(equality, "", extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));

      using opt_suites_map_t = std::optional<filter_by_names::suites_map_type>;
      using opt_items_map_t  = std::optional<filter_by_names::items_map_type>;
      check(equivalence, "", filter, std::pair{opt_suites_map_t{}, opt_items_map_t{{{"bar1"s, true}, {"far"s, false}}}});
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{baz<0>{"baz"}, bar<1>{"bar1"}};

      check(equality, "", extract_leaves(make_test_suite(), filter_by_names{{}, {{"bar1"s, "baz"s}}}), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<1>{"foo1"}, bar<1>{"bar1"}};

      filter_by_names filter{{{"suite_2"s}}, {{"bar1"s}}};

      check(equality, "", extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));

      using opt_suites_map_t = std::optional<filter_by_names::suites_map_type>;
      using opt_items_map_t  = std::optional<filter_by_names::items_map_type>;
      check(equivalence, "", filter, std::pair{opt_suites_map_t{{{"suite_2"s, true}}}, opt_items_map_t{{{"bar1"s, true}}}});
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<1>{"foo1"}, bar<1>{"bar1"}};

      filter_by_names filter{{{{"suite_2"}, {"suite_2_0"}}}, {{{"bar1"}}}};

      check(equality, "", extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));

      using opt_suites_map_t = std::optional<filter_by_names::suites_map_type>;
      using opt_items_map_t  = std::optional<filter_by_names::items_map_type>;
      check(equivalence, "", filter, std::pair{opt_suites_map_t{{{"suite_2"s, true}, {"suite_2_0"s, true}}}, opt_items_map_t{{{"bar1"s, true}}}});
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<1>{"foo1"}, bar<1>{"bar1"}};

      filter_by_names filter{{{"suite_2"s, "aardvark"s}}, {{"bar1"s, "aardvark"s}}};

      check(equality, "", extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));

      using opt_suites_map_t = std::optional<filter_by_names::suites_map_type>;
      using opt_items_map_t  = std::optional<filter_by_names::items_map_type>;
      check(equivalence, "", filter, std::pair{opt_suites_map_t{{{"suite_2"s, true},  {"aardvark"s, false}}}, opt_items_map_t{{{"bar1"s, true}, {"aardvark"s, false}}}});
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<0>{"foo"}, foo<1>{"foo1"}, bar<1>{"bar1"}};

      check(equality, "", extract_leaves(make_test_suite(), filter_by_names{{{"suite_2"s, "suite_0"s}}, {}}), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }
  }

  void suite_free_test::test_flat_to_tree()
  {
    using namespace maths;

    static_assert(std::is_same_v<to_variant_or_unique_type_t<int, decltype([](int) -> int { return 42; })>, int>);

    {
      using tree_type = directed_tree<tree_link_direction::forward, null_weight, std::string>;

      check(equality,
            "",
            extract_tree(suite{"Numbers", int{42}}, [](auto&&...) { return false; }, stringify{}),
            tree_type{});

      check(equality,
            "",
            extract_tree(suite{"Numbers", int{42}}, [](auto&&...) { return true; }, stringify{}),
            tree_type{{"Numbers", {{"42"}}}});

      check(equality,
            "",
            extract_tree(suite{"Numbers", int{42}, long{314}}, [](auto&&...) { return true; }, stringify{}),
            tree_type{{"Numbers", {{"42"}, {"314"}}}});

      check(equality,
            "",
            extract_tree(suite{"Numbers", int{42}, long{314}}, [](const auto& val, const auto&...) { return val == 314; }, stringify{}),
            tree_type{{"Numbers", {{"314"}}}});
    }

    {
      using tree_type = directed_tree<tree_link_direction::forward, null_weight, std::variant<std::string, int>>;

      check(equality,
            "",
            extract_tree(suite{"Numbers", int{42}},
                         [](auto&&...) { return true; },
                         overloaded{
                           [] <class... Ts> (const suite<Ts...>& s) -> std::string { return s.name; },
                           [](auto x) { return x; }
                         }),
            tree_type{{"Numbers", {{42}}}});
    }

    {
      using tree_type = directed_tree<tree_link_direction::forward, null_weight, std::string>;

      filter_by_paths filter{{}, {{"foo/bar1"s}}};

      check(equality,
            "Projection from items to paths, which are compared using an equivalence relationship, rather than equality",
            extract_tree(suite{"flat", foo<0>{"foo"}, bar<1>{"bar1"}},
                         filter,
                         [](const auto& s) { return s.name; }),
            tree_type{{"flat", {{"bar1"}}}});

      using opt_suites_map_t = std::optional<filter_by_paths::suites_map_type>;
      using opt_items_map_t  = std::optional<filter_by_paths::items_map_type>;
      check(equivalence, "", filter, std::pair{opt_suites_map_t{}, opt_items_map_t{{{"foo/bar1", true}}}});
    }
  }

  void suite_free_test::test_nested_to_tree()
  {
    using namespace maths;

    using tree_type = directed_tree<tree_link_direction::forward, null_weight, std::string>;

    check(equality, "", extract_tree(make_test_suite(), [](auto&&...) { return false; }, [](const auto& s) { return s.name; }), tree_type{});

    check(equality, "", extract_tree(make_test_suite(), filter_by_names{{}, {{"foo"s}}}, [](const auto& s) { return s.name; }), tree_type{{"root", {{"suite_0", {{"foo"}}}}}});

    check(equality,
          "",
          extract_tree(make_test_suite(),
                       filter_by_names{{{"suite_2"s}}, {}},
                       [](const auto& s) { return s.name; }),
          tree_type{{"root",
                      {{"suite_2",
                        {
                          {"suite_2_0", {{"foo1"}} },
                          {"suite_2_1",
                            { {"suite_2_1_0", {{"bar1"}}} }
                          }
                        }
                       }}
                    }});

    check(equality,
          "",
          extract_tree(make_test_suite(),
                       filter_by_names{{{"suite_2"s}}, {{"bar"s}}},
                       [](const auto& s) { return s.name; }),
          tree_type{{"root",
                      {
                        {"suite_1", {{"bar"}}},
                        {"suite_2",
                          {
                            {"suite_2_0", {{"foo1"}} },
                            {"suite_2_1",
                              { {"suite_2_1_0", {{"bar1"}}} }
                            }
                          }
                        }
                      }
                    }});

    check(equality,
          "",
          extract_tree(make_test_suite(),
                       filter_by_paths{{{"suite_2"s}}, {{"foo/bar"s}}}, [](const auto& s) { return s.name; }),
          tree_type{{"root",
                      {
                        {"suite_1", {{"bar"}}},
                        {"suite_2",
                          {
                            {"suite_2_0", {{"foo1"}} },
                            {"suite_2_1",
                              { {"suite_2_1_0", {{"bar1"}}} }
                            }
                          }
                        }
                      }
                    }});
  }
}
