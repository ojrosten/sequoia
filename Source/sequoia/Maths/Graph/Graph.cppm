////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include "sequoia/PlatformSpecific/Macros.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <execution>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <numeric>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

export module sequoia.maths.graph;

export import :Connectivity;
export import :DynamicGraph;
export import :DynamicGraphTraversalDetails;
export import :DynamicGraphTraversals;
export import :DynamicTree;
export import :Edge;
export import :EdgesAndNodesUtilities;
export import :GraphAlgorithms;
export import :GraphDetails;
export import :GraphErrors;
export import :GraphPrimitive;
export import :GraphTraits;
export import :GraphTraversalDetails;
export import :GraphTraversalFunctions;
export import :GraphTraversals;
export import :HeterogeneousNodeStorage;
export import :HeterogeneousStaticGraph;
export import :NodeStorage;
export import :StaticGraph;
export import :StaticGraphConfig;
export import :StaticGraphDetails;
export import :StaticGraphTraversalDetails;
export import :StaticGraphTraversals;
export import :StaticNodeStorage;

import :Connectivity;
import :DynamicGraph;
import :Edge;
import :EdgesAndNodesUtilities;
import :GraphDetails;
import :GraphErrors;
import :GraphPrimitive;
import :GraphTraits;
import :HeterogeneousNodeStorage;
import :HeterogeneousStaticGraph;
import :NodeStorage;
import :StaticGraph;
import :StaticGraphConfig;
import :StaticGraphDetails;
import :StaticNodeStorage;
export import sequoia.algorithms;
export import sequoia.core.container_utilities;
export import sequoia.core.data_structures;
export import sequoia.core.meta;
export import sequoia.core.object;
export import sequoia.maths.sequences;
export import sequoia.platform_specific;

/** \file
    \brief Includes for dynamic, static and heterogeneous graphs.
 */
