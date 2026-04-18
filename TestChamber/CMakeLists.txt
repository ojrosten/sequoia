cmake_minimum_required(VERSION 3.20)

project(SequoiaTestChamber DESCRIPTION "The Sequoia Library plus a test chamber"
                           LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../build_system/Utilities.cmake)

sequoia_init()

add_executable(TestChamber TestChamberMain.cpp)

set(TestDir ${CMAKE_CURRENT_LIST_DIR}/../Tests)

target_sources(TestChamber PRIVATE            
               ${TestDir}/Maths/Algebra/RatioFreeTest.cpp
               ${TestDir}/Maths/Geometry/AbsoluteCoordinatesTest.cpp
               ${TestDir}/Maths/Geometry/AbsoluteCoordinatesTestingDiagnostics.cpp
               ${TestDir}/Maths/Geometry/AbsoluteLogarithmicCoordinatesTest.cpp
               ${TestDir}/Maths/Geometry/AffineCoordinatesTest.cpp
               ${TestDir}/Maths/Geometry/AffineCoordinatesTestingDiagnostics.cpp
               ${TestDir}/Maths/Geometry/FreeModuleCoordinatesTest.cpp
               ${TestDir}/Maths/Geometry/SpacesMetaFreeTest.cpp
               ${TestDir}/Maths/Geometry/VectorCoordinatesTest.cpp
               ${TestDir}/Maths/Geometry/VectorCoordinatesTestingDiagnostics.cpp
               ${TestDir}/Maths/Geometry/VectorPolarCoordinatesTest.cpp
               ${TestDir}/Physics/AbsolutePhysicalValueTest.cpp
               ${TestDir}/Physics/AffinePhysicalValueTest.cpp
               ${TestDir}/Physics/ConvexPhysicalValueTest.cpp
               ${TestDir}/Physics/IntegralPhysicalValueTest.cpp
               ${TestDir}/Physics/MixedPhysicalValueTest.cpp
               ${TestDir}/Physics/PhysicalValueMetaFreeTest.cpp               
               ${TestDir}/Physics/PhysicalValueTestingDiagnostics.cpp
               ${TestDir}/Physics/UnsafeAbsolutePhysicalValueTest.cpp
               ${TestDir}/Physics/VectorPhysicalValueTest.cpp)

sequoia_finalize_self(TestChamber ${TestDir}/Experimental Experimental)
