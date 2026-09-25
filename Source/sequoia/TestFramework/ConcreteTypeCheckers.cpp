////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/PathCheckers.hpp"

namespace sequoia::testing
{
  namespace
  {
    using path_tester_t = value_tester<std::filesystem::path>;
  }
  
  [[nodiscard]]
  std::string path_check_preamble(std::string_view prefix, const std::filesystem::path& path, const std::filesystem::path& prediction)
  {
    return append_lines(prefix, path.generic_string(), "vs", prediction.generic_string()).append("\n");
  }

  const path_tester_t::basic_file_checker_type path_tester_t::basic_file_checker{".*"};

  const general_equivalence_check_t<path_tester_t::basic_file_checker_type> path_tester_t::basic_path_equivalence{basic_file_checker};

  const general_weak_equivalence_check_t<path_tester_t::basic_file_checker_type> path_tester_t::basic_path_weak_equivalence{basic_file_checker};
}
