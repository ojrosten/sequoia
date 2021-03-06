////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Node storage for graphs with heterogeneous node weights.

 */

#include "sequoia/Maths/Graph/HeterogeneousNodeDetails.hpp"
#include <tuple>

namespace sequoia::maths::graph_impl
{
  /*! class heterogeneous_node_storage
      \brief Storage for heterogeneous node weights.
   */
  
  template<class... Ts>
  class heterogeneous_node_storage
  {
  public:
    using size_type = std::size_t;
    
    template<class... Args>
    constexpr explicit heterogeneous_node_storage(Args&&... args) : m_Weights{std::forward<Args>(args)...}
    {
    }

    [[nodiscard]]
    constexpr std::size_t size() const noexcept
    {
      return std::tuple_size_v<std::tuple<Ts...>>;
    }

    template<std::size_t I>
    [[nodiscard]]
    constexpr const auto& node_weight() const noexcept
    {
      return std::get<I>(m_Weights);
    }

    template<class T>
    [[nodiscard]]
    constexpr const auto& node_weight() const noexcept
    {
      return std::get<T>(m_Weights);
    }

    [[nodiscard]]
    constexpr const auto& all_node_weights() const noexcept
    {
      return m_Weights;
    }

    template<std::size_t I, class Arg, class... Args>
    constexpr void node_weight(Arg&& arg, Args&&... args)
    {
      using T = typename std::tuple_element<I, std::tuple<Ts...> >::type ;
      std::get<I>(m_Weights) = T{std::forward<Arg>(arg), std::forward<Args>(args)...};
    }

    template<class T, class Arg, class... Args>
    constexpr void node_weight(Arg&& arg, Args&&... args)
    {
      std::get<T>(m_Weights) = T{std::forward<Arg>(arg), std::forward<Args>(args)...};
    }
    
    template<std::size_t I, class Fn>
    constexpr void mutate_node_weight(Fn fn)
    {
      fn(std::get<I>(m_Weights));
    }

    template<class T, class Fn>
    constexpr void mutate_node_weight(Fn fn)
    {
      fn(std::get<T>(m_Weights));
    }

    [[nodiscard]]
    friend constexpr bool operator==(const heterogeneous_node_storage& lhs, const heterogeneous_node_storage& rhs) noexcept
      = default;

    [[nodiscard]]
    friend constexpr bool operator!=(const heterogeneous_node_storage& lhs, const heterogeneous_node_storage& rhs) noexcept
      = default;
  protected:    
    struct null_proxy_tag {};
    
    using weight_type = heterogeneous_tag;
    using weight_proxy_type = null_proxy_tag;
    
    constexpr heterogeneous_node_storage(const heterogeneous_node_storage& in) = default;
      
    constexpr heterogeneous_node_storage(heterogeneous_node_storage&&) noexcept = default;
        
    ~heterogeneous_node_storage() = default;
    
    constexpr heterogeneous_node_storage& operator=(const heterogeneous_node_storage& in) = default;
 
    constexpr heterogeneous_node_storage& operator=(heterogeneous_node_storage&&) noexcept = default;
  private:
    std::tuple<Ts...> m_Weights;
  };
}
