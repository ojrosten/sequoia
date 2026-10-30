////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

export module sequoia.test_framework;

export import :Advice;
export import :AllocationCheckers;
export import :AllocationCheckersCore;
export import :AllocationCheckersDetails;
export import :AllocationCheckersTraits;
export import :AllocationTestUtilities;
export import :BinaryRelationships;
export import :BuildArtefacts;
export import :CMakeCache;
export import :CoarseSleeps;
export import :Commands;
export import :ConcreteTypeCheckers;
export import :CoreInfrastructure;
export import :DependencyAnalyzer;
export import :DumpComparison;
export import :FailureInfo;
export import :FailureReporting;
export import :FileEditors;
export import :FileSystemUtilities;
export import :FloatingPointDurations;
export import :FreeCheckers;
export import :FreeTestCore;
export import :FunctionCheckers;
export import :IndividualTestPaths;
export import :InstabilityAnalysis;
export import :InstabilityProjection;
export import :MaterialsUpdater;
export import :MoveOnlyAllocationCheckers;
export import :MoveOnlyAllocationCheckersDetails;
export import :MoveOnlyAllocationTestCore;
export import :MoveOnlyCheckers;
export import :MoveOnlyCheckersDetails;
export import :MoveOnlyTestCore;
export import :Output;
export import :PathCheckers;
export import :PerformanceSummary;
export import :PerformanceTestCore;
export import :PerformanceTestTraits;
export import :PointerCheckers;
export import :ProductTypeCheckers;
export import :ProjectCreator;
export import :ProjectPaths;
export import :RegularAllocationCheckers;
export import :RegularAllocationCheckersDetails;
export import :RegularAllocationTestCore;
export import :RegularCheckers;
export import :RegularCheckersDetails;
export import :RegularTestCore;
export import :ReturnCodes;
export import :SemanticsCheckersDetails;
export import :SeparatorAfterCheck;
export import :SmartPointerCheckers;
export import :StateTransitionUtilities;
export import :StringCheckers;
export import :SumTypeCheckers;
export import :Summary;
export import :TestCreator;
export import :TestExecutionRecord;
export import :TestLogger;
export import :TestMode;
export import :TestPreprocessorDefinitions;
export import :TestRunner;
export import :TimingUtilities;
export import :VersionedOutput;
export import :TestRunnerUtilities;
