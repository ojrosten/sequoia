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
    constexpr const auto& node_weight() noexcept
    {
      return std::get<I>(m_Weights);
    }

    template<class T>
    constexpr const auto& node_weight() noexcept
    {
      return std::get<T>(m_Weights);
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

    friend constexpr bool operator==(const heterogeneous_node_storage& lhs, const heterogeneous_node_storage& rhs) noexcept
    {
      return lhs.m_NodeWeights == rhs.m_NodeWeights;
    }

    friend constexpr bool operator!=(const heterogeneous_node_storage& lhs, const heterogeneous_node_storage& rhs) noexcept
    {
      return !(lhs == rhs);
    }
  protected:
    constexpr heterogeneous_node_storage(const heterogeneous_node_storage& in) = default;
      
    constexpr heterogeneous_node_storage(heterogeneous_node_storage&&) noexcept = default;
        
    ~heterogeneous_node_storage() = default;
    
    constexpr heterogeneous_node_storage& operator=(const heterogeneous_node_storage& in) = default;
 
    constexpr heterogeneous_node_storage& operator=(heterogeneous_node_storage&&) noexcept = default;
  private:
    std::tuple<Ts...> m_Weights;
  };
}
