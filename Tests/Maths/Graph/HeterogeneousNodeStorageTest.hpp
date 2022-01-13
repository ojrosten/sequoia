////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/RegularTestCore.hpp"

#include "sequoia/Maths/Graph/HeterogeneousNodeStorage.hpp"

namespace sequoia::testing
{
  class test_heterogeneous_node_storage final : public regular_test
  {
  public:
    using regular_test::regular_test;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:
    void run_tests() final;

    template<class... Ts>
    class storage_tester : public maths::graph_impl::heterogeneous_node_storage<Ts...>
    {
    public:
      template<class... Args>
      constexpr explicit storage_tester(Args&&... args)
        : maths::graph_impl::heterogeneous_node_storage<Ts...>(std::forward<Args>(args)...)
      {
      }
    };

    constexpr static auto make_storage() -> storage_tester<float, int>;
  };
}
