cmake_minimum_required(VERSION 3.20)

project(SequoiaTestChamber DESCRIPTION "The Sequoia Library plus a test chamber"
                           LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../build_system/Utilities.cmake)

sequoia_init()

add_executable(TestChamber TestChamberMain.cpp)

set(TestDir ${CMAKE_CURRENT_LIST_DIR}/../Tests)

target_sources(TestChamber PRIVATE 
               
               ${TestDir}/Maths/Geometry/AffineCoordinatesTest.cpp
               ${TestDir}/Maths/Geometry/AffineCoordinatesTestingDiagnostics.cpp
               ${TestDir}/Maths/Geometry/AngleTest.cpp
               ${TestDir}/Maths/Geometry/AngleTestingDiagnostics.cpp
               ${TestDir}/Maths/Geometry/SpacesMetaFreeTest.cpp
               ${TestDir}/Maths/Geometry/VectorCoordinatesTest.cpp
               ${TestDir}/Maths/Geometry/VectorCoordinatesTestingDiagnostics.cpp
               ${TestDir}/Physics/AbsoluteQuantityTest.cpp
               ${TestDir}/Physics/AffineQuantityTest.cpp
               ${TestDir}/Physics/ConvexQuantityTest.cpp
               ${TestDir}/Physics/MixedQuantityTest.cpp
               ${TestDir}/Physics/QuantityMetaFreeTest.cpp               
               ${TestDir}/Physics/QuantityTestingDiagnostics.cpp
               ${TestDir}/Physics/UnsafeAbsoluteQuantityTest.cpp
               ${TestDir}/Physics/VectorQuantityTest.cpp)

sequoia_finalize_self(TestChamber ${TestDir}/Experimental Experimental)
