////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/Maths/Graph/DynamicGraph.hpp"

#include <filesystem>

namespace sequoia::testing
{
  using tests_dependency_graph = maths::graph<maths::directed_flavour::directed, maths::null_weight, std::filesystem::path>;

  [[nodiscard]]
  tests_dependency_graph build_dependency_graph(const std::filesystem::path& sourceRepo, const std::filesystem::path& testRepo);
}
