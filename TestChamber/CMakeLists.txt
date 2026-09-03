# 3.28: consumers of a module library must themselves be scanned for imports,
# and under CMP0155's OLD behaviour they are not - every import then fails with
# "module not found" even though the library built.
cmake_minimum_required(VERSION 3.28)

project(SequoiaTestChamber DESCRIPTION "The Sequoia Library plus a test chamber"
                           LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../build_system/Utilities.cmake)

sequoia_init()

add_executable(TestChamber TestChamberMain.cpp)

set(TestDir ${CMAKE_CURRENT_LIST_DIR}/../Tests)

target_sources(TestChamber PRIVATE            
               ${TestDir}/Maths/Algebra/RatioFreeTest.cpp
               ${TestDir}/Maths/Arithmetic/SaturatingArithmeticFreeTest.cpp
               ${TestDir}/Maths/Geometry/AbsoluteCoordinatesTest.cpp
               ${TestDir}/Maths/Geometry/AbsoluteCoordinatesTestingDiagnostics.cpp
               ${TestDir}/Maths/Geometry/AbsoluteLogarithmicCoordinatesTest.cpp
               ${TestDir}/Maths/Geometry/AffineCoordinatesTest.cpp
               ${TestDir}/Maths/Geometry/AffineCoordinatesTestingDiagnostics.cpp
               ${TestDir}/Maths/Geometry/BoundsFreeTest.cpp
               ${TestDir}/Maths/Geometry/ComplexVectorCoordinatesTest.cpp
               ${TestDir}/Maths/Geometry/FreeModuleCoordinatesTest.cpp
               ${TestDir}/Maths/Geometry/M_AffineCoordinatesTest.cpp
               ${TestDir}/Maths/Geometry/NumericRingsMetaFreeTest.cpp
               ${TestDir}/Maths/Geometry/PartialMTorsorCoordinatesTest.cpp
               ${TestDir}/Maths/Geometry/SpacesMetaFreeTest.cpp
               ${TestDir}/Maths/Geometry/ValidatorsFreeTest.cpp
               ${TestDir}/Maths/Geometry/VectorCoordinatesTest.cpp
               ${TestDir}/Maths/Geometry/VectorCoordinatesTestingDiagnostics.cpp
               ${TestDir}/Maths/Geometry/VectorPolarCoordinatesTest.cpp
               ${TestDir}/Physics/AbsolutePhysicalValueCompositionsTest.cpp
               ${TestDir}/Physics/AbsolutePhysicalValueTest.cpp
               ${TestDir}/Physics/AbsolutePhysicalValueCompositionsTest.cpp
               ${TestDir}/Physics/AffinePhysicalValueTest.cpp
               ${TestDir}/Physics/ConvexPhysicalValueTest.cpp
               ${TestDir}/Physics/IntegralPhysicalValueTest.cpp
               ${TestDir}/Physics/MixedPhysicalValueTest.cpp
               ${TestDir}/Physics/PhysicalValueConversionsFreeTest.cpp
               ${TestDir}/Physics/PhysicalValueMetaFreeTest.cpp
               ${TestDir}/Physics/PhysicalValueConversionsFreeTest.cpp
               ${TestDir}/Physics/PhysicalValueTestingDiagnostics.cpp
               ${TestDir}/Physics/SpaceOrderingMetaFreeTest.cpp
               ${TestDir}/Physics/UnsafeAbsolutePhysicalValueCompositionsTest.cpp
               ${TestDir}/Physics/UnsafeAbsolutePhysicalValueTest.cpp
               ${TestDir}/Physics/UnsafeAbsolutePhysicalValueCompositionsTest.cpp
               ${TestDir}/Physics/VectorNonlinearRepresentationsTest.cpp
               ${TestDir}/Physics/VectorPhysicalValueCompositionsTest.cpp
               ${TestDir}/Physics/VectorPhysicalValueTest.cpp)

sequoia_finalize_self(TestChamber ${TestDir}/Experimental Experimental)
