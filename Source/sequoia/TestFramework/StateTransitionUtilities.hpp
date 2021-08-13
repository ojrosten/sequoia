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
  namespace impl
  {
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
               (((lhs.fn == nullptr) && (rhs.fn == nullptr)) || ((lhs.fn != nullptr) && (rhs.fn != nullptr)));
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
      template<class... Args>
        requires (!resolve_to_copy_v<object_info, Args...>)
      object_info(Args&&... args)
        : fn{[...args{args}] () { return T{args...}; }}
      {}

      std::function<T()> fn;

      // TO DO: investigate why MSVC requires this but clang does not
      [[nodiscard]]
      friend bool operator==(const object_info& lhs, const object_info& rhs) noexcept
      {
        return ((lhs.fn == nullptr) && (rhs.fn == nullptr)) || ((lhs.fn != nullptr) && (rhs.fn != nullptr));
      }

      [[nodiscard]]
      friend bool operator!=(const object_info& lhs, const object_info& rhs) noexcept
      {
        return !(lhs == rhs);
      }

    };
  }

  template<class T, invocable_r<T, const T&> TransitionFn = std::function<T(const T&)>>
  struct transition_checker
  {
    using transition_graph = maths::graph<maths::directed_flavour::directed, impl::transition_info<T, TransitionFn>, impl::object_info<T>>;
    using edge = typename transition_graph::edge_type;

    template<invocable<std::string, T, T> CheckFn>
    static void check(std::string_view description, const transition_graph& g, CheckFn checkFn)
    {
      auto edgeFn{
        [description,&g,checkFn](auto i) {
          const auto [parent,target,message] {make(description, i)};

          const auto& parentObject{(g.cbegin_node_weights() + parent)->fn()};
          const auto& w{i->weight()};
          checkFn(message,
                  w.fn(parentObject),
                  (g.cbegin_node_weights() + target)->fn());
        }
      };

      check(g, edgeFn);
    }

    template<invocable<std::string, std::function<T()>, std::function<T()>, std::function<T()>, std::weak_ordering> CheckFn>
      requires orderable<T>
    static void check(std::string_view description, const transition_graph& g, CheckFn checkFn)
    {
      auto edgeFn{
        [description,&g,checkFn](auto i) {
          const auto [parent,target,message] {make(description, i)};

          const auto parentIter{(g.cbegin_node_weights() + parent)};
          const auto& w{i->weight()};
          checkFn(message,
                  [&]() { return w.fn(parentIter->fn()); },
                  (g.cbegin_node_weights() + target)->fn,
                  parentIter->fn,
                  w.ordering);
        }
      };

      check(g, edgeFn);
    }

    template<invocable<std::string, T, T, T, std::weak_ordering> CheckFn>
      requires (orderable<T> && pseudoregular<T>)
    static void check(std::string_view description, const transition_graph& g, CheckFn checkFn)
    {
      auto edgeFn{
        [description,&g,checkFn](auto i) {
          const auto [parent,target,message] {make(description, i)};

          const auto parentIter{(g.cbegin_node_weights() + parent)};
          const auto& w{i->weight()};
          checkFn(message,
                  w.fn(parentIter->fn()),
                  (g.cbegin_node_weights() + target)->fn(),
                  parentIter->fn(),
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
}
