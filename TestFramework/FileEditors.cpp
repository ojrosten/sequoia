////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for FileEditors.hpp
 */

#include "FileEditors.hpp"
#include "Format.hpp"

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
      throw fs::filesystem_error("Unable to find file", filePath, std::error_code{});

    std::string text{};
    
    if(std::ifstream ifile{filePath})
    {
      std::string line{};
      bool inserted{};
            
      while(std::getline(ifile, line))
      {
        if(!inserted)
        {
          constexpr auto npos{std::string::npos};
          if(auto pos{line.find(familyName)}; pos != npos)
          {
            // TO DO
          }
          else if(pos = line.find("runner.execute"); pos != npos)
          {
            const auto indent_0{line.substr(0, pos)};
            const auto indent_1{std::string{indent_0}.append("  ")};
            
            append_indented(text, "runner.add_test_family(", indent_0);
            append_indented(text, std::string{"\""}.append(familyName).append("\","), indent_1);
            for(auto i{tests.begin()}; i != tests.end() - 1; ++i)
            {
              append_indented(text, std::string{*i}.append(","), indent_1);
            }

            append_indented(text, tests.back(), indent_1);
            append_indented(text, ");\n\n", indent_0);
          }
        }
          
        text.reserve(line.size() + 1);
        text.append(std::move(line)).append("\n");
      }
    }
    else
    {
      throw std::runtime_error{std::string{"Unable to open "}.append(filePath.string()).append(" for reading")};
    }

    const auto tempPath{fs::path{filePath}.concat("x")};
    if(std::ofstream ofile{tempPath})
    {
      ofile.write(text.data(), text.size());
    }
    else
    {
      throw std::runtime_error{std::string{"Unable to open "}.append(tempPath.string()).append(" for writing")};
    }

    fs::copy_file(tempPath, filePath, fs::copy_options::overwrite_existing);
    fs::remove(tempPath);
  }
}
