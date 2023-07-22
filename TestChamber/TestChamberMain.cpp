////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestIncludes.hpp"

#include <iostream>

int main(int argc, char** argv)
{
  try
  {
    using namespace sequoia;
    using namespace testing;
    using namespace object;
    using namespace std::literals::chrono_literals;

    test_runner runner{argc,
                       argv,
                       "Oliver J. Rosten",
                       {"TestChamber/TestChamberMain.cpp", {}, "TestCommon/TestIncludes.hpp"}};

    runner.add_test_suite(
      "Experimental",
      experimental_test{"Unit Test"}
    );

    runner.add_test_suite(
      "Graph",
      suite{
        "Manipulations",
        suite{
          "Directed",
          dynamic_directed_graph_unweighted_test{"Directed Graph Unweighted Tests"},
          dynamic_directed_graph_faithful_faithful_test{"Directed Graph Faithful-Faithful Tests"},
          dynamic_directed_graph_faithful_pool_test{"Directed Graph Faithful-Pool Tests"},
          dynamic_directed_graph_pool_faithful_test{"Directed Graph Pool-Faithful Tests"},
          dynamic_directed_graph_pool_pool_test{"Directed Graph Pool-Pool Tests"}
        },
        suite{
          "Directed Embedded",
          dynamic_directed_embedded_graph_unweighted_test{"Directed Embedded Graph Unweighted Tests"},
          dynamic_directed_embedded_graph_faithful_faithful_test{"Directed Embedded Graph Faithful-Faithful Tests"},
          dynamic_directed_embedded_graph_faithful_pool_test{"Directed Embedded Graph Faithful-Pool Tests"},
          dynamic_directed_embedded_graph_pool_faithful_test{"Directed Embedded Graph Pool-Faithful Tests"},
          dynamic_directed_embedded_graph_pool_pool_test{"Directed Embedded Graph Pool-Pool Tests"}
        },
        suite{
          "Undirected",
          dynamic_undirected_graph_unweighted_test{"Undirected Graph Unweighted Tests"},
          dynamic_undirected_graph_faithful_faithful_test{"Undirected Graph Faithful-Faithful Tests"},
          dynamic_undirected_graph_faithful_faithful_shared_weight_test{"Undirected Graph Faithful-Faithful Shared Weight Tests"},
          dynamic_undirected_graph_faithful_pool_test{"Undirected Graph Faithful-Pool Tests"},
          dynamic_undirected_graph_pool_faithful_test{"Undirected Graph Pool-Faithful Tests"},
          dynamic_undirected_graph_pool_pool_test{"Undirected Graph Pool-Pool Tests"}
        },
        suite{
          "Undirected Embedded",
          dynamic_undirected_embedded_graph_unweighted_test{"Undirected Embedded Graph Unweighted Tests"},
          dynamic_undirected_embedded_graph_faithful_faithful_test{"Undirected Embedded Graph Faithful-Faithful Tests"},
          dynamic_undirected_embedded_graph_faithful_pool_test{"Undirected Embedded Graph Faithful-Pool Tests"},
          dynamic_undirected_embedded_graph_pool_faithful_test{"Undirected Embedded Graph Pool-Faithful Tests"},
          dynamic_undirected_embedded_graph_pool_pool_test{"Undirected Embedded Graph Pool-Pool Tests"}
        }
      }
    );

    runner.execute(timer_resolution{1ms});
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

