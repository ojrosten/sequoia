cmake_minimum_required(VERSION 3.20)

project(SequoiaTestChamber DESCRIPTION "The Sequoia Library plus a test chamber"
                           LANGUAGES CXX)

include(../build_system/Utilities.cmake)

sequoia_init()

add_executable(TestChamber TestChamberMain.cpp)

set(TestDir ../Tests)

target_sources(TestChamber PRIVATE 
               ${TestDir}/Experimental/ExperimentalTest.cpp
               ${TestDir}/Maths/Graph/Directed/DynamicDirectedGraphFaithfulFaithfulTest.cpp
               ${TestDir}/Maths/Graph/Directed/DynamicDirectedGraphFaithfulPoolTest.cpp
               ${TestDir}/Maths/Graph/Directed/DynamicDirectedGraphPoolFaithfulTest.cpp
               ${TestDir}/Maths/Graph/Directed/DynamicDirectedGraphPoolPoolTest.cpp
               ${TestDir}/Maths/Graph/Directed/DynamicDirectedGraphUnweightedTest.cpp
               ${TestDir}/Maths/Graph/DirectedEmbedded/DynamicDirectedEmbeddedGraphFaithfulFaithfulTest.cpp
               ${TestDir}/Maths/Graph/DirectedEmbedded/DynamicDirectedEmbeddedGraphFaithfulPoolTest.cpp
               ${TestDir}/Maths/Graph/DirectedEmbedded/DynamicDirectedEmbeddedGraphPoolFaithfulTest.cpp
               ${TestDir}/Maths/Graph/DirectedEmbedded/DynamicDirectedEmbeddedGraphPoolPoolTest.cpp
               ${TestDir}/Maths/Graph/DirectedEmbedded/DynamicDirectedEmbeddedGraphUnweightedTest.cpp
               ${TestDir}/Maths/Graph/Undirected/DynamicUndirectedGraphFaithfulFaithfulTest.cpp
               ${TestDir}/Maths/Graph/Undirected/DynamicUndirectedGraphFaithfulFaithfulSharedWeightTest.cpp
               ${TestDir}/Maths/Graph/Undirected/DynamicUndirectedGraphFaithfulPoolTest.cpp
               ${TestDir}/Maths/Graph/Undirected/DynamicUndirectedGraphPoolFaithfulTest.cpp
               ${TestDir}/Maths/Graph/Undirected/DynamicUndirectedGraphPoolPoolTest.cpp
               ${TestDir}/Maths/Graph/Undirected/DynamicUndirectedGraphUnweightedTest.cpp
               ${TestDir}/Maths/Graph/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphFaithfulFaithfulTest.cpp
               ${TestDir}/Maths/Graph/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphFaithfulPoolTest.cpp
               ${TestDir}/Maths/Graph/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphPoolFaithfulTest.cpp
               ${TestDir}/Maths/Graph/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphPoolPoolTest.cpp
               ${TestDir}/Maths/Graph/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphUnweightedTest.cpp
)

sequoia_finalize_self(TestChamber ${TestDir}/Experimental)
