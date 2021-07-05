////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/DependencyAnalyzer.hpp"

#include "sequoia/Maths/Graph/DynamicGraph.hpp"
#include "sequoia/Maths/Graph/GraphTraversalFunctions.hpp"
#include "sequoia/Streaming/Streaming.hpp"

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    struct file_info
    {
      file_info(fs::path f, const fs::file_time_type& timeStamp)
        : file{std::move(f)}
        , stale{fs::last_write_time(file) > timeStamp}
      {}

      fs::path file;
      bool stale{true};
    };

    [[nodiscard]]
    bool in_repo(const fs::path& file, const fs::path& repo)
    {
      if(std::distance(repo.begin(), repo.end()) >= std::distance(file.begin(), file.end())) return false;

      auto fIter{file.begin()}, rIter{repo.begin()};
      while(rIter != repo.end())
      {
        if(*fIter++ != *rIter++)
          return false;
      }

      return true;
    }

    [[nodiscard]]
    bool is_cpp(const std::filesystem::path& file)
    {
      const auto ext{file.extension()};
      return (ext == ".cpp") || (ext == ".cc") || (ext == ".cxx");
    }

    [[nodiscard]]
    bool is_header(const std::filesystem::path& file)
    {
      const auto ext{file.extension()};
      return (ext == ".hpp") || (ext == ".h") || (ext == ".hxx");
    }

    using tests_dependency_graph = maths::graph<maths::directed_flavour::directed, maths::null_weight, file_info>;
    using node_iterator = tests_dependency_graph::const_iterator;

    void add_files(tests_dependency_graph& g, const std::filesystem::path& repo, const fs::file_time_type& timeStamp)
    {
      for(const auto& entry : fs::recursive_directory_iterator(repo))
      {
        const auto file{entry.path()};
        if(is_cpp(file) || is_header(file))
        {
          g.add_node(file, timeStamp);
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

                    return (weight.file == sourceRepo / includedFile) || (weight.file == testRepo / includedFile);
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

    void consider_materials(tests_dependency_graph& g,
                            node_iterator i,
                            const fs::path& relFilePath,
                            const fs::path& materialsRepo,
                            const std::filesystem::file_time_type timeStamp)
    {
      const auto materials{materialsRepo / fs::path{relFilePath}.replace_extension("")};
      if(fs::exists(materials))
      {
        for(const auto& entry : fs::recursive_directory_iterator(materials))
        {
          if(fs::last_write_time(entry) > timeStamp)
          {
            g.mutate_node_weight(i, [](auto& w) { w.stale = true; });
            break;
          }
        }
      }
    }
  }

  [[nodiscard]]
  std::optional<std::vector<std::string>> tests_to_run(const std::filesystem::path& sourceRepo,
                                                       const std::filesystem::path& testRepo,
                                                       const std::filesystem::path& materialsRepo,
                                                       const std::optional<std::filesystem::file_time_type>& timeStamp)
  {
    using namespace maths;

    if(!timeStamp) return std::nullopt;

    std::optional<std::vector<std::string>> testsToRun{{}};

    tests_dependency_graph g{};
    add_files(g, sourceRepo, timeStamp.value());
    add_files(g, testRepo, timeStamp.value());
    build_dependencies(g, sourceRepo, testRepo);

    auto nodesLate{
      [&g](const std::size_t node) {
        for(auto i{g.cbegin_edges(node)}; i != g.cend_edges(node); ++i)
        {
          if((g.cbegin_node_weights() + i->target_node())->stale)
          {
            g.mutate_node_weight(g.cbegin_node_weights() + node, [](auto& w) { w.stale = true; });
            break;
          }
        }
      }
    };

    depth_first_search(g, find_disconnected_t{0}, null_func_obj{}, nodesLate);

    for(auto i{g.cbegin_node_weights()}; i != g.cend_node_weights(); ++i)
    {
      if(const auto& weight{*i}; is_cpp(weight.file) && in_repo(weight.file, testRepo))
      {
        const auto relPath{fs::relative(weight.file, testRepo)};

        if(!weight.stale) consider_materials(g, i, relPath, materialsRepo, timeStamp.value());

        if(weight.stale) testsToRun->push_back(relPath.generic_string());
      }
    }

    return testsToRun;
  }
}
