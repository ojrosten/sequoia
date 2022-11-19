////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Facility to define tests via a graph comprising states of an object and transitions between them.
 */

#include "sequoia/Core/Meta/Concepts.hpp"
#include "sequoia/Maths/Graph/DynamicGraph.hpp"
#include "sequoia/Maths/Graph/GraphTraversalFunctions.hpp"
#include "sequoia/TextProcessing/Indent.hpp"

namespace sequoia::testing
{
  template<class T, invocable_r<T, const T&> TransitionFn>
  struct transition_info_base
  {
    std::string description;
    TransitionFn fn;
  };

  template<class T, invocable_r<T, const T&> TransitionFn>
  struct transition_info : transition_info_base<T, TransitionFn>
  {};

  template<std::totally_ordered T, invocable_r<T, const T&> TransitionFn>
  struct transition_info<T, TransitionFn> : transition_info_base<T, TransitionFn>
  {
    std::weak_ordering ordering;
  };

  template<class T>
  class object_generator
  {
  public:
    template<std::invocable Fn>
      requires std::convertible_to<std::invoke_result_t<Fn>, T>
    object_generator(Fn f) : fn{std::move(f)}
    {}

    object_generator(T t)
      requires std::movable<T>
      : fn{[t{std::move(t)}]() { return t; }}
    {}

    [[nodiscard]]
    decltype(auto) operator()() const { return fn(); }
  private:
    std::function<T()> fn;
  };

  template<class T, invocable_r<T, const T&> TransitionFn = std::function<T(const T&)>>
  struct transition_checker
  {
    using transition_graph = maths::graph<maths::directed_flavour::directed, transition_info<T, TransitionFn>, object_generator<T>>;
    using edge = typename transition_graph::edge_type;

    template<std::invocable<std::string, T, T> CheckFn>
    static void check(std::string_view description, const transition_graph& g, CheckFn checkFn)
    {
      auto edgeFn{
        [description,&g,checkFn](auto i) {
          const auto [parent,target,message] {make(description, i)};

          const auto& parentObject{g.cbegin_node_weights()[parent]()};
          const auto& w{i->weight()};
          checkFn(message,
                  w.fn(parentObject),
                  g.cbegin_node_weights()[target]());
        }
      };

      check(g, edgeFn);
    }

    template<std::invocable<std::string, T, T, T> CheckFn>
    static void check(std::string_view description, const transition_graph& g, CheckFn checkFn)
    {
      auto edgeFn{
        [description,&g,checkFn](auto i) {
          const auto [parent,target,message] {make(description, i)};

          const auto& parentObject{g.cbegin_node_weights()[parent]()};
          const auto& w{i->weight()};
          checkFn(message,
                  w.fn(parentObject),
                  g.cbegin_node_weights()[target](),
                  parentObject);
        }
      };

      check(g, edgeFn);
    }

    template<std::invocable<std::string, T, T, T, std::weak_ordering> CheckFn>
      requires (std::totally_ordered<T>&& pseudoregular<T>)
    static void check(std::string_view description, const transition_graph& g, CheckFn checkFn)
    {
      auto edgeFn{
        [description,&g,checkFn](auto i) {
          const auto [parent,target,message] {make(description, i)};

          const auto parentGenerator{g.cbegin_node_weights()[parent]};
          const auto& w{i->weight()};
          checkFn(message,
                  w.fn(parentGenerator()),
                  g.cbegin_node_weights()[target](),
                  parentGenerator(),
                  w.ordering);
        }
      };

      check(g, edgeFn);
    }

    template<std::invocable<std::string, std::function<T()>, std::function<T()>, std::function<T()>> CheckFn>
    static void check(std::string_view description, const transition_graph& g, CheckFn checkFn)
    {
      auto edgeFn{
        [description,&g,checkFn](auto i) {
          const auto [parent,target,message] {make(description, i)};

          const auto parentGenerator{g.cbegin_node_weights()[parent]};
          const auto& w{i->weight()};
          checkFn(message,
                  [&]() { return w.fn(parentGenerator()); },
                  g.cbegin_node_weights()[target],
                  parentGenerator);
        }
      };

      check(g, edgeFn);
    }

    template<std::invocable<std::string, std::function<T()>, std::function<T()>, std::function<T()>, std::weak_ordering> CheckFn>
      requires std::totally_ordered<T>
    static void check(std::string_view description, const transition_graph& g, CheckFn checkFn)
    {
      auto edgeFn{
        [description,&g,checkFn](auto i) {
          const auto [parent,target,message] {make(description, i)};

          const auto parentGenerator{g.cbegin_node_weights()[parent]};
          const auto& w{i->weight()};
          checkFn(message,
                  [&]() { return w.fn(parentGenerator()); },
                  g.cbegin_node_weights()[target],
                  parentGenerator,
                  w.ordering);
        }
      };

      check(g, edgeFn);
    }


  private:
    using edge_iterator = typename transition_graph::const_edge_iterator;
    using size_type = typename transition_graph::size_type;

    template<std::invocable<edge_iterator> EdgeFn>
    static void check(const transition_graph& g, EdgeFn edgeFn)
    {
      using namespace maths;
      traverse(breadth_first, g, find_disconnected_t{0}, null_func_obj{}, null_func_obj{}, edgeFn);
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
}
