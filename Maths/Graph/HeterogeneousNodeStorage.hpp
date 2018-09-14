#pragma once

#include "NodeStorage.hpp"

namespace sequoia::maths::graph_impl
{  
  template<class... Ts>
  class heterogeneous_node_storage
  {
  public:    
    template<class... Args>
    constexpr explicit heterogeneous_node_storage(Args&&... args) : m_Weights{std::forward<Args>(args)...}
    {
    }

    constexpr std::size_t size() const noexcept
    {
      return std::tuple_size_v<std::tuple<Ts...>>;
    }

    template<std::size_t I>
    constexpr const auto& get() noexcept
    {
      return std::get<I>(m_Weights);
    }

    template<std::size_t I, class Arg, class... Args>
    constexpr void node_weight(Arg&& arg, Args&&... args)
    {
      using T = typename std::tuple_element<I, std::tuple<Ts...> >::type ;
      std::get<I>(m_Weights) = T{std::forward<Arg>(arg), std::forward<Args>(args)...};
    }

    template<std::size_t I, class Fn>
    constexpr void mutate_node_weight(Fn fn)
    {
      fn(std::get<I>(m_Weights));
    }

    friend constexpr bool operator==(const heterogeneous_node_storage& lhs, const heterogeneous_node_storage& rhs) noexcept
    {
      return lhs.m_NodeWeights == rhs.m_NodeWeights;
    }

    friend constexpr bool operator!=(const heterogeneous_node_storage& lhs, const heterogeneous_node_storage& rhs) noexcept
    {
      return !(lhs == rhs);
    }
  private:
    std::tuple<Ts...> m_Weights;
  };
}
