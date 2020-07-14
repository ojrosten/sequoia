////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Classes for node storage that may be used in a constexpr context.

 */

#include "NodeStorage.hpp"

namespace sequoia::maths::graph_impl
{
  template
  <
    class WeightMaker,
    std::size_t N,
    bool=std::is_empty_v<typename WeightMaker::weight_proxy::value_type>
  >
  struct static_node_storage_traits
  {
    constexpr static bool throw_on_range_error{true};
    constexpr static bool static_storage_v{true};
    constexpr static std::size_t num_elements_v{N};
    template<class S> using container_type = std::array<S, N>;
  };

  template<class WeightMaker, std::size_t N>
  struct static_node_storage_traits<WeightMaker, N, true>
  {
  };
      
  template
  <
    class WeightMaker,
    std::size_t N,
    bool=std::is_empty_v<typename WeightMaker::weight_proxy::value_type>
  >
  class static_node_storage : public node_storage<WeightMaker, static_node_storage_traits<WeightMaker, N>>
  {
    using node_storage<WeightMaker, static_node_storage_traits<WeightMaker, N>>::node_storage;
  };

  template<class WeightMaker, std::size_t N>
  class static_node_storage<WeightMaker, N, true>
  {
  public:
    using weight_proxy_type = typename WeightMaker::weight_proxy;
    using weight_type       = typename weight_proxy_type::value_type;
    using size_type         = std::size_t;
        
    constexpr static_node_storage() = default;
    constexpr static_node_storage(const std::size_t) {}

    [[nodiscard]]
    constexpr friend bool operator==(const static_node_storage&, const static_node_storage&) noexcept = default;

    [[nodiscard]]
    constexpr friend bool operator!=(const static_node_storage&, const static_node_storage&) noexcept = default;

    [[nodiscard]]
    constexpr static std::size_t size() noexcept { return N; }
  protected:
    constexpr static_node_storage(const static_node_storage&)                = default;
    constexpr static_node_storage(static_node_storage&&) noexcept            = default;
    constexpr static_node_storage& operator=(const static_node_storage&)     = default;
    constexpr static_node_storage& operator=(static_node_storage&&) noexcept = default;
        
    ~static_node_storage() = default;
  };
}
