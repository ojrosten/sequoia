# 3.28 is the floor for FILE_SET CXX_MODULES on a static library.  It also
# matters for the *implementation* units: under CMP0155's OLD behaviour CMake
# does not scan sources outside the module file set, so a `module M;` .cpp gets
# no module map and fails with "module not found" while the interfaces build.
cmake_minimum_required(VERSION 3.28)

include(${CMAKE_CURRENT_LIST_DIR}/../../build_system/Utilities.cmake)

sequoia_init()

set(SourceList
    Maths/Graph/GraphErrors.cpp
    Parsing/CommandLineArguments.cpp
    PlatformSpecific/Helpers.cpp
    Runtime/ShellCommands.cpp
    Streaming/Streaming.cpp
    TestFramework/Advice.cpp
    TestFramework/AllocationCheckersCore.cpp
    TestFramework/AllocationCheckersDetails.cpp
    TestFramework/Commands.cpp
    TestFramework/ConcreteTypeCheckers.cpp
    TestFramework/DependencyAnalyzer.cpp
    TestFramework/FailureInfo.cpp
    TestFramework/FileEditors.cpp
    TestFramework/FileSystemUtilities.cpp
    TestFramework/FreeTestCore.cpp
    TestFramework/IndividualTestPaths.cpp
    TestFramework/MaterialsUpdater.cpp
    TestFramework/MoveOnlyTestCore.cpp
    TestFramework/Output.cpp
    TestFramework/PerformanceTestCore.cpp
    TestFramework/ProjectCreator.cpp
    TestFramework/ProjectPaths.cpp
    TestFramework/RegularTestCore.cpp
    TestFramework/SemanticsCheckersDetails.cpp
    TestFramework/Summary.cpp
    TestFramework/TestCreator.cpp
    TestFramework/TestLogger.cpp
    TestFramework/TestRunner.cpp
    TestFramework/TestRunnerUtilities.cpp
    TextProcessing/Indent.cpp
    TextProcessing/Patterns.cpp
    TextProcessing/Substitutions.cpp)

set(ModuleList
    Algorithms/Algorithms.cppm
    Core/Concurrency/Concurrency.cppm
    Core/Concurrency/ConcurrencyModels.cppm
    Core/ContainerUtilities/ArrayUtilities.cppm
    Core/ContainerUtilities/AssignmentUtilities.cppm
    Core/ContainerUtilities/ContainerUtilities.cppm
    Core/ContainerUtilities/Iterator.cppm
    Core/DataStructures/DataStructures.cppm
    Core/DataStructures/DataStructuresTypeTraits.cppm
    Core/DataStructures/MemOrderedTuple.cppm
    Core/DataStructures/PartitionedData.cppm
    Core/DataStructures/PartitionedDataDetails.cppm
    Core/DataStructures/StaticLinearlyPartitionedSequence.cppm
    Core/DataStructures/StaticPriorityQueue.cppm
    Core/DataStructures/StaticQueue.cppm
    Core/DataStructures/StaticStack.cppm
    Core/Logic/Bitmask.cppm
    Core/Logic/Logic.cppm
    Core/Meta/Concepts.cppm
    Core/Meta/Meta.cppm
    Core/Meta/Sequences.cppm
    Core/Meta/TypeAlgorithms.cppm
    Core/Meta/TypeList.cppm
    Core/Meta/TypeTraits.cppm
    Core/Meta/Utilities.cppm
    Core/Object/Creator.cppm
    Core/Object/Factory.cppm
    Core/Object/HandlerTraits.cppm
    Core/Object/Handlers.cppm
    Core/Object/Nomenclator.cppm
    Core/Object/Object.cppm
    Core/Object/Suite.cppm
    FileSystem/FileSystem.cppm
    Maths/Algebra/Algebra.cppm
    Maths/Algebra/Ratio.cppm
    Maths/Arithmetic/Arithmetic.cppm
    Maths/Arithmetic/SaturatingArithmetic.cppm
    Maths/Geometry/Geometry.cppm
    Maths/Geometry/Spaces.cppm
    Maths/Graph/Connectivity.cppm
    Maths/Graph/DynamicGraph.cppm
    Maths/Graph/DynamicGraphTraversalDetails.cppm
    Maths/Graph/DynamicGraphTraversals.cppm
    Maths/Graph/DynamicTree.cppm
    Maths/Graph/Edge.cppm
    Maths/Graph/EdgesAndNodesUtilities.cppm
    Maths/Graph/Graph.cppm
    Maths/Graph/GraphAlgorithms.cppm
    Maths/Graph/GraphDetails.cppm
    Maths/Graph/GraphErrors.cppm
    Maths/Graph/GraphPrimitive.cppm
    Maths/Graph/GraphTraits.cppm
    Maths/Graph/GraphTraversalDetails.cppm
    Maths/Graph/GraphTraversalFunctions.cppm
    Maths/Graph/GraphTraversals.cppm
    Maths/Graph/HeterogeneousNodeStorage.cppm
    Maths/Graph/HeterogeneousStaticGraph.cppm
    Maths/Graph/NodeStorage.cppm
    Maths/Graph/StaticGraph.cppm
    Maths/Graph/StaticGraphConfig.cppm
    Maths/Graph/StaticGraphDetails.cppm
    Maths/Graph/StaticGraphTraversalDetails.cppm
    Maths/Graph/StaticGraphTraversals.cppm
    Maths/Graph/StaticNodeStorage.cppm
    Maths/Sequences/LinearSequence.cppm
    Maths/Sequences/MonotonicSequence.cppm
    Maths/Sequences/MonotonicSequenceDetails.cppm
    Maths/Sequences/Sequences.cppm
    Maths/Statistics/StatisticalAlgorithms.cppm
    Maths/Statistics/Statistics.cppm
    Parsing/CommandLineArguments.cppm
    Parsing/Parsing.cppm
    Physics/PhysicalValues.cppm
    Physics/PhysicalValuesDetails.cppm
    Physics/Physics.cppm
    PlatformSpecific/Helpers.cppm
    PlatformSpecific/PlatformDiscriminators.cppm
    PlatformSpecific/PlatformSpecific.cppm
    PlatformSpecific/Preprocessor.cppm
    Runtime/Runtime.cppm
    Runtime/ShellCommands.cppm
    Streaming/Streaming.cppm
    TestFramework/Advice.cppm
    TestFramework/AllocationCheckers.cppm
    TestFramework/AllocationCheckersCore.cppm
    TestFramework/AllocationCheckersDetails.cppm
    TestFramework/AllocationCheckersTraits.cppm
    TestFramework/AllocationTestUtilities.cppm
    TestFramework/BinaryRelationships.cppm
    TestFramework/ChronoCheckers.cppm
    TestFramework/Commands.cppm
    TestFramework/ConcreteTypeCheckers.cppm
    TestFramework/CoreInfrastructure.cppm
    TestFramework/DependencyAnalyzer.cppm
    TestFramework/FailureInfo.cppm
    TestFramework/FileEditors.cppm
    TestFramework/FileSystemUtilities.cppm
    TestFramework/FreeCheckers.cppm
    TestFramework/FreeTestCore.cppm
    TestFramework/FunctionCheckers.cppm
    TestFramework/IndividualTestPaths.cppm
    TestFramework/MaterialsUpdater.cppm
    TestFramework/MoveOnlyAllocationCheckers.cppm
    TestFramework/MoveOnlyAllocationCheckersDetails.cppm
    TestFramework/MoveOnlyAllocationTestCore.cppm
    TestFramework/MoveOnlyCheckers.cppm
    TestFramework/MoveOnlyCheckersDetails.cppm
    TestFramework/MoveOnlyTestCore.cppm
    TestFramework/Output.cppm
    TestFramework/PathCheckers.cppm
    TestFramework/PerformanceTestCore.cppm
    TestFramework/PointerCheckers.cppm
    TestFramework/ProductTypeCheckers.cppm
    TestFramework/ProjectCreator.cppm
    TestFramework/ProjectPaths.cppm
    TestFramework/RegularAllocationCheckers.cppm
    TestFramework/RegularAllocationCheckersDetails.cppm
    TestFramework/RegularAllocationTestCore.cppm
    TestFramework/RegularCheckers.cppm
    TestFramework/RegularCheckersDetails.cppm
    TestFramework/RegularTestCore.cppm
    TestFramework/SemanticsCheckersDetails.cppm
    TestFramework/SmartPointerCheckers.cppm
    TestFramework/StateTransitionUtilities.cppm
    TestFramework/StringCheckers.cppm
    TestFramework/SumTypeCheckers.cppm
    TestFramework/Summary.cppm
    TestFramework/TestCreator.cppm
    TestFramework/TestFramework.cppm
    TestFramework/TestLogger.cppm
    TestFramework/TestMode.cppm
    TestFramework/TestPreprocessorDefinitions.cppm
    TestFramework/TestRunner.cppm
    TestFramework/TestRunnerUtilities.cppm
    TextProcessing/Indent.cppm
    TextProcessing/Patterns.cppm
    TextProcessing/Substitutions.cppm
    TextProcessing/TextProcessing.cppm)

add_library(sequoia STATIC ${SourceList})

# The interface is the modules; only the two macro headers remain, and they are
# included in the global module fragment of whatever needs them.
target_sources(sequoia PUBLIC FILE_SET CXX_MODULES
                              BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}
                              FILES ${ModuleList})

sequoia_add_std_module()
target_link_libraries(sequoia PUBLIC sequoia_std)

target_include_directories(sequoia PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../)

sequoia_finalize_library(sequoia)
