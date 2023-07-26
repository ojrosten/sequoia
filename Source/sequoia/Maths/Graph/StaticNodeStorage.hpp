////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Classes for node storage that may be used in a constexpr context.

 */

#include "sequoia/Maths/Graph/NodeStorage.hpp"

namespace sequoia::maths::graph_impl
{
  template
  <
    class Weight,
    std::size_t N
  >
  struct static_node_storage_traits
  {
    constexpr static bool throw_on_range_error{true};
    constexpr static bool static_storage_v{true};
    constexpr static std::size_t num_elements_v{N};
    template<class S> using container_type = std::array<S, N>;
  };

  template<class Weight, std::size_t N>
    requires std::is_empty_v<Weight>
  struct static_node_storage_traits<Weight, N>
  {};

  template
  <
    class Weight,
    std::size_t N
  >
  class static_node_storage : public node_storage<Weight, static_node_storage_traits<Weight, N>>
  {
  public:
    using node_storage<Weight, static_node_storage_traits<Weight, N>>::node_storage;
  };

  template<class Weight, std::size_t N>
    requires std::is_empty_v<Weight>
  class static_node_storage<Weight, N>
  {
  public:
    using weight_type = Weight;
    using size_type   = std::size_t;

    constexpr static_node_storage() = default;
    constexpr static_node_storage(const std::size_t) {}

    [[nodiscard]]
    constexpr friend bool operator==(const static_node_storage&, const static_node_storage&) noexcept = default;

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
