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

    [[nodiscard]]
    friend foo operator-(const foo& lhs, const foo& rhs)
    {
      return {lhs.x - rhs.x};
    }
  };

  [[nodiscard]]
  std::string to_string(const foo& f)
  {
    return std::to_string(f.x);
  }

  [[nodiscard]]
  foo abs(const foo& f)
  {
    return {std::abs(f.x)};
  }

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

    // TO DO: investigate why MSVC requires this but clang does not
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

    // TO DO: investigate why MSVC requires this but clang does not
    [[nodiscard]]
    friend bool operator==(const object_info&, const object_info&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const object_info&, const object_info&) noexcept = default;

  };

  template<class T, invocable_r<T, const T&> TransitionFn=std::function<T(const T&)>>
  struct transition_checker
  {
    using transition_graph = maths::graph<maths::directed_flavour::directed, transition_info<TransitionFn>, object_info<T>>;
    using edge = typename transition_graph::edge_type;

    template<invocable<std::string, T, T> CheckFn>
    static void check(std::string_view description, const transition_graph& g, CheckFn fn)
    {
      using namespace maths;

      auto edgeFn{
        [description,&g,fn](auto i) {
          const auto& w{i->weight()};
          const auto host{i.partition_index()}, target{i->target_node()};
          const auto preamble{std::string{"Transition from node "}.append(std::to_string(host)).append(" to ").append(std::to_string(target))};
          
          fn(append_lines(description, preamble, w.description), w.fn((g.cbegin_node_weights() + host)->object), (g.cbegin_node_weights() + target)->object);
        }
      };

      breadth_first_search(g, find_disconnected_t{0}, null_func_obj{}, null_func_obj{}, edgeFn);
    }
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
    using foo_graph = transition_checker<foo>::transition_graph;
    using edge_t = transition_checker<foo>::edge;

    foo_graph g{
      { { edge_t{1, "Adding 1.1", [](const foo& f) -> foo { return {f.x + 1.1}; }} },

        { edge_t{0, "Subtracting 1.1", [](const foo& f) -> foo { return {f.x - 1.1};}},
          edge_t{2, "Multiplying by 2", [](const foo& f) -> foo { return {f.x * 2};}} },

        { edge_t{1, "Dividing by 2", [](const foo& f) -> foo { return {f.x / 2}; }} }
      },
      {{"Empty", foo{}}, {"1.1", foo{1.1}}, {"2.2", foo{2.2}}}
    };

    auto checker{
      [this](std::string_view description, const foo& obtained, const foo& prediction) {
        check_equality(description, obtained, prediction);
        check_relation(description, within_tolerance{foo{0.1}}, obtained, prediction);
      }
    };

    transition_checker<foo>::check(LINE(""), g, checker);
  }
}
