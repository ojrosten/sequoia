////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include "sequoia/PlatformSpecific/Macros.hpp"

export module sequoia.maths.graph:GraphTraversals;

import std;

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
    \brief Headers for traversals of both static and dynamic graphs.

 */
