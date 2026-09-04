////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include "sequoia/PlatformSpecific/Macros.hpp"

module sequoia.test_framework;

import std;

import sequoia.maths.graph;
import sequoia.streaming;

/** \file
    \brief Definitions for DependencyAnalyzer.hpp
 */

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    struct path_projector
    {
      const fs::path& operator()(const prune_record& record) const { return record.test_path; }
    };
    
    [[nodiscard]]
    bool is_stale(const fs::path& file, const fs::file_time_type& lastImplicitModTime, const fs::file_time_type& stalenessThreshold, const std::optional<fs::file_time_type>& exeTimeStamp)
    {
      if(exeTimeStamp.has_value() && (lastImplicitModTime >= exeTimeStamp.value()))
        throw std::runtime_error{
                std::format(
                  "Executable is out of date; please build it!\nExecutable time stamp: {}\n{} time stamp: {}\n",
                  exeTimeStamp.value(),
                  file.generic_string(),
                  lastImplicitModTime
                )
              };

      return lastImplicitModTime > stalenessThreshold;
    }

    struct file_info
    {
      file_info(fs::path f, const fs::file_time_type& stalenessThreshold, const std::optional<fs::file_time_type>& exeTimeStamp)
        : file{std::move(f)}
        , implicit_modification_time{fs::last_write_time(file)}
        , stale{is_stale(file, implicit_modification_time, stalenessThreshold, exeTimeStamp)}
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
      auto zipped{std::views::zip(file, repo)};
      return std::ranges::find_if(zipped, [](const auto& e) { return std::get<0>(e) != std::get<1>(e); }) == zipped.end();
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
    std::string from_stream(std::istream& istr, std::string_view delimiters)
    {
      constexpr auto eof{std::ifstream::traits_type::eof()};
      using int_type = std::ifstream::int_type;

      std::string str{};

      int_type c{};
      while((c = istr.get()) != eof)
      {
        if(std::ranges::contains(delimiters, c)) break;

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
            // TO DO: Bug here with #endif
            const auto followsHash{from_stream(ifile, " \n")};
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
                      return from_stream(ifile, "\"");
                    }
                    else if(ch == '<')
                    {
                      return from_stream(ifile, ">");
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
            const auto pattern{from_stream(ifile, "\n")};
            if(const auto pos{pattern.find(cutoff)}; pos != std::string::npos)
            {
              break;
            }
          }
        }
      }

      return includes;
    }

    using tests_dependency_graph = maths::directed_graph<maths::null_weight, file_info>;
    using node_iterator = tests_dependency_graph::iterator;

    void add_files(std::vector<file_info>& info, const fs::path& repo, const fs::file_time_type& stalenessThreshold, const std::optional<fs::file_time_type>& exeTimeStamp)
    {
      for(const auto& entry : fs::recursive_directory_iterator(repo))
      {
        const auto file{entry.path()};
        if(is_cpp(file) || is_header(file))
        {
          info.emplace_back(file, stalenessThreshold, exeTimeStamp);
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
        const auto nodePos{static_cast<size_type>(std::ranges::distance(g.begin_node_weights(), i))};
        const auto& file{i->file};

        for(const auto& includedFile : get_includes(file, cutoff))
        {
          if(auto eqrange{std::ranges::equal_range(g.node_weights(), includedFile.filename(), std::ranges::less{}, [](const file_info& weight){ return weight.file.filename(); })}; !eqrange.empty())
          {
            auto found{
              std::ranges::find_if(eqrange, [&includedFile,&projPaths,&file](const file_info& wt){
                  if(includedFile.is_absolute())
                  {
                    if(wt.file == includedFile) return true;
                  }
                  else
                  {
                    if(    (wt.file == (projPaths.source().repo() / includedFile))
                        || (wt.file == (projPaths.tests().repo() / includedFile))
                        || std::ranges::contains(projPaths.additional_dependency_analysis_paths(), wt.file, [&includedFile](const fs::path& p) {  return  p / includedFile; })
                      )
                      return true;

                    if(const auto trial{file.parent_path() / includedFile}; fs::exists(trial) && (wt.file == fs::canonical(trial)))
                      return true;
                  }

                  return false;
                }
              )
            };

            if(found != eqrange.end())
            {
              const auto includeNodePos{static_cast<size_type>(std::ranges::distance(g.begin_node_weights(), found))};
              g.join(nodePos, includeNodePos);

              if(is_cpp(file))
              {
                if(file.stem() == includedFile.stem())
                {
                  // Ensure that if cpp is stale, then its associated hpp is
                  // also rendered stale
                  if(i->stale) found->stale = true;

                  found->implicit_modification_time = std::ranges::max(i->implicit_modification_time, found->implicit_modification_time);
                }
                else
                {
                  // Furnish the associated header with the same dependencies,
                  // as these are what ultimately determine whether or not
                  // the test cpp is considered stale. Sorting of g ensures
                  // that headers are directly after sources; note that since
                  // only files considered to be headers or sources are added
                  // to g, this is robust.

                  if(auto next{std::ranges::next(i)}; next != g.end_node_weights())
                  {
                    if(next->file.stem() == file.stem())
                    {
                      const auto nextPos{static_cast<size_type>(std::ranges::distance(g.begin_node_weights(), next))};
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

      std::ranges::sort(externalDependencies);
      auto iters{std::ranges::unique(externalDependencies)};
      externalDependencies.erase(iters.begin(), iters.end());

      write_tests(projPaths.prune().external_dependencies(), externalDependencies);
    }

    [[nodiscard]]
    bool materials_modified(const fs::path& relFilePath,
                            const fs::path& materialsRepo,
                            const fs::file_time_type stalenessThreshold)
    {
      const auto materials{materialsRepo / fs::path{relFilePath}.replace_extension("")};
      if(fs::exists(materials))
      {
        for(const auto& entry : fs::recursive_directory_iterator(materials))
        {
          if(fs::last_write_time(entry) > stalenessThreshold) return true;
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
          maxTime = std::ranges::max(maxTime, fs::last_write_time(entry));
        }

        return maxTime;
      }

      return std::nullopt;
    }

    void consider_passing_tests(node_iterator i,
                                const fs::path& relFilePath,
                                std::span<const prune_record> passingTests,
                                fs::file_time_type maxModificationTime)
    {
      auto iter{std::ranges::lower_bound(passingTests, relFilePath, {}, path_projector{})};
      if((iter != passingTests.end()) && (iter->test_path == relFilePath) && (iter->time_stamp > maxModificationTime))
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
    std::vector<fs::path> find_stale_tests(fs::file_time_type stalenessThreshold, const project_paths& projPaths, std::string_view cutoff)
    {
      using namespace maths;

      tests_dependency_graph g{};

      const auto exeTimeStamp{get_stamp(projPaths.executable())};
      std::vector<file_info> files{};

      add_files(files, projPaths.source().repo(), stalenessThreshold, exeTimeStamp);
      add_files(files, projPaths.tests().repo(), stalenessThreshold, exeTimeStamp);
      for(const auto& p : projPaths.additional_dependency_analysis_paths())
      {
        add_files(files, p, stalenessThreshold, exeTimeStamp);
      }

      std::ranges::sort(
        files,
        [](const auto& lhs, const auto& rhs) {
          const fs::path& lfile{lhs.file}, rfile{rhs.file};

          const fs::path
            lname{lfile.filename()},
            rname{rfile.filename()};

          return lname != rname ? lname < rname : lfile < rfile;
        }
      );

      for(const auto& info : files)
      {
        g.add_node(info);
      }

      build_dependencies(g, projPaths, cutoff);

      auto nodesLate{
        [&g](const std::size_t node) {
          for(const auto& edge : g.cedges(node))
          {
            auto& wt{g.begin_node_weights()[node]};
            auto& targetWt{g.cbegin_node_weights()[edge.target_node()]};

            wt.implicit_modification_time = std::ranges::max(wt.implicit_modification_time, targetWt.implicit_modification_time);

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

          if(passesStamp && std::ranges::binary_search(passingTestsFromFile, relPath, {}, path_projector{}))
          {
            const auto materialsWriteTime{materials_max_write_time(relPath, projPaths.test_materials().repo())};
            if(!weight.stale && (materialsWriteTime > stalenessThreshold))
              i->stale = true;

            const auto maxModificationTime{materialsWriteTime ? std::ranges::max(materialsWriteTime.value(), weight.implicit_modification_time) : weight.implicit_modification_time};

            if(weight.stale && (passesStamp.value() > maxModificationTime))
              consider_passing_tests(i, relPath, passingTestsFromFile, maxModificationTime);
          }
          else if(!weight.stale)
          {
            if(materials_modified(relPath, projPaths.test_materials().repo(), stalenessThreshold))
            {
              i->stale = true;
            }
          }

          if(weight.stale) staleTests.push_back(relPath);
        }
      }

      std::ranges::sort(staleTests);

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

    std::vector<prune_record>& read_tests_to(const fs::path& file, std::vector<prune_record>& tests)
    {
      
      if(std::ifstream ifile{file})
      {
        using iter_t = std::istream_iterator<prune_record>;
        tests.append_range(
            std::ranges::subrange{iter_t{ifile}, iter_t{}}
          | std::views::filter([](const prune_record& record) {return !record.test_path.empty();})
        );
      }

      return tests;
    }

    struct least_path_most_recent{
      [[nodiscard]]
      bool operator()(const prune_record& lhs, const prune_record& rhs) const {
        auto comp{lhs.test_path <=> rhs.test_path};
        return comp == 0 ? lhs.time_stamp > rhs.time_stamp : comp < 0;
      }
    };

    std::vector<prune_record>& to_unique_range(std::vector<prune_record>& r) {
      auto erased{std::ranges::unique(r, {}, path_projector{})};
      r.erase(erased.begin(), erased.end());
      return r;
    }
    
    [[nodiscard]]
    std::vector<prune_record> aggregate_failures(const prune_paths& prunePaths, const std::size_t numReps)
    {
      std::vector<prune_record> allTests{};
      for(auto i : std::views::iota(0uz, numReps))
      {
        read_tests_to(prunePaths.failures(i), allTests);
      }

      return to_unique_range(allTests);
    }

    [[nodiscard]]
    std::optional<std::vector<prune_record>> aggregate_passes(const prune_paths& prunePaths, const std::size_t numReps)
    {
      std::vector<prune_record> intersection{};
      for(auto i : std::views::iota(0uz, numReps))
      {
        const auto file{prunePaths.selected_passes(i)};
        if(!fs::exists(file)) return std::nullopt;

        std::vector<prune_record> tests{testing::read_tests(file)};
        if(i)
        {
          std::vector<prune_record> currentIntersection{};
          std::ranges::set_intersection(tests, intersection, std::back_inserter(currentIntersection));
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

  [[nodiscard]]
  fs::file_time_type staleness_threshold(const fs::file_time_type stamp)
  {
    using namespace std::chrono;
    const auto subSecond{stamp.time_since_epoch() - floor<seconds>(stamp.time_since_epoch())};

    return (subSecond == fs::file_time_type::duration::zero())
      ? stamp - duration_cast<fs::file_time_type::duration>(seconds{1})
      : stamp;
  }

  [[nodiscard]]
  std::vector<prune_record> read_tests(const fs::path& file)
  {
    std::vector<prune_record> tests{};
    return read_tests_to(file, tests);
  }

  void write_tests(const project_paths& projPaths, const fs::path& file, std::span<const prune_record> tests)
  {
    if(std::ofstream ostream{file})
    {
      for(const auto& test : tests)
      {
        ostream << prune_record{rebase_from(test.test_path, projPaths.tests().repo()), test.time_stamp} << "\n";
      }
    }
  }

  namespace
  {
    void do_update_prune_files(const project_paths& projPaths,
                          std::vector<prune_record> failedTests,
                          fs::file_time_type updateTime,
                          std::optional<std::size_t> id)
    {
      std::ranges::sort(failedTests, least_path_most_recent{});
      
      const auto prunePaths{projPaths.prune()};
      write_tests(projPaths, prunePaths.failures(id), failedTests);
      fs::remove(prunePaths.selected_passes(id));
      update_prune_stamp_on_disk(prunePaths, updateTime);
    }

    void do_update_prune_files(const project_paths& projPaths,
                               std::vector<prune_record> executedTests,
                               std::vector<prune_record> failedTests,
                               std::optional<std::size_t> id)
    {
      std::ranges::sort(executedTests, least_path_most_recent{});
      std::ranges::sort(failedTests, least_path_most_recent{});
      to_unique_range(executedTests);

      auto unionize{
        [](std::span<const prune_record> a, std::span<const prune_record> b){
          std::vector<prune_record> tests{};
          std::ranges::set_union(a, b, std::back_inserter(tests), least_path_most_recent{});
          to_unique_range(tests);
          return tests;
        }
      };

      auto difference{
        [](std::span<const prune_record> a, std::span<const prune_record> b){
          std::vector<prune_record> tests{};
          std::ranges::set_difference(a, b, std::back_inserter(tests), {}, path_projector{}, path_projector{});
          return tests;
        }
      };

      const auto prunePaths{projPaths.prune()};
      const auto passesFile{prunePaths.selected_passes(id)},
                 failuresFile{prunePaths.failures(id)};
      
      const std::vector<prune_record> trialPasses{unionize(executedTests, read_tests(passesFile))};      
      const std::vector<prune_record> passingTests{difference(trialPasses, failedTests)};
      const std::vector<prune_record> remainingPreviousFailures{difference(read_tests(failuresFile), passingTests)};
      const std::vector<prune_record> allFailures{unionize(remainingPreviousFailures, failedTests)};

      write_tests(projPaths, failuresFile, allFailures);
      write_tests(projPaths, passesFile, passingTests);
    }

    [[nodiscard]]
    std::vector<prune_record> build_prune_records(std::span<const fs::path> tests, fs::file_time_type updateTime) {
      return   std::views::transform(tests, [updateTime](const fs::path& p){ return prune_record{p, updateTime}; })
             | std::ranges::to<std::vector>();
    }
  }

  [[nodiscard]]
  std::optional<std::vector<fs::path>>
  tests_to_run(const project_paths& projPaths, std::string_view cutoff)
  {
    const auto prunePaths{projPaths.prune()};
    const auto pruneTimeStamp{get_stamp(prunePaths.stamp())};

    if(!pruneTimeStamp) return std::nullopt;

    const auto staleTests{find_stale_tests(staleness_threshold(pruneTimeStamp.value()), projPaths, cutoff)};

    const std::vector<fs::path> failingTests{
      std::views::transform(read_tests(prunePaths.failures(std::nullopt)), path_projector{}) | std::ranges::to<std::vector>()
    };

    std::vector<fs::path> testsToRun{};
    std::ranges::set_union(staleTests, failingTests, std::back_inserter(testsToRun));

    return testsToRun;
  }

  void update_prune_files(const project_paths& projPaths,
                          std::span<const fs::path> failedTests,
                          fs::file_time_type updateTime,
                          std::optional<std::size_t> id)
  {
    do_update_prune_files(
      projPaths,
      build_prune_records(failedTests, updateTime),
      updateTime,
      id
    );
  }

  void update_prune_files(const project_paths& projPaths,
                          std::span<const fs::path> executedTests,
                          std::span<const fs::path> failedTests,
                          fs::file_time_type updateTime,
                          std::optional<std::size_t> id)
  {    
    do_update_prune_files(
      projPaths,
      build_prune_records(executedTests, updateTime),
      build_prune_records(failedTests, updateTime),
      id
    );
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

        do_update_prune_files(projPaths, std::move(executedCases), std::move(failingCases), std::nullopt);
      }
      else
      {
        do_update_prune_files(projPaths, std::move(failingCases), timeStamp, std::nullopt);
      }

      break;
    }
    case prune_mode::active:
    {
      do_update_prune_files(projPaths, std::move(failingCases), timeStamp, std::nullopt);
      break;
    }
    }

    fs::remove_all(prunePaths.instability_analysis());
  }
}
