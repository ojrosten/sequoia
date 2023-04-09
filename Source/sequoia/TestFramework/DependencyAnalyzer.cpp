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
    bool is_stale(const fs::file_time_type& lastImplicitModTime, const fs::file_time_type& pruneTimeStamp, const std::optional<fs::file_time_type>& exeTimeStamp)
    {
      if(exeTimeStamp.has_value() && (lastImplicitModTime >= exeTimeStamp.value()))
        throw std::runtime_error{"Executable is out of date; please build it!\n"};

      return lastImplicitModTime > pruneTimeStamp;
    }

    struct file_info
    {
      file_info(fs::path f, const fs::file_time_type& pruneTimeStamp, const std::optional<fs::file_time_type>& exeTimeStamp)
        : file{std::move(f)}
        , implicit_modification_time{fs::last_write_time(file)}
        , stale{is_stale(implicit_modification_time, pruneTimeStamp, exeTimeStamp)}
      {}

      file_info(fs::path f)
        : file{std::move(f)}
      {}

      fs::path file;
      fs::file_time_type implicit_modification_time;
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
    bool is_cpp(const fs::path& file)
    {
      const auto ext{file.extension()};
      return (ext == ".cpp") || (ext == ".cc") || (ext == ".cxx");
    }

    [[nodiscard]]
    bool is_header(const fs::path& file)
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

    /// No rebasing perfomed
    void write_tests(const fs::path& file, const std::vector<fs::path>& tests)
    {
      if(std::ofstream ostream{file})
      {
        for(const auto& test : tests)  ostream << test.generic_string() << "\n";
      }
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
    using node_iterator = tests_dependency_graph::iterator;

    void add_files(tests_dependency_graph& g, const fs::path& repo, const fs::file_time_type& pruneTimeStamp, const std::optional<fs::file_time_type>& exeTimeStamp)
    {
      for(const auto& entry : fs::recursive_directory_iterator(repo))
      {
        const auto file{entry.path()};
        if(is_cpp(file) || is_header(file))
        {
          g.add_node(file, pruneTimeStamp, exeTimeStamp);
        }
      }
    }

    /// pre-condition: the nodes of g have been sorted by file path
    void build_dependencies(tests_dependency_graph& g, const project_paths& projPaths, std::string_view cutoff)
    {
      using size_type = tests_dependency_graph::size_type;
      std::vector<fs::path> externalDependencies{};

      for(auto i{g.begin_node_weights()}; i != g.end_node_weights(); ++i)
      {
        const auto nodePos{static_cast<size_type>(distance(g.begin_node_weights(), i))};
        const auto& file{i->file};

        for(const auto& includedFile : get_includes(file, cutoff))
        {
          auto comparer{
            [](const file_info& weight, const file_info& incFile) {
              return weight.file.filename() < incFile.file.filename();
            }
          };

          if(auto [b, e] {std::equal_range(g.begin_node_weights(), g.end_node_weights(), includedFile, comparer)}; b != e)
          {
            auto found{
              [&includedFile,&projPaths,&file](auto b, auto e) {
                while(b != e)
                {
                  if(includedFile.is_absolute())
                  {
                    if(b->file == includedFile) return b;
                    else                        continue;
                  }

                  if((b->file == (projPaths.source().repo() / includedFile)) || (b->file == (projPaths.tests().repo() / includedFile)))
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
              const auto includeNodePos{static_cast<size_type>(distance(g.begin_node_weights(), found))};
              g.join(nodePos, includeNodePos);

              if(is_cpp(file))
              {
                if(file.stem() == includedFile.stem())
                {
                  // Ensure that if cpp is stale, then its associated hpp is
                  // also rendered stale
                  if(i->stale) found->stale = true;

                  found->implicit_modification_time = std::max(i->implicit_modification_time, found->implicit_modification_time);
                }
                else
                {
                  // Furnish the associated header with the same dependencies,
                  // as these are what ultimately determine whether or not
                  // the test cpp is considered stale. Sorting of g ensures
                  // that headers are directly after sources; note that since
                  // only files considered to be headers or sources are added
                  // to g, this is robust.

                  if(auto next{std::next(i)}; next != g.end_node_weights())
                  {
                    if(next->file.stem() == file.stem())
                    {
                      const auto nextPos{static_cast<size_type>(distance(g.begin_node_weights(), next))};
                      g.join(nextPos, includeNodePos);
                    }
                  }
                }
              }
            }
          }
          else
          {
            externalDependencies.push_back(includedFile);
          }
        }
      }

      std::sort(externalDependencies.begin(), externalDependencies.end());
      externalDependencies.erase(std::unique(externalDependencies.begin(), externalDependencies.end()), externalDependencies.end());

      write_tests(projPaths.prune().external_dependencies(), externalDependencies);
    }

    [[nodiscard]]
    bool materials_modified(const fs::path& relFilePath,
                            const fs::path& materialsRepo,
                            const fs::file_time_type pruneTimeStamp)
    {
      const auto materials{materialsRepo / fs::path{relFilePath}.replace_extension("")};
      if(fs::exists(materials))
      {
        for(const auto& entry : fs::recursive_directory_iterator(materials))
        {
          if(fs::last_write_time(entry) > pruneTimeStamp) return true;
        }
      }

      return false;
    }

    [[nodiscard]]
    std::optional<fs::file_time_type> materials_max_write_time(const fs::path& relFilePath, const fs::path& materialsRepo)
    {
      const auto materials{materialsRepo / fs::path{relFilePath}.replace_extension("")};
      if(fs::exists(materials))
      {
        fs::file_time_type maxTime{fs::last_write_time(materials)};

        for(const auto& entry : fs::recursive_directory_iterator(materials))
        {
          maxTime = std::max(maxTime, fs::last_write_time(entry));
        }

        return maxTime;
      }

      return std::nullopt;
    }

    void consider_passing_tests(node_iterator i,
                                const fs::path& relFilePath,
                                const std::vector<fs::path>& passingTests)
    {
      if(auto iter{std::lower_bound(passingTests.begin(), passingTests.end(), relFilePath)}; (iter != passingTests.end() && (*iter == relFilePath)))
      {
        i->stale = false;
      }
    }

    [[nodiscard]]
    std::optional<fs::file_time_type> get_stamp(const fs::path& file)
    {
      if(fs::exists(file)) return fs::last_write_time(file);

      return std::nullopt;
    }

    [[nodiscard]]
    std::vector<fs::path> find_stale_tests(fs::file_time_type pruneTimeStamp, const project_paths& projPaths, std::string_view cutoff)
    {
      using namespace maths;

      tests_dependency_graph g{};

      const auto exeTimeStamp{get_stamp(projPaths.executable())};
      add_files(g, projPaths.source().project(), pruneTimeStamp, exeTimeStamp);
      add_files(g, projPaths.tests().repo(), pruneTimeStamp, exeTimeStamp);
      g.sort_nodes([&g](auto i, auto j) {
        const fs::path&
          lfile{(g.cbegin_node_weights() + i)->file},
          rfile{(g.cbegin_node_weights() + j)->file};

        const fs::path
          lname{lfile.filename()},
          rname{rfile.filename()};

        return lname != rname ? lname < rname : lfile < rfile;
        });

      build_dependencies(g, projPaths, cutoff);

      auto nodesLate{
        [&g](const std::size_t node) {
          for(auto i{g.cbegin_edges(node)}; i != g.cend_edges(node); ++i)
          {
            auto& wt{g.begin_node_weights()[node]};
            auto& targetWt{g.cbegin_node_weights()[i->target_node()]};

            wt.implicit_modification_time = std::max(wt.implicit_modification_time, targetWt.implicit_modification_time);

            if(targetWt.stale) wt.stale = true;
          }
        }
      };

      traverse(depth_first, g, find_disconnected_t{0}, null_func_obj{}, nodesLate);

      const auto passesFile{projPaths.prune().selected_passes(std::nullopt)};
      const auto passingTestsFromFile{read_tests(passesFile)};
      const auto passesStamp{get_stamp(passesFile)};

      std::vector<fs::path> staleTests{};

      for(auto i{g.begin_node_weights()}; i != g.end_node_weights(); ++i)
      {
        if(const auto& weight{*i}; is_cpp(weight.file) && in_repo(weight.file, projPaths.tests().repo()))
        {
          const auto relPath{fs::relative(weight.file, projPaths.tests().repo())};

          if(passesStamp && std::binary_search(passingTestsFromFile.begin(), passingTestsFromFile.end(), relPath))
          {
            const auto materialsWriteTime{materials_max_write_time(relPath, projPaths.test_materials().repo())};
            if(!weight.stale && (materialsWriteTime > pruneTimeStamp))
              i->stale = true;

            const auto maxModificationTime{materialsWriteTime ? std::max(materialsWriteTime.value(), weight.implicit_modification_time) : weight.implicit_modification_time};

            if(weight.stale && (passesStamp.value() >= maxModificationTime))
              consider_passing_tests(i, relPath, passingTestsFromFile);
          }
          else if(!weight.stale)
          {
            if(materials_modified(relPath, projPaths.test_materials().repo(), pruneTimeStamp))
            {
              i->stale = true;
            }
          }

          if(weight.stale) staleTests.push_back(relPath);
        }
      }

      std::sort(staleTests.begin(), staleTests.end());

      return staleTests;
    }

    void update_prune_stamp_on_disk(const prune_paths& prunePaths, fs::file_time_type time)
    {
      const auto stamp{prunePaths.stamp()};
      if(!fs::exists(stamp))
      {
        std::ofstream{stamp};
      }
      fs::last_write_time(stamp, time);
    }

    [[nodiscard]]
    prune_paths prepare(const project_paths& projPaths, std::vector<fs::path>& failedTests)
    {
      std::sort(failedTests.begin(), failedTests.end());

      return projPaths.prune();
    }

    [[nodiscard]]
    std::vector<fs::path> aggregate_failures(const prune_paths& prunePaths, const std::size_t numReps)
    {
      std::vector<fs::path> allTests{};
      for(std::size_t i{}; i < numReps; ++i)
      {
        const auto file{prunePaths.failures(i)};
        read_tests(file, allTests);
      }

      std::sort(allTests.begin(), allTests.end());
      auto last{std::unique(allTests.begin(), allTests.end())};
      allTests.erase(last, allTests.end());

      return allTests;
    }

    [[nodiscard]]
    std::optional<std::vector<fs::path>> aggregate_passes(const prune_paths& prunePaths, const std::size_t numReps)
    {
      std::vector<fs::path> intersection{};
      for(std::size_t i{}; i < numReps; ++i)
      {
        const auto file{prunePaths.selected_passes(i)};
        if(!fs::exists(file)) return std::nullopt;

        std::vector<fs::path> tests{};
        read_tests(file, tests);
        if(i)
        {
          std::vector<fs::path> currentIntersection{};
          std::set_intersection(tests.begin(), tests.end(), intersection.begin(), intersection.end(), std::back_inserter(currentIntersection));
          intersection = std::move(currentIntersection);
        }
        else
        {
          intersection = std::move(tests);
        }
      }

      return intersection;
    }
  }

  std::vector<fs::path>& read_tests(const fs::path& file, std::vector<fs::path>& tests)
  {
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

    return tests;
  }

  [[nodiscard]]
  std::vector<fs::path> read_tests(const fs::path& file)
  {
    std::vector<fs::path> tests{};
    return read_tests(file, tests);
  }

  void write_tests(const project_paths& projPaths, const fs::path& file, const std::vector<fs::path>& tests)
  {
    if(std::ofstream ostream{file})
    {
      for(const auto& test : tests)
      {
        ostream << rebase_from(test, projPaths.tests().repo()).generic_string() << "\n";
      }
    }
  }

  [[nodiscard]]
  std::optional<std::vector<fs::path>>
  tests_to_run(const project_paths& projPaths, std::string_view cutoff)
  {
    const auto prunePaths{projPaths.prune()};
    const auto pruneTimeStamp{get_stamp(prunePaths.stamp())};

    if(!pruneTimeStamp) return std::nullopt;

    const auto staleTests{find_stale_tests(pruneTimeStamp.value(), projPaths, cutoff)};

    const std::vector<fs::path> failingTests{read_tests(prunePaths.failures(std::nullopt))};

    std::vector<fs::path> testsToRun{};
    std::set_union(staleTests.begin(), staleTests.end(), failingTests.begin(), failingTests.end(), std::back_inserter(testsToRun));

    return testsToRun;
  }

  void update_prune_files(const project_paths& projPaths,
                          std::vector<fs::path> failedTests,
                          fs::file_time_type updateTime,
                          std::optional<std::size_t> id)
  {
    const auto prunePaths{prepare(projPaths, failedTests)};

    write_tests(projPaths, prunePaths.failures(id), failedTests);
    fs::remove(prunePaths.selected_passes(id));
    update_prune_stamp_on_disk(prunePaths, updateTime);
  }

  void update_prune_files(const project_paths& projPaths,
                          std::vector<fs::path> executedTests,
                          std::vector<fs::path> failedTests,
                          std::optional<std::size_t> id)
  {
    std::sort(executedTests.begin(), executedTests.end());
    const auto prunePaths{prepare(projPaths, failedTests)};
    const auto passesFile{prunePaths.selected_passes(id)},
               failuresFile{prunePaths.failures(id)};

    const auto previousPasses{read_tests(passesFile)};
    std::vector<fs::path> trialPasses{};

    std::set_union(executedTests.begin(),
                   executedTests.end(),
                   previousPasses.begin(),
                   previousPasses.end(),
                   std::back_inserter(trialPasses));

    std::vector<fs::path> passingTests{};
    std::set_difference(trialPasses.begin(),
                        trialPasses.end(),
                        failedTests.begin(),
                        failedTests.end(),
                        std::back_inserter(passingTests));

    const auto previousFailures{read_tests(failuresFile)};

    std::vector<fs::path> remainingPreviousFailures{};
    std::set_difference(previousFailures.begin(),
                        previousFailures.end(),
                        passingTests.begin(),
                        passingTests.end(),
                        std::back_inserter(remainingPreviousFailures));

    std::vector<fs::path> allFailures{};
    std::set_union(remainingPreviousFailures.begin(),
                   remainingPreviousFailures.end(),
                   failedTests.begin(),
                   failedTests.end(),
                   std::back_inserter(allFailures));

    write_tests(projPaths, failuresFile, allFailures);
    write_tests(projPaths, passesFile, passingTests);
  }

  void setup_instability_analysis_prune_folder(const project_paths& projPaths)
  {
    const auto dir{projPaths.prune().instability_analysis()};
    fs::remove_all(dir);
    fs::create_directories(dir);
  }

  void aggregate_instability_analysis_prune_files(const project_paths& projPaths, prune_mode mode, std::filesystem::file_time_type timeStamp, std::size_t numReps)
  {
    const auto prunePaths{projPaths.prune()};
    auto failingCases{aggregate_failures(prunePaths, numReps)};

    switch(mode)
    {
    case prune_mode::passive:
    {
      if(auto optPasses{aggregate_passes(prunePaths, numReps)})
      {
        auto& executedCases{optPasses.value()};
        executedCases.insert(executedCases.end(), failingCases.begin(), failingCases.end());

        update_prune_files(projPaths, std::move(executedCases), std::move(failingCases), std::nullopt);
      }
      else
      {
        update_prune_files(projPaths, std::move(failingCases), timeStamp, std::nullopt);
      }

      break;
    }
    case prune_mode::active:
    {
      update_prune_files(projPaths, std::move(failingCases), timeStamp, std::nullopt);
      break;
    }
    }

    fs::remove_all(prunePaths.instability_analysis());
  }
}
