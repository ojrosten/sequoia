////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "NodeStorageAllocationTest.hpp"
#include "NodeStorageAllocationTestingUtilities.hpp"

#include "Core/DataStructures/ElementTestingUtilities.hpp"

#include "sequoia/Maths/Graph/GraphDetails.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path node_storage_allocation_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void node_storage_allocation_test::run_tests()
  {
    do_allocation_tests();
    test_permute_nodes_allocation();
  }

  void node_storage_allocation_test::test_permute_nodes_allocation()
  {
    using storage        = node_storage_tester<int>;
    using allocator_t    = shared_counting_allocator<std::size_t>;
    using destinations_t = std::vector<std::size_t, allocator_t>;

    allocator_t allocator{};

    // None, except under MSVC's debug build, which gives every vector it
    // constructs, by move included, a container proxy
    const int emptyVectorAllocations{allocations_made_by(allocator, [&allocator]() { destinations_t empty(allocator); })};

    destinations_t destinations({2, 0, 1}, allocator);
    storage s{{10, 11, 12}};

    // Of the vectors, only permute's working copy is constructed, by move
    check(equality,
          "Permuting nodes by moved destinations does not copy them",
          allocations_made_by(allocator, [&]() { s.permute_nodes(std::move(destinations)); }),
          emptyVectorAllocations);

    check(equivalence, "Permuting nodes by moved destinations", s, std::initializer_list<int>{11, 12, 10});
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void node_storage_allocation_test::test_allocation()
  {
    test_allocation_impl<int, PropagateCopy, PropagateMove, PropagateSwap>();
    test_allocation_impl<constructible_from_anything, PropagateCopy, PropagateMove, PropagateSwap>();
  }

  template<class Weight, bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void node_storage_allocation_test::test_allocation_impl()
  {
    using namespace maths::graph_impl;

    using storage = node_storage_tester<Weight, PropagateCopy, PropagateMove, PropagateSwap>;
    using allocator = storage::allocator_type;

    auto mutator{
      [](storage& s){ s.add_node(); }
    };

    auto[s,t]{check_semantics("",
                    [](){ return storage(allocator{}); },
                    [](){ return storage({1, 1, 2}, allocator{}); },
                    mutator,
                    allocation_info{node_storage_alloc_getter<storage>{}, {0_c, {1_c,1_mu}, {1_anp,1_awp}}})};

    check(equivalence, "", s, std::initializer_list<Weight>{});
    check(equivalence, "", t, std::initializer_list<Weight>{1, 1, 2});
  }
}
