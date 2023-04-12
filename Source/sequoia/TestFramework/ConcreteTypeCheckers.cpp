////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file 
    \brief Definitions for ConcreteTypeCheckers.hpp
 */

#include "sequoia/TestFramework/ConcreteTypeCheckers.hpp"

namespace sequoia::testing
{
  namespace
  {
    using basic_file_checker_t = general_file_checker<string_based_file_comparer>;
  }
  
  [[nodiscard]]
  std::string path_check_preamble(std::string_view prefix, const std::filesystem::path& path, const std::filesystem::path& prediction)
  {
    return append_lines(prefix, path.generic_string(), "vs", prediction.generic_string()).append("\n");
  }

  const basic_file_checker_t value_tester<std::filesystem::path>::basic_file_checker{{".*"}};

  const general_equivalence_check_t<basic_file_checker_t> value_tester<std::filesystem::path>::basic_path_equivalence{basic_file_checker};

  const general_weak_equivalence_check_t<basic_file_checker_t> value_tester<std::filesystem::path>::basic_path_weak_equivalence{basic_file_checker};
}
