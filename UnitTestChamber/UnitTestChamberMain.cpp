////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "AlgorithmsTest.hpp"
#include "StatisticalAlgorithmsTest.hpp"
#include "MonotonicSequenceTestingDiagnostics.hpp"
#include "MonotonicSequenceTest.hpp"

#include "TypeTraitsTest.hpp"
#include "UtilitiesTest.hpp"

#include "ArrayUtilitiesTest.hpp"
#include "ProtectiveWrapperTestingDiagnostics.hpp"
#include "ProtectiveWrapperTest.hpp"
#include "IteratorTest.hpp"

#include "DataPoolTest.hpp"

#include "ConcurrencyModelsTest.hpp"

#include "PartitionedDataTest.hpp"
#include "PartitionedDataTestingDiagnostics.hpp"

#include "StaticStackTest.hpp"
#include "StaticQueueTest.hpp"
#include "StaticPriorityQueueTest.hpp"
#include "StaticStackTestingDiagnostics.hpp"
#include "StaticQueueTestingDiagnostics.hpp"
#include "StaticPriorityQueueTestingDiagnostics.hpp"

#include "EdgeTest.hpp"
#include "EdgeTestingDiagnostics.hpp"

#include "NodeStorageTest.hpp"
#include "HeterogeneousNodeStorageTest.hpp"

#include "GraphMetaTest.hpp"

#include "DynamicGraphTestingDiagnostics.hpp"
#include "DynamicGraphInitializationTest.hpp"
#include "DynamicGraphCommonTest.hpp"
#include "DynamicGraphEmbeddedTest.hpp"
#include "DynamicGraphFixedTopologyTest.hpp"

#include "StaticGraphInitializationTest.hpp"
#include "StaticGraphManipulationsTest.hpp"

#include "HeterogeneousStaticGraphTest.hpp"

#include "DynamicGraphTraversalsTest.hpp"
#include "DynamicGraphUpdateTest.hpp"
#include "DynamicSubgraphTest.hpp"

#include "StaticGraphTraversalsTest.hpp"

#include "CommandLineArgumentsDiagnostics.hpp"
#include "CommandLineArgumentsTest.hpp"

#include "ExperimentalTest.hpp"

#include "UnitTestDiagnostics.hpp"
#include "UnitTestAllocatorDiagnostics.hpp"

#include "UnitTestRunner.hpp"

int main(int argc, char** argv)
{
  using namespace sequoia;
  using namespace unit_testing;

  try
  {
    unit_test_runner runner{argc, argv};
    const concurrency_flavour asynchronous{runner.asynchronous() ? concurrency_flavour::async : concurrency_flavour::serial};
  
    runner.add_test_family(
      test_family{
        "Diagnostics",
        false_positive_diagnostics{"False Positive Diagnostics"},
        allocator_false_positive_diagnostics{"False Positive Diagnostics"},
        false_negative_diagnostics{"Allocator False Negative Diagnostics"},
        allocator_false_negative_diagnostics{"Allocator False Negative Diagnostics"}
      }
    );

    runner.add_test_family(
      test_family{
        "CommandLine Arguments",
        commandline_arguments_test{"Unit Test"},
        commandline_arguments_false_positive_test{"False Positive Test"}
      }
    );

    runner.add_test_family(
      test_family{
        "Graph",
        test_graph_false_positives{"Graph false positive diagnostics", asynchronous}
      }
    );
      
    runner.execute();
  }
  catch(const std::exception& e)
  {
    std::cout << e.what();
  }
  catch(...)
  {
    std::cout << "Unrecognized error\n"; 
  }
  
  return 0;
}

