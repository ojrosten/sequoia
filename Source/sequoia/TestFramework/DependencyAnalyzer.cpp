////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for DependencyAnalyzer.hpp
 */

#include "sequoia/TestFramework/DependencyAnalyzer.hpp"
#include "sequoia/TestFramework/FileSystemUtilities.hpp"

#include "sequoia/Maths/Graph/DynamicGraph.hpp"
#include "sequoia/Maths/Graph/GraphTraversalFunctions.hpp"
#include "sequoia/Streaming/Streaming.hpp"

#include <fstream>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    [[nodiscard]]
    bool is_stale(fs::path file, const fs::file_time_type& timeStamp, const std::optional<fs::file_time_type>& exeTimeStamp)
    {
      const auto lwt{fs::last_write_time(file)};
      if(exeTimeStamp.has_value() && (lwt >= exeTimeStamp.value()))
        throw std::runtime_error{"Exectuable is out of date; please build it!\n"};

      return lwt > timeStamp;
    }

    struct file_info
    {
      file_info(fs::path f, const fs::file_time_type& timeStamp, const std::optional<fs::file_time_type>& exeTimeStamp)
        : file{std::move(f)}
        , stale{is_stale(file, timeStamp, exeTimeStamp)}
      {}

      file_info(fs::path f)
        : file{std::move(f)}
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

    [[nodiscard]]
    std::string from_stream(std::istream& istr, char delimiter)
    {
      constexpr auto eof{std::ifstream::traits_type::eof()};
      using int_type = std::ifstream::int_type;

      std::string str{};

      int_type c{};
      while((c = istr.get()) != eof)
      {
        if(c == delimiter) break;

        str.push_back(static_cast<char>(c));
      }

      return str;
    }

    [[nodiscard]]
    std::vector<fs::path> get_includes(const fs::path& file, std::string_view cutoff)
    {
      std::vector<fs::path> includes{};

      if(std::ifstream ifile{file})
      {
        constexpr auto eof{std::ifstream::traits_type::eof()};
        using int_type = std::ifstream::int_type;

        int_type c{};
        while((c = ifile.get()) != eof)
        {
          if(c == '/')
          {
            if(ifile.peek() == '/')
            {
              ifile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
            else if(ifile.peek() == '*')
            {
              ifile.get();
              while(ifile)
              {
                ifile.ignore(std::numeric_limits<std::streamsize>::max(), '*');
                if(ifile.peek() == '/')
                {
                  ifile.get();
                  break;
                }
              }
            }
          }
          else if(c == '#')
          {
            const auto followsHash{from_stream(ifile, ' ')};
            if(followsHash == "include")
            {
              int_type ch{};
              while(std::isspace(ch = ifile.get())) {};

              if(ifile)
              {
                auto includedFile{
                  [&ifile, ch]() -> fs::path {
                    if(ch == '\"')
                    {
                      return from_stream(ifile, '\"');
                    }
                    else if(ch == '<')
                    {
                      return from_stream(ifile, '>');
                    }

                    return "";
                  }()
                };

                if(includedFile.has_extension())
                {
                  if(includedFile.parent_path().empty())
                  {
                    // Maybe check if this file actually exists... if path is absolute
                    includedFile = file.parent_path() / includedFile;
                  }

                  includes.push_back(includedFile);
                }
              }
            }

          }
          else if(!cutoff.empty() && (c == cutoff.front()))
          {
            ifile.unget();
            const auto pattern{from_stream(ifile, '\n')};
            if(const auto pos{pattern.find(cutoff)}; pos != std::string::npos)
            {
              break;
            }
          }
        }
      }

      return includes;
    }

    using tests_dependency_graph = maths::graph<maths::directed_flavour::directed, maths::null_weight, file_info>;
    using node_iterator = tests_dependency_graph::const_iterator;

    void add_files(tests_dependency_graph& g, const std::filesystem::path& repo, const fs::file_time_type& timeStamp, const std::optional<fs::file_time_type>& exeTimeStamp)
    {
      for(const auto& entry : fs::recursive_directory_iterator(repo))
      {
        const auto file{entry.path()};
        if(is_cpp(file) || is_header(file))
        {
          g.add_node(file, timeStamp, exeTimeStamp);
        }
      }
    }

    /// pre-condition: the nodes of g have been sorted by file path
    /// Return value: external dependencies
    [[nodiscard]]
    std::set<std::string> build_dependencies(tests_dependency_graph& g,
                                             const std::filesystem::path& sourceRepo,
                                             const std::filesystem::path& testRepo,
                                             std::string_view cutoff)
    {
      using size_type = tests_dependency_graph::size_type;
      std::set<std::string> externalDependencies{};

      for(auto i{g.cbegin_node_weights()}; i != g.cend_node_weights(); ++i)
      {
        const auto nodePos{static_cast<size_type>(distance(g.cbegin_node_weights(), i))};
        const auto& file{i->file};

        for(const auto& includedFile : get_includes(file, cutoff))
        {
          auto comparer{
            [](const file_info& weight, const file_info& incFile) {
              return weight.file.filename() < incFile.file.filename();
            }
          };

          if(auto [b, e] {std::equal_range(g.cbegin_node_weights(), g.cend_node_weights(), includedFile, comparer)}; b != e)
          {
            auto found{
              [&includedFile,&sourceRepo,&testRepo,&file](auto b, auto e) {
                while(b != e)
                {
                  if(includedFile.is_absolute())
                  {
                    if(b->file == includedFile) return b;
                    else                        continue;
                  }

                  if((b->file == sourceRepo / includedFile) || (b->file == testRepo / includedFile))
                    return b;

                  if(const auto trial{file.parent_path() / includedFile}; fs::exists(trial) && (b->file == fs::canonical(trial)))
                    return b;

                  ++b;
                }

                return b;
              }(b, e)
            };

            if(found != e)
            {
              if((file.stem() == includedFile.stem()) && i->stale)
              {
                g.mutate_node_weight(found, [](auto& w) { w.stale = true; });
              }

              const auto includeNodePos{static_cast<size_type>(distance(g.cbegin_node_weights(), found))};
              g.join(nodePos, includeNodePos);

              if(is_cpp(file))
              {
                // Furnish the associated hpp with the same dependencies,
                // as these are what ultimately determine whether or not
                // the test cpp is considered stale. Sorting of g ensures
                // that headers are directly after sources
                if(auto next{std::next(i)}; next != g.cend_node_weights())
                {
                  if(next->file.stem() == file.stem())
                  {
                    const auto nextPos{static_cast<size_type>(distance(g.cbegin_node_weights(), next))};
                    g.join(nextPos, includeNodePos);
                  }
                }
              }
            }
            else
            {
              externalDependencies.insert(includedFile.generic_string());
            }
          }
        }
      }

      return externalDependencies;
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

    [[nodiscard]]
    std::optional<fs::file_time_type> get_stamp(const fs::path& file)
    {
      if(fs::exists(file)) return fs::last_write_time(file);

      return std::nullopt;
    }

    [[nodiscard]]
    std::vector<fs::path> read_tests(const fs::path& file)
    {
      std::vector<fs::path> tests{};
      if(std::ifstream ifile{file})
      {
        while(ifile)
        {
          fs::path source{};
          ifile >> source;
          if(!source.empty())
          {
            tests.push_back(source);
          }
        }
      }

      std::sort(tests.begin(), tests.end());
      return tests;
    }

    [[nodiscard]]
    std::vector<fs::path> find_naively_stale_tests(fs::file_time_type timeStamp, const project_paths& projPaths, std::string_view cutoff)
    {
      using namespace maths;

      std::vector<fs::path> naivelyStaleTests{};

      tests_dependency_graph g{};

      const auto exeTimeStamp{get_stamp(projPaths.executable())};
      add_files(g, projPaths.source(), timeStamp, exeTimeStamp);
      add_files(g, projPaths.tests(), timeStamp, exeTimeStamp);
      g.sort_nodes([&g](auto i, auto j) {
        const fs::path&
          lfile{(g.cbegin_node_weights() + i)->file},
          rfile{(g.cbegin_node_weights() + j)->file};

        const fs::path
          lname{lfile.filename()},
          rname{rfile.filename()};

        return lname != rname ? lname < rname : lfile < rfile;
        });

      // TO DO: process these!
      const auto externalDependencies{build_dependencies(g, projPaths.source_root(), projPaths.tests(), cutoff)};

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
        if(const auto& weight{*i}; is_cpp(weight.file) && in_repo(weight.file, projPaths.tests()))
        {
          const auto relPath{fs::relative(weight.file, projPaths.tests())};

          if(!weight.stale) consider_materials(g, i, relPath, projPaths.test_materials(), timeStamp);

          if(weight.stale) naivelyStaleTests.push_back(relPath);
        }
      }

      std::sort(naivelyStaleTests.begin(), naivelyStaleTests.end());

      return naivelyStaleTests;
    }
  }

  [[nodiscard]]
  std::optional<std::vector<std::filesystem::path>>
  tests_to_run(const project_paths& projPaths, std::string_view cutoff)
  {
    const auto prunePaths{projPaths.prune()};
    const auto timeStamp{get_stamp(prunePaths.stamp())};

    if(!timeStamp) return std::nullopt;

    const auto naivelyStaleTests{find_naively_stale_tests(timeStamp.value(), projPaths, cutoff)},
               passingTests{read_tests(prunePaths.selected_passes(std::nullopt))};

    std::vector<fs::path> staleTests{};
    std::set_difference(naivelyStaleTests.begin(), naivelyStaleTests.end(), passingTests.begin(), passingTests.end(), std::back_inserter(staleTests));

    const std::vector<fs::path> failingTests{read_tests(prunePaths.failures(std::nullopt))};

    std::vector<fs::path> testsToRun{};
    std::set_union(staleTests.begin(), staleTests.end(), failingTests.begin(), failingTests.end(), std::back_inserter(testsToRun));

    return testsToRun;
  }
}
