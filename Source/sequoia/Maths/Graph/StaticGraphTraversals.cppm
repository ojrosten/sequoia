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
#include <condition_variable>
#include <execution>
#include <functional>
#include <future>
#include <iterator>
#include <memory>
#include <mutex>
#include <queue>
#include <span>
#include <stack>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

export module sequoia.maths.graph:StaticGraphTraversals;

import :DynamicGraphTraversalDetails;
import :Edge;
import :EdgesAndNodesUtilities;
import :GraphDetails;
import :GraphTraits;
import :GraphTraversalDetails;
import :GraphTraversalFunctions;
import :StaticGraphTraversalDetails;
export import sequoia.algorithms;
export import sequoia.core.concurrency;
export import sequoia.core.container_utilities;
export import sequoia.core.data_structures;
export import sequoia.core.meta;
export import sequoia.core.object;
export import sequoia.platform_specific;

/** \file
    \brief Headers for traversals of static graphs.
 */
