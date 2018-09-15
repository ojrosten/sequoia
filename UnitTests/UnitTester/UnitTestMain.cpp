#include "TestProtectiveWrapper.hpp"
#include "TestPartitionedData.hpp"

#include "TestDataPool.hpp"

#include "TestEdges.hpp"
#include "TestNodeStorage.hpp"
#include "TestHeterogeneousNodeStorage.hpp"
#include "TestGraph.hpp"
#include "TestStaticGraph.hpp"
#include "TestGraphInit.hpp"
#include "TestFixedTopology.hpp"
#include "TestStaticFixedTopology.hpp"
#include "TestGraphAlgorithms.hpp"
#include "TestGraphUpdate.hpp"
#include "TestGraphMeta.hpp"
#include "TestEdgeInsertion.hpp"
#include "GraphTesterDiagnostics.hpp"

#include "TestThreadingModels.hpp"

#include "TestSample.hpp"

#include "TestExperimental.hpp"

#include "TestIterator.hpp"

#include "TestAlgorithms.hpp"

#include "UnitTestDiagnostics.hpp"

#include "UnitTestRunner.hpp"

#include <vector>
#include <memory>
#include <iostream>

int main(int argc, char** argv)
{
  using namespace sequoia;
  using namespace unit_testing;

  unit_test_runner runner{argc, argv};
  
  runner.add_test_family(
    test_family{
      "Diagnostics",
      false_positive_diagnostics{"False Positive Diagnostics"},
      false_negative_diagnostics{"False Negative Diagnostics"}
    }
  );

  runner.add_test_family(
    test_family{
      "Algorithms",
      test_algorithms{"Unit Test"}
    }
  );
  
  runner.add_test_family(
    test_family{
      "Sample",
      test_sample{"Unit Test"}
    }
  );


  runner.add_test_family(
    test_family{
      "Data Pool",
      test_data_pool{"Unit Test"}
    }
  );

  runner.add_test_family(
    test_family{
      "Partitioned Data",
      test_partitioned_data{"Unit Test"}
    }
  );

  runner.add_test_family(
    test_family{
      "Protective Wrapper",
      test_protective_wrapper{"Unit Test"}
    }
  );

  runner.add_test_family(
    test_family{
      "Iterator",
      test_iterator{"Unit Test"}
    }
  );

  runner.add_test_family(
    test_family{
      "Threading Models",
      test_threading_models{"Unit Test"}
    }
  );
  
  runner.add_test_family(
    test_family{
      "Edges",
      test_edges{"Unit Test"}
    }
  );

  runner.add_test_family(
    test_family{
      "Node Storage",
      test_node_storage{"Dynamic and Static"},
      test_heterogeneous_node_storage{"Heterogeneuous"}
    }
  );

  runner.add_test_family(
    test_family{
      "Graph",
      test_graph_false_positives{"Graph false positive diagnostics"},
      test_graph_meta("Meta Tests"),
      test_graph_init("Initialization Tests"),        
      test_static_graph{"Static Graphs"},
      test_graph{"Basic Tests"},
      test_fixed_topology{"Fixed Topology"},
      test_static_fixed_topology{"Static Fixed Topology"},
      test_edge_insertion{"Edge Insertions"}  
    }
  );

  runner.add_test_family(
    test_family{
      "Graph Algorithms",
      test_graph_algorithms{"Unit Test"}
    }
  );

  runner.add_test_family(
    test_family{
      "Graph Update",
      test_graph_update{"Unit Test"}
    }
  );

  runner.add_test_family(
    test_family{
      "Experimental",
      test_experimental{"Unit Test"}
    }
  );
 
  runner.execute();
  
  return 0;
}

