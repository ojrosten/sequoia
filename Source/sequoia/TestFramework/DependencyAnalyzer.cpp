////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/DependencyAnalyzer.hpp"

#include "sequoia/Maths/Graph/DynamicGraph.hpp"
#include "sequoia/Streaming/Streaming.hpp"

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    enum class target_repository {yes, no};

    struct file_info
    {
      file_info(const target_repository targetRepo, const fs::path& f, const fs::file_time_type& timeStamp)
        : target{targetRepo}
        , file{f}
        , stale{fs::last_write_time(file) > timeStamp}
      {}

      target_repository target{target_repository::no};
      fs::path file;
      bool stale{true};
    };

    using tests_dependency_graph = maths::graph<maths::directed_flavour::directed, maths::null_weight, file_info>;

    void add_files(tests_dependency_graph& g, const std::filesystem::path& repo, const target_repository targetRepo, const fs::file_time_type& timeStamp)
    {
      constexpr std::array<std::string_view, 5> exts{".h", ".hpp", ".cpp", ".cc", ".cxx"};

      for(const auto& entry : fs::recursive_directory_iterator(repo))
      {
        const auto file{entry.path()};
        if(auto found{std::find(exts.begin(), exts.end(), file.extension().string())}; found != exts.end())
        {
          g.add_node(targetRepo, file, timeStamp);
        }
      }
    }

    void build_dependencies(tests_dependency_graph& g,
                            const std::filesystem::path& sourceRepo,
                            const std::filesystem::path& testRepo)
    {
      using size_type = tests_dependency_graph::size_type;

      for(auto i{g.cbegin_node_weights()}; i != g.cend_node_weights(); ++i)
      {
        const auto nodePos{static_cast<size_type>(distance(g.cbegin_node_weights(), i))};

        const auto text{read_to_string(i->file)};
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
                const auto& file{i->file};
                if(includedFile.parent_path().empty())
                {
                  includedFile = file.parent_path() / includedFile;
                }

                auto includeMatcher{
                  [&includedFile,&sourceRepo,&testRepo](const auto& weight) {
                    if(includedFile.is_absolute()) return weight.file == includedFile;

                    return (weight.file == (sourceRepo / includedFile).lexically_normal()) || (weight.file == (testRepo / includedFile).lexically_normal());
                  }
                };

                if(auto incIter{std::find_if(g.cbegin_node_weights(), g.cend_node_weights(), includeMatcher)};
                  incIter != g.cend_node_weights())
                {
                  if((file.stem() == includedFile.stem()) && i->stale)
                  {
                    g.mutate_node_weight(incIter, [](auto& w) { w.stale = true; });
                  }

                  const auto includeNodePos{static_cast<size_type>(distance(g.cbegin_node_weights(), incIter))};
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
  std::optional<std::vector<std::string>> tests_to_run(const std::filesystem::path& sourceRepo,
                                                       const std::filesystem::path& testRepo,
                                                       const std::optional<std::filesystem::file_time_type>& timeStamp)
  {
    if(!timeStamp) return std::nullopt;

    std::optional<std::vector<std::string>> testsToRun{};

    tests_dependency_graph g{};
    add_files(g, sourceRepo, target_repository::no, timeStamp.value());
    add_files(g, testRepo, target_repository::yes, timeStamp.value());
    build_dependencies(g, sourceRepo, testRepo);

    return testsToRun;
  }
}
