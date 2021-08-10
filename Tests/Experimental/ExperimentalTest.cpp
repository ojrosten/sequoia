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

  template<class T, invocable_r<T, const T&> TransitionFn>
  struct transition_info_base
  {
    std::string description;
    TransitionFn fn;

    // TO DO: investigate why MSVC requires this but clang does not
    [[nodiscard]]
    friend bool operator==(const transition_info_base& lhs, const transition_info_base& rhs) noexcept
    {
      return (lhs.description == rhs.description) &&
        (((lhs.fn == nullptr) && (rhs.fn == nullptr) || (lhs.fn != nullptr) && (rhs.fn != nullptr)));
    }

    [[nodiscard]]
    friend bool operator!=(const transition_info_base& lhs, const transition_info_base& rhs) noexcept
    {
      return !(lhs == rhs);
    }
  };

  template<class T, invocable_r<T, const T&> TransitionFn>
  struct transition_info : transition_info_base<T, TransitionFn>
  {};

  template<orderable T, invocable_r<T, const T&> TransitionFn>
  struct transition_info<T, TransitionFn> : transition_info_base<T, TransitionFn>
  {
    std::weak_ordering ordering;
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
    using transition_graph = maths::graph<maths::directed_flavour::directed, transition_info<T, TransitionFn>, object_info<T>>;
    using edge = typename transition_graph::edge_type;

    template<invocable<std::string, T, T> CheckFn>
    static void check(std::string_view description, const transition_graph& g, CheckFn checkFn)
    {
      auto edgeFn{
        [description,&g,checkFn](auto i) {
          const auto [parent,target,message] {make(description, i)};

          const auto& parentObject{(g.cbegin_node_weights() + parent)->object};
          const auto& w{i->weight()};
          checkFn(message,
                  w.fn(parentObject),
                  (g.cbegin_node_weights() + target)->object);
        }
      };

     check(g, edgeFn);
    }

    template<invocable<std::string, T, T, T, std::weak_ordering> CheckFn>
      requires orderable<T>
    static void check(std::string_view description, const transition_graph& g, CheckFn checkFn)
    {
      auto edgeFn{
        [description,&g,checkFn](auto i) {
          const auto [parent,target,message] {make(description, i)};

          const auto& parentObject{(g.cbegin_node_weights() + parent)->object};
          const auto& w{i->weight()};
          checkFn(message,
                  w.fn(parentObject),
                  (g.cbegin_node_weights() + target)->object,
                  parentObject,
                  w.ordering);
        }
      };

      check(g, edgeFn);
    }
  private:
    using edge_iterator = typename transition_graph::const_edge_iterator;
    using size_type = typename transition_graph::size_type;

    template<invocable<edge_iterator> EdgeFn>
    static void check(const transition_graph& g, EdgeFn edgeFn)
    {
      using namespace maths;
      breadth_first_search(g, find_disconnected_t{0}, null_func_obj{}, null_func_obj{}, edgeFn);
    }

    struct info
    {
      size_type parent, target;
      std::string message;
    };

    [[nodiscard]]
    static info make(std::string_view description, edge_iterator i)
    {
      const auto& w{i->weight()};
      const auto parent{i.partition_index()}, target{i->target_node()};
      return {parent,
              target,
              append_lines(description,
                           std::string{"Transition from node "}.append(std::to_string(parent)).append(" to ").append(std::to_string(target)),
                           w.description)};
    }
  };

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
      { { edge_t{1, "Adding 1.1", [](const foo& f) -> foo { return {f.x + 1.1}; }, std::weak_ordering::greater }},

        { edge_t{0, "Subtracting 1.1", [](const foo& f) -> foo { return {f.x - 1.1}; }, std::weak_ordering::less},
          edge_t{2, "Multiplying by 2", [](const foo& f) -> foo { return {f.x * 2}; }, std::weak_ordering::greater} },

        { edge_t{1, "Dividing by 2", [](const foo& f) -> foo { return {f.x / 2}; }, std::weak_ordering::less} }
      },
      {{"Empty", foo{}}, {"1.1", foo{1.1}}, {"2.2", foo{2.2}}}
    };

    {
      auto checker{
        [this](std::string_view description, const foo& obtained, const foo& prediction, const foo& parent, std::weak_ordering ordering) {
          check_equality(description, obtained, prediction);
          check_relation(description, within_tolerance{foo{0.1}}, obtained, prediction);
          check_semantics(description, prediction, parent, ordering);
        }
      };

      transition_checker<foo>::check(LINE(""), g, checker);
    }

    {
      auto checker{
        [this](std::string_view description, const foo& obtained, const foo& prediction) {
          check_equality(description, obtained, prediction);
          check_relation(description, within_tolerance{foo{0.1}}, obtained, prediction);
        }
      };

      transition_checker<foo>::check(LINE(""), g, checker);
    }
  }
}
