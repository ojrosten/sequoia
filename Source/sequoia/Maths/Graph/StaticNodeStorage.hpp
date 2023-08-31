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

#include "sequoia/Core/ContainerUtilities/ArrayUtilities.hpp"
#include "sequoia/Maths/Graph/NodeStorage.hpp"

namespace sequoia::maths
{
  template
  <
    class Weight,
    std::size_t N
  >
  class static_node_storage : public node_storage_base<Weight, std::array<Weight, N>>
  {
    using base_t = node_storage_base<Weight, std::array<Weight, N>>;
  public:
    using size_type   = typename base_t::size_type;
    using weight_type = typename base_t::weight_type;

    constexpr static_node_storage() = default;

    constexpr static_node_storage(const size_type)
      : base_t{}
    {}

    constexpr static_node_storage(std::initializer_list<weight_type> weights)
      : base_t{utilities::to_array<weight_type, N>(weights, std::identity{})}
    {}
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
