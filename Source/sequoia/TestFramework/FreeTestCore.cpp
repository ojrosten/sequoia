////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/FreeTestCore.hpp"
#include "sequoia/TestFramework/FileEditors.hpp"

#include <fstream>

namespace sequoia::testing
{
  namespace fs = std::filesystem;
  
  namespace impl
  {
    void versioned_write(const std::filesystem::path& file, std::string_view text)
    {
      if(!text.empty() || std::filesystem::exists(file))
      {
        write_to_file(file, text);
      }
    }

    void versioned_write(const std::filesystem::path& file, const failure_output& output)
    {
      for(const auto& info : output)
      {
        versioned_write(file, info.message);
      }
    }

    void serialize(const std::filesystem::path& file, std::string_view name, const failure_output& output)
    {      
      fs::create_directories(file.parent_path());
      if(std::ofstream ofile{file})
      {
        ofile << name << "\n";
        for(const auto& info : output)
        {
          ofile << info << "\n";
        }
      }
      else
      {
        throw std::runtime_error{report_failed_write(file)};
      }
    }
  }
  
  timer::timer()
    : m_Start{std::chrono::steady_clock::now()}
  {}

  [[nodiscard]]
  std::chrono::nanoseconds timer::time_elapsed() const
  {
    return std::chrono::steady_clock::now() - m_Start;
  }

  [[nodiscard]]
  std::string to_tag(test_mode mode)
  {
    switch(mode)
    {
    case test_mode::false_negative:
      return "FN";
    case test_mode::false_positive:
      return "FP";
    case test_mode::standard:
      return "";
    }

    throw std::logic_error{"Unrecognized case for test_mode"};
  }
}
