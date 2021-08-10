////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ExperimentalTest.hpp"

#include "sequoia/Maths/Graph/DynamicGraph.hpp"
#include "sequoia/Maths/Graph/GraphTraversalFunctions.hpp"

namespace sequoia::testing
{
  struct foo
  {
    double x{};

    [[nodiscard]]
    friend auto operator<=>(const foo&, const foo&) noexcept = default;
  };

  template<>
  struct detailed_equality_checker<foo>
  {
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const foo& obtained, const foo& prediction)
    {
      check_equality("value of x", logger, obtained.x, prediction.x);
    }
  };

  template<class TransitionFn>
  struct transition_info
  {
    std::string description;
    TransitionFn fn;

    [[nodiscard]]
    friend bool operator==(const transition_info& lhs, const transition_info& rhs) noexcept
    {
      return (lhs.description == rhs.description) &&
        (((lhs.fn == nullptr) && (rhs.fn == nullptr) || (lhs.fn != nullptr) && (rhs.fn != nullptr)));
    }

    [[nodiscard]]
    friend bool operator!=(const transition_info& lhs, const transition_info& rhs) noexcept
    {
      return !(lhs == rhs);
    }
  };

  template<class T>
  struct object_info
  {
    std::string description;
    T object;

    [[nodiscard]]
    friend bool operator==(const object_info&, const object_info&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const object_info&, const object_info&) noexcept = default;

  };

  template<class T, invocable_r<T, const T&> TransitionFn>
  using object_transition_graph = maths::graph<maths::directed_flavour::directed, transition_info<TransitionFn>, object_info<T>>;

  [[nodiscard]]
  std::string_view experimental_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void experimental_test::run_tests()
  {
    using foo_graph = object_transition_graph<foo, std::function<foo (foo)>>;
    using edge_init_t = foo_graph::edge_init_type;
    using namespace maths;

    foo_graph g{
      { { edge_init_t{1, "Adding 1.1", [](const foo& f) -> foo { return {f.x + 1.1}; }} },

        { edge_init_t{0, "Subtracting 1.1", [](const foo& f) -> foo { return {f.x - 1.1};}},
          edge_init_t{2, "Multiplying by 2", [](const foo& f) -> foo { return {f.x * 2};}} },

        { edge_init_t{1, "Dividing by 2", [](const foo& f) -> foo { return {f.x / 2}; }} }
      },
      {{"Empty", foo{}}, {"1.1", foo{1.1}}, {"2.2", foo{2.2}}}
    };

    auto edgeFn{
      [this,&g](auto i) {

        const auto& w{i->weight()};
        const auto host{i.partition_index()}, target{i->target_node()};
        const auto preamble{std::string{"Transition from node "}.append(std::to_string(host)).append(" to ").append(std::to_string(target))};

        check_equality(append_lines(LINE(""), preamble, w.description),
                       w.fn((g.cbegin_node_weights() + host)->object),
                       (g.cbegin_node_weights() + target)->object);
      }
    };

    breadth_first_search(g, find_disconnected_t{0}, null_func_obj{}, null_func_obj{}, edgeFn);

  }
}
