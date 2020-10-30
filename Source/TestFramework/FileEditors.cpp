////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for FileEditors.hpp
 */

#include "FileEditors.hpp"
#include "FileSystem.hpp"
#include "Output.hpp"

#include <fstream>

namespace sequoia::testing
{
  void add_include(const std::filesystem::path& filePath, std::string_view include)
  {
    if(std::ofstream ofile{filePath, std::ios_base::app})      
    {
      ofile << "#include \"" << include << "\"\n";
    }    
    else
    {
      throw std::runtime_error(std::string{"Unable to open "}.append(filePath.string()).append(" for writing"));
    }
  }

  void add_to_family(const std::filesystem::path& filePath, std::string_view familyName, const std::vector<std::string>& tests)
  {
    namespace fs = std::filesystem;

    if(!fs::exists(filePath))
      throw fs::filesystem_error{"Unable to find file", filePath, std::error_code{}};

    if(tests.empty())
      throw std::logic_error{std::string{"No tests specified to be added to the test family \""}.append(familyName).append("\"")};

    std::string text{};
    
    if(std::ifstream ifile{filePath})
    {
      std::string line{};
      bool inserted{};
            
      while(std::getline(ifile, line))
      {
        bool lineAdded{};
        if(!inserted)
        {
          constexpr auto npos{std::string::npos};  

          if(auto pos{line.find(familyName)}; pos != npos)
          {
            const indentation indent{line.substr(0, pos)};
            
            text.reserve(line.size() + 1);
            text.append(std::move(line));

            for(const auto& t : tests)
            {
              append_indented(text, std::string{t}.append(","), indent);
            }
            text.append("\n");

            lineAdded = true;
            inserted = true;
          }
          else if(pos = line.find("runner.execute"); pos != npos)
          {            
            const indentation indent_0{line.substr(0, pos)};
            const indentation indent_1{std::string{indent_0}.append("  ")};

            append_indented(text, "runner.add_test_family(", indent_0);
            append_indented(text, std::string{"\""}.append(familyName).append("\","), indent_1);
            for(auto i{tests.cbegin()}; i != tests.cend() - 1; ++i)
            {
              append_indented(text, std::string{*i}.append(","), indent_1);
            }

            append_indented(text, tests.back(), indent_1);
            append_indented(text, ");\n\n", indent_0);

            inserted = true;
          }
        }

        if(!lineAdded)
        {
          text.reserve(line.size() + 1);
          text.append(std::move(line)).append("\n");
        }
      }

      if(!inserted)
        throw std::runtime_error{"Unable to find appropriate place to add test family"};
    }
    else
    {
      throw std::runtime_error{report_failed_read(filePath)};
    }

    const auto tempPath{fs::path{filePath}.concat("x")};
    if(std::ofstream ofile{tempPath})
    {
      ofile.write(text.data(), text.size());
    }
    else
    {
      throw std::runtime_error{report_failed_write(tempPath)};
    }

    fs::copy_file(tempPath, filePath, fs::copy_options::overwrite_existing);
    fs::remove(tempPath);
  }
}
