#pragma once

#include "NodeStorage.hpp"

namespace sequoia::maths::graph_impl
{
  template<class WeightProxy, std::size_t N, bool=std::is_empty_v<typename WeightProxy::value_type>>
  struct static_node_storage_traits
  {
    constexpr static bool throw_on_range_error{true};
    template<class S> using underlying_storage_type = typename data_structures::impl::static_contiguous_data<1,N>::template data<S>;
  };

  template<class WeightProxy, std::size_t N>
  struct static_node_storage_traits<WeightProxy, N, true>
  {
  };
      
  template<class WeightProxy, std::size_t N, bool=std::is_empty_v<typename WeightProxy::value_type>>
  class static_node_storage : public node_storage<WeightProxy, static_node_storage_traits<WeightProxy, N>>
  {
    using node_storage<WeightProxy, static_node_storage_traits<WeightProxy, N>>::node_storage;
  };

  template<class WeightProxy, std::size_t N>
  class static_node_storage<WeightProxy, N, true>
  {
  public:
    using weight_proxy_type = WeightProxy;
    using weight_type = typename WeightProxy::value_type;
        
    constexpr static_node_storage() = default;
    constexpr static_node_storage(const std::size_t) {}

    constexpr friend bool operator==(const static_node_storage& lhs, const static_node_storage& rhs) noexcept { return true;}
    constexpr friend bool operator!=(const static_node_storage& lhs, const static_node_storage& rhs) noexcept { return !(lhs == rhs);}

    constexpr static std::size_t size() noexcept { return N; }
  protected:
    constexpr static_node_storage(const static_node_storage&)                = default;
    constexpr static_node_storage(static_node_storage&&) noexcept            = default;
    constexpr static_node_storage& operator=(const static_node_storage&)     = default;
    constexpr static_node_storage& operator=(static_node_storage&&) noexcept = default;
        
    ~static_node_storage() = default;
  };
}
