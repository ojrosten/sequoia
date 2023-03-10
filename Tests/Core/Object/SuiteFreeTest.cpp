////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "SuiteFreeTest.hpp"
#include "SuiteTestingUtilities.hpp"
#include "Maths/Graph/TreeTestingUtilities.hpp"

#include "sequoia/Maths/Graph/DynamicTree.hpp"

namespace sequoia::testing
{

  using namespace object;

  namespace
  {
    template<class... Ts,
             class Tree,
             class SizeType = typename Tree::size_type,
             class Fn>
    static Tree&& to_tree(suite<Ts...>& s, Tree&& tree, const SizeType parentNode, Fn fn)
    {
      const auto node{tree.add_node(parentNode, nomenclator<suite<Ts...>>::name(s))};

      [node, fn, &s] <std::size_t... Is> (std::index_sequence<Is...>) {
        (fn(node, std::move(std::get<Is>(s.values))), ...);
      }(std::make_index_sequence<sizeof...(Ts)>{});

      if(!std::distance(tree.cbegin_edges(node), tree.cend_edges(node)))
        tree.prune(node);

      return std::forward<Tree>(tree);
    }


    template<class... Ts, class Filter, class Transform, class Tree, class SizeType = typename Tree::size_type, class... PreviousSuites>
      requires (is_suite_v<PreviousSuites> && ...) && ((!is_suite_v<Ts>) && ...)
    Tree&& to_tree(suite<Ts...>& s, Filter&& filter, Transform transform, Tree&& tree, const SizeType parentNode, const PreviousSuites&... previous)
    {
      auto emplacer{
        [&] <class V> (SizeType node, V&& val) {
          if(filter(val, previous..., s))
            tree.add_node(node, transform(std::forward<V>(val)));
        }
      };

      return to_tree(s, std::forward<Tree>(tree), parentNode, emplacer);
    }

    template<class... Ts, class Filter, class Transform, class Tree, class SizeType = typename Tree::size_type, class... PreviousSuites>
      requires (is_suite_v<PreviousSuites> && ...) && ((is_suite_v<Ts>) && ...)
    Tree&& to_tree(suite<Ts...>& s, Filter&& filter, Transform transform, Tree&& tree, const SizeType parentNode, const PreviousSuites&... previous)
    {
      auto recurser{
        [&](SizeType node, auto&& val) {
          to_tree(val, std::forward<Filter>(filter), transform, tree, node, previous..., s);
        }
      };

      return to_tree(s, std::forward<Tree>(tree), parentNode, recurser);
    }

    template<class Suite,
             class Filter,
             class Transform = std::identity,
             class Tree = maths::tree<maths::directed_flavour::directed, maths::tree_link_direction::forward, maths::null_weight, to_variant_or_unique_type_t<Suite, Transform>>>
      requires is_suite_v<Suite>
    [[nodiscard]]
    Tree to_tree(Suite s, Filter&& filter, Transform transform = {})
    {
      return to_tree(s, std::forward<Filter>(filter), std::move(transform), Tree{}, Tree::npos);
    }

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

    template<class T>
    [[nodiscard]]
    foo<-1> make_common(const T& t)
    {
      return foo<-1>{t.name};
    }
  }

  [[nodiscard]]
  std::string_view suite_free_test::source_file() const noexcept
  {
    return __FILE__;
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
      check(equality, LINE(""), extract_leaves(suite{"root", int{42}, double{3.14}}, [](auto&&...) { return true; }), std::vector<variant_t>{ {int{42}}, {double{3.14}}});
    }

    {
      check(equality, LINE(""), extract_leaves(suite{"root", int{42}}, [](auto&&...) { return true; }), std::vector<int>{ 42 });
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

    static_assert(std::is_same_v<to_variant_or_unique_type_t<suite<suite<foo<0>>, suite<bar<0>, baz<0>>>, std::identity>, std::variant<foo<0>, bar<0>, baz<0>>>);

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<0>{"foo"}, bar<0>{"bar"}, baz<0>{"baz"}, foo<1>{"foo1"}, bar<1>{"bar1"}};
      check(equality, LINE(""), extract_leaves(make_test_suite(), [](auto&&...) { return true; }), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<0>{"foo"}, baz<0>{"baz"}, foo<1>{"foo1"}, bar<1>{"bar1"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto & val, const Suites&...) { return val.name != "bar"; },
      };
      check(equality, LINE(""), extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<0>{"foo"}};

      auto filter{
        [] <class T, class... Suites> requires (is_suite_v<Suites> && ...) (const T&, const Suites&...) { return std::is_same_v<foo<0>, T>; }
      };
      check(equality, LINE(""), extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<0>{"foo"}, bar<0>{"bar"}, baz<0>{"baz"}, foo<1>{"foo1"}, bar<1>{"bar1"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto&, const Suites&... s) { return ((s.name == "root") || ...); }
      };

      check(equality, LINE(""), extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<0>{"foo"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto&, const Suites&... s) { return ((s.name == "suite_0") || ...); }
      };

      check(equality, LINE(""), extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{bar<0>{"bar"}, baz<0>{"baz"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto&, const Suites&... s) { return ((s.name == "suite_1") || ...); }
      };

      check(equality, LINE(""), extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<1>{"foo1"}, bar<1>{"bar1"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto&, const Suites&... s) { return ((s.name == "suite_2") || ...); },
      };

      check(equality, LINE(""), extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<1>{"foo1"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto&, const Suites&... s) { return ((s.name == "suite_2_0") || ...); }
      };

      check(equality, LINE(""), extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{bar<1>{"bar1"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto&, const Suites&... s) { return ((s.name == "suite_2_1") || ...); }
      };

      check(equality, LINE(""), extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{bar<1>{"bar1"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto&, const Suites&... s) { return ((s.name == "suite_2_1_0") || ...); }
      };

      check(equality, LINE(""), extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<1>, bar<1>, baz<1>, foo<2>, bar<2>>;
      variant_t init[]{foo<1>{"foo->1"}, bar<1>{"bar->1"}, baz<1>{"baz->1"}, foo<2>{"foo1->2"}, bar<2>{"bar1->2"}};
      check(equality, LINE(""), extract_leaves(make_test_suite(), [](auto&&...) { return true; }, [](const auto& val) { return val.next(); }), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<1>, bar<1>, baz<1>, foo<2>, bar<2>>;
      variant_t init[]{baz<1>{"baz->1"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto & val, const Suites&...) { return val.name == "baz"; },
      };
      check(equality, LINE(""), extract_leaves(make_test_suite(), filter, [](const auto& val) { return val.next(); }), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<1>, bar<1>, baz<1>, foo<2>, bar<2>>;
      variant_t init[]{foo<2>{"foo1->2"}, bar<2>{"bar1->2"}};

      auto filter{
        [] <class... Suites> requires (is_suite_v<Suites> && ...) (const auto&, const Suites&... s) { return ((s.name == "suite_2") || ...); },
      };

      check(equality, LINE(""), extract_leaves(make_test_suite(), filter, [](const auto& val) { return val.next(); }), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      check(equality, LINE(""), extract_leaves(make_test_suite(), [](auto&&...) {return true; }, [](const auto& val) -> foo<-1> { return make_common(val); }), std::vector<foo<-1>>{ {"foo"}, {"bar"}, {"baz"}, {"foo1"}, {"bar1"}});
    }
  }

  void suite_free_test::test_name_filter()
  {
    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<1>{"foo1"}, bar<1>{"bar1"}};

      {
        auto filter{filter_by_names{{{"suite_2"}}, {}}};

        check(equality, LINE(""), extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));

        using map_t = filter_by_names::map_type;
        check(equivalence, LINE(""), filter, map_t{{{"suite_2"}, true}}, map_t{});
      }
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<1>{"foo1"}, bar<1>{"bar1"}};

      {
        auto filter{filter_by_names{{{"suite_2"}, "plurgh"}, {}}};

        check(equality, LINE(""), extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));

        using map_t = filter_by_names::map_type;
        check(equivalence, LINE(""), filter, map_t{{"plurgh", false}, {{"suite_2"}, true}}, map_t{});
      }
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{bar<1>{"bar1"}};
      auto filter{filter_by_names{{}, {{"bar1"}}}};

      check(equality, LINE(""), extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));

      using map_t = filter_by_names::map_type;
      check(equivalence, LINE(""), filter, map_t{}, map_t{{"bar1", true}});
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{bar<1>{"bar1"}};
      auto filter{filter_by_names{{}, {{"bar1"}, {"far"}}}};

      check(equality, LINE(""), extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));

      using map_t = filter_by_names::map_type;
      check(equivalence, LINE(""), filter, map_t{}, map_t{{"bar1", true}, {"far", false}});
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{baz<0>{"baz"}, bar<1>{"bar1"}};

      check(equality, LINE(""), extract_leaves(make_test_suite(), filter_by_names{{{}}, {{"bar1"}, {"baz"}}}), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<1>{"foo1"}, bar<1>{"bar1"}};
      auto filter{filter_by_names{{{"suite_2"}}, {{"bar1"}}}};

      check(equality, LINE(""), extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));

      using map_t = filter_by_names::map_type;
      check(equivalence, LINE(""), filter, map_t{{{"suite_2"}, true}}, map_t{{{"bar1"}, true}});
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<1>{"foo1"}, bar<1>{"bar1"}};
      auto filter{filter_by_names{{{"suite_2"}, {"suite_2_0"}}, {{"bar1"}}}};

      check(equality, LINE(""), extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));

      using map_t = filter_by_names::map_type;
      check(equivalence, LINE(""), filter, map_t{{{"suite_2"}, true}, {{"suite_2_0"}, true}}, map_t{{{"bar1"}, true}});
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<1>{"foo1"}, bar<1>{"bar1"}};
      auto filter{filter_by_names{{{"suite_2"}, {"aardvark"}}, {{"bar1"}, {"aardvark"}}}};

      check(equality, LINE(""), extract_leaves(make_test_suite(), filter), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));

      using map_t = filter_by_names::map_type;
      check(equivalence, LINE(""), filter, map_t{{{"aardvark"}, false}, {{"suite_2"}, true}}, map_t{{{"aardvark"}, false}, {{"bar1"}, true}});
    }

    {
      using variant_t = std::variant<foo<0>, bar<0>, baz<0>, foo<1>, bar<1>>;
      variant_t init[]{foo<0>{"foo"}, foo<1>{"foo1"}, bar<1>{"bar1"}};

      check(equality, LINE(""), extract_leaves(make_test_suite(), filter_by_names{{{"suite_2"}, {"suite_0"}}, {}}), std::vector<variant_t>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init))));
    }
  }

  void suite_free_test::test_flat_to_tree()
  {
    using namespace maths;

    using tree_type = tree<directed_flavour::directed, tree_link_direction::forward, null_weight, std::string>;
    check(equality, LINE(""), to_tree(suite{"Numbers", int{42}}, [](auto&&...) { return false; }, [](const auto& val) { return std::to_string(val); }), tree_type{});
    check(equality, LINE(""), to_tree(suite{"Numbers", int{42}}, [](auto&&...) { return true; }, [](const auto& val) { return std::to_string(val); }), tree_type{{"Numbers", {{"42"}}}});
    check(equality, LINE(""), to_tree(suite{"Numbers", int{42}, long{314}}, [](auto&&...) { return true; }, [](const auto& val) { return std::to_string(val); }), tree_type{{"Numbers", {{"42"}, {"314"}}}});
    check(equality, LINE(""), to_tree(suite{"Numbers", int{42}, long{314}}, [](const auto& val, const auto&...) { return val==314; }, [](const auto& val) { return std::to_string(val); }), tree_type{{"Numbers", {{"314"}}}});
  }

  void suite_free_test::test_nested_to_tree()
  {
    using namespace maths;

    using tree_type = tree<directed_flavour::directed, tree_link_direction::forward, null_weight, std::string>;
    check(equality, LINE(""), to_tree(make_test_suite(), [](auto&&...) { return false; }, [](const auto& s) { return s.name; }), tree_type{});
    check(equality, LINE(""), to_tree(make_test_suite(), filter_by_names{{}, {"foo"}}, [](const auto& s) { return s.name; }), tree_type{{"root", {{"suite_0", {{"foo"}}}}}});
    check(equality,
          LINE(""),
          to_tree(make_test_suite(),
          filter_by_names{{"suite_2"}, {}},
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
  }
}
