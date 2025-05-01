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
  enum class check_ordering : bool {no, yes};
  
  template<class T, invocable_r<T, const T&> TransitionFn>
  struct transition_info_base
  {
    std::string description;
    TransitionFn fn;
  };

  template<class T, invocable_r<T, const T&> TransitionFn, check_ordering=check_ordering{std::totally_ordered<T>}>
  struct transition_info : transition_info_base<T, TransitionFn>
  {};

  template<std::totally_ordered T, invocable_r<T, const T&> TransitionFn>
  struct transition_info<T, TransitionFn, check_ordering::yes> : transition_info_base<T, TransitionFn>
  {
    std::weak_ordering ordering;
  };

  template<class T>
  class object_generator
  {
  public:
    template<std::invocable Fn>
      requires std::convertible_to<std::invoke_result_t<Fn>, T>
    object_generator(Fn f) : m_Fn{std::move(f)}
    {}

    object_generator(T t)
      requires std::movable<T>
      : m_Fn{[t{std::move(t)}]() -> const T& { return t; }}
    {}

    template<class... Args>
      requires (initializable_from<T, Args...> &&
               ((sizeof...(Args) != 1) || (!std::is_same_v<T, std::remove_cvref_t<Args>> && ...)))
    object_generator(Args&&... args)
      : object_generator{T{std::forward<Args>(args)...}}
    {}

    template<class InitCheckFn, class... Args>
      requires (initializable_from<T, Args...> && std::invocable<InitCheckFn, std::string, T, Args...>)
    object_generator(std::string_view message, InitCheckFn initCheckFn, const Args&... args)
      : object_generator{T{args...}}
    {
      initCheckFn(message, m_Fn(), args...);
    }

    [[nodiscard]]
    decltype(auto) operator()() const { return m_Fn(); }
  private:
    std::function<T()> m_Fn;
  };

  template<class T, check_ordering CheckOrdering=check_ordering{std::totally_ordered<T>}>
  struct transition_checker
  {
  public:
    using transition_graph
      = maths::directed_graph<transition_info<T, std::function<T(const T&)>, CheckOrdering>, object_generator<T>>;

    using size_type = typename transition_graph::size_type;

  private:
    using edge_iterator = typename transition_graph::const_edge_iterator;

    template<class CheckFn, class... Args>
    static void invoke_check_fn(const transition_graph& g, edge_iterator i, CheckFn fn, const std::string& message, object_generator<T> parentGenerator, size_type target, Args... args)
    {
      const auto& w{i->weight()};
      fn(message,
         w.fn(parentGenerator()),
         g.cbegin_node_weights()[target](),
         std::move(args)...);
    }

    template<class CheckFn, class... Args>
    static void invoke_check_fn(std::string_view description, const transition_graph& g, edge_iterator i, CheckFn fn, Args... args)
    {
      const auto [message, parentGenerator, target] {make(description, g, i)};
      const auto& w{i->weight()};
      fn(message,
         [&w,pg{parentGenerator}]() { return w.fn(pg()); },
         g.cbegin_node_weights()[target],
         parentGenerator,
         std::move(args)...);
    }
  public:
    using edge = typename transition_graph::edge_type;

    template<std::invocable<std::string, T, T> CheckFn>
    static void check(std::string_view description, const transition_graph& g, CheckFn checkFn)
    {
      auto edgeFn{
        [description,&g,checkFn](edge_iterator i) {
          const auto [message, parentGenerator, target] {make(description, g, i)};
          invoke_check_fn(g, i, checkFn, message, parentGenerator, target);
        }
      };

      check(g, edgeFn);
    }

    template<std::invocable<std::string, T, T, T> CheckFn>
    static void check(std::string_view description, const transition_graph& g, CheckFn checkFn)
    {
      auto edgeFn{
        [description,&g,checkFn](edge_iterator i) {
          const auto [message, parentGenerator, target] {make(description, g, i)};
          invoke_check_fn(g, i, checkFn, message, parentGenerator, target, parentGenerator());
        }
      };

      check(g, edgeFn);
    }

    template<std::invocable<std::string, T, T, T, size_type, size_type> CheckFn>
    static void check(std::string_view description, const transition_graph& g, CheckFn checkFn)
    {
      auto edgeFn{
        [description,&g,checkFn](edge_iterator i) {
          const auto [message, parentGenerator, target] {make(description, g, i)};
          invoke_check_fn(g, i, checkFn, message, parentGenerator, target, parentGenerator(), i.partition_index(), target);
        }
      };

      check(g, edgeFn);
    }

    template<std::invocable<std::string, T, T, T, std::weak_ordering> CheckFn>
      requires (std::totally_ordered<T>&& pseudoregular<T>)
    static void check(std::string_view description, const transition_graph& g, CheckFn checkFn)
    {
      auto edgeFn{
        [description,&g,checkFn](edge_iterator i) {
          const auto [message, parentGenerator, target] {make(description, g, i)};
          invoke_check_fn(g, i, checkFn, message, parentGenerator, target, parentGenerator(), i->weight().ordering);
        }
      };

      check(g, edgeFn);
    }

    template<std::invocable<std::string, std::function<T()>, std::function<T()>, std::function<T()>> CheckFn>
    static void check(std::string_view description, const transition_graph& g, CheckFn checkFn)
    {
      auto edgeFn{
        [description,&g,checkFn](edge_iterator i) { invoke_check_fn(description, g, i, checkFn); }
      };

      check(g, edgeFn);
    }

    template<std::invocable<std::string, std::function<T()>, std::function<T()>, std::function<T()>, std::weak_ordering> CheckFn>
      requires std::totally_ordered<T>
    static void check(std::string_view description, const transition_graph& g, CheckFn checkFn)
    {
      auto edgeFn{
        [description,&g,checkFn](edge_iterator i) { invoke_check_fn(description, g, i, checkFn, i->weight().ordering); }
      };

      check(g, edgeFn);
    }


  private:
    template<std::invocable<edge_iterator> EdgeFn>
    static void check(const transition_graph& g, EdgeFn edgeFn)
    {
      using namespace maths;
      traverse(breadth_first, g, find_disconnected_t{0}, null_func_obj{}, null_func_obj{}, edgeFn);
    }

    struct edge_fn_info
    {
      std::string message;
      object_generator<T> parentGenerator;
      size_type target;
    };

    [[nodiscard]]
    static edge_fn_info make(std::string_view description, const transition_graph& g, edge_iterator i)
    {
      const auto& w{i->weight()};
      const auto parent{i.partition_index()}, target{i->target_node()};
      return {append_lines(description,
                           std::string{"Transition from node "}.append(std::to_string(parent)).append(" to ").append(std::to_string(target)),
                           w.description),
              g.cbegin_node_weights()[parent],
              target};
    }
  };
}
