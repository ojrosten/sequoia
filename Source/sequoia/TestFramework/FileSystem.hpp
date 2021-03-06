////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief File paths and related utilities.
 */

#include "sequoia/Core/Meta/Concepts.hpp"

#include <filesystem>

namespace sequoia::testing
{
  const static auto working_path_v{std::filesystem::current_path().lexically_normal()};

  [[nodiscard]]
  inline std::filesystem::path working_path()
  {
    return working_path_v;
  }

  [[nodiscard]]
  std::filesystem::path project_root(int argc, char** argv, const std::filesystem::path& fallback=working_path().parent_path());

  [[nodiscard]]
  std::filesystem::path aux_files_path(std::filesystem::path projectRoot);

  [[nodiscard]]
  std::filesystem::path code_templates_path(std::filesystem::path projectRoot);

  [[nodiscard]]
  std::filesystem::path project_template_path(std::filesystem::path projectRoot);
  
  [[nodiscard]]
  std::filesystem::path recovery_path(std::filesystem::path outputDir);

  [[nodiscard]]
  std::filesystem::path tests_temporary_data_path(std::filesystem::path outputDir);

  [[nodiscard]]
  std::filesystem::path diagnostics_output_path(std::filesystem::path outputDir);

  [[nodiscard]]
  std::filesystem::path test_summaries_path(std::filesystem::path outputDir);
  

  template<class Pred>
    requires invocable<Pred, std::filesystem::path>
  void throw_if(const std::filesystem::path& p, std::string_view message, Pred pred)
  {
    if(pred(p))
    {      
      throw std::runtime_error{p.generic_string().append(" ").append(message)};
    }
  }

  [[nodiscard]]
  std::string report_file_issue(const std::filesystem::path& file, std::string_view description);
 
  [[nodiscard]]
  std::string report_failed_read(const std::filesystem::path& file);
 
  [[nodiscard]]
  std::string report_failed_write(const std::filesystem::path& file);

  void throw_unless_exists(const std::filesystem::path& p, std::string_view message="");  

  void throw_unless_directory(const std::filesystem::path& p, std::string_view message="");
  
  void throw_unless_regular_file(const std::filesystem::path& p, std::string_view message="");


  /*! \brief Used for performing a recursive search
   */

  class search_tree
  {
  public:
    search_tree(std::filesystem::path root)
      : m_Root{std::move(root)}
    {
      throw_unless_directory(m_Root, "");
    }

    [[nodiscard]]
    const std::filesystem::path& root() const noexcept
    {
      return m_Root;
    }

    [[nodiscard]]
    std::filesystem::path find(const std::filesystem::path& filename) const;

    [[nodiscard]]
    friend bool operator==(const search_tree&, const search_tree&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const search_tree&, const search_tree&) noexcept = default;
  private:
    std::filesystem::path m_Root{};
  };

  [[nodiscard]]
  std::filesystem::path rebase_from(const std::filesystem::path& filename, const std::filesystem::path& dir);
}
