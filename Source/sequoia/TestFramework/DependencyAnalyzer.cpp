////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/DependencyAnalyzer.hpp"

#include "sequoia/Streaming/Streaming.hpp"

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    [[nodiscard]]
    tests_dependency_graph::size_type add_file(tests_dependency_graph& g, const fs::path& file, const fs::path& repo)
    {
      if(auto inGraph{std::find_if(g.cbegin_node_weights(), g.cend_node_weights(), [&file](const auto& w) { return w == file; })};
        inGraph == g.cend_node_weights())
      {
        return g.add_node(file.is_absolute() ? fs::relative(file, repo) : file);
      }
      else
      {
        using size_type = tests_dependency_graph::size_type;
        return static_cast<size_type>(distance(g.cbegin_node_weights(), inGraph));
      }
    }

    void build_dependency_graph(tests_dependency_graph& g, const std::filesystem::path& repo)
    {
      constexpr std::array<std::string_view, 5> exts{".h", ".hpp", ".cpp", ".cc", ".cxx"};

      for(const auto& entry : fs::recursive_directory_iterator(repo))
      {
        const auto file{entry.path()};
        if(auto found{std::find(exts.begin(), exts.end(), file.extension().string())}; found != exts.end())
        {
          const auto nodePos{add_file(g, file, repo)};

          const auto text{read_to_string(entry.path())};
          std::string::size_type pos{};
          constexpr auto npos{std::string::npos};
          while(pos != npos)
          {
            constexpr std::string_view include{"#include"};
            pos = text.find(include, pos);
            if(pos != npos)
            {
              pos += include.size();
              const auto start{text.find_first_of("\"<", pos)};
              const auto end{text.find_first_of("\">", start + 1)};
              if((start != npos) && (end != npos) && (((text[start] == '\"') && (text[end] == '\"')) || ((text[start] == '<') && (text[end] == '>'))))
              {
                fs::path includedFile{text.substr(start + 1, end - 1 - start)};
                if(includedFile.has_extension())
                {
                  if(includedFile.parent_path().empty())
                  {
                    includedFile = file.parent_path() / includedFile;
                  }

                  const auto includeNodePos{add_file(g, includedFile, repo)};
                  g.join(nodePos, includeNodePos);
                }
              }
            }
          }
        }
      }
    }
  }

  [[nodiscard]]
  tests_dependency_graph build_dependency_graph(const std::filesystem::path& sourceRepo, const std::filesystem::path& testRepo)
  {
    tests_dependency_graph g{};
    build_dependency_graph(g, sourceRepo);
    build_dependency_graph(g, testRepo);

    return g;
  }
}
