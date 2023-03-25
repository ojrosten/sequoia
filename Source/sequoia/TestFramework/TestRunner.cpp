////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for TestRunner.hpp
*/

#include "sequoia/TestFramework/TestRunner.hpp"

#include "sequoia/TestFramework/DependencyAnalyzer.hpp"
#include "sequoia/TestFramework/MaterialsUpdater.hpp"
#include "sequoia/TestFramework/ProjectCreator.hpp"
#include "sequoia/TestFramework/Summary.hpp"
#include "sequoia/TestFramework/TestCreator.hpp"

#include "sequoia/Parsing/CommandLineArguments.hpp"
#include "sequoia/Runtime/ShellCommands.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

#include <fstream>
#include <future>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  auto time_stamps::from_file(const std::filesystem::path& stampFile) -> stamp
  {
    if(fs::exists(stampFile))
    {
      return fs::last_write_time(stampFile);
    }

    return std::nullopt;
  }

  namespace
  {
    struct nascent_test_data
    {
      nascent_test_data(std::string type, std::string subType, test_runner& r, std::vector<nascent_test_vessel>& nascentTests);

      void operator()(const parsing::commandline::arg_list& args);

      std::string genus, species;
      test_runner& runner;
      std::vector<nascent_test_vessel>& nascent_tests;
    };


    nascent_test_data::nascent_test_data(std::string type, std::string subType, test_runner& r, std::vector<nascent_test_vessel>& nascentTests)
      : genus{std::move(type)}
      , species{std::move(subType)}
      , runner{r}
      , nascent_tests{nascentTests}
    {}

    void nascent_test_data::operator()(const parsing::commandline::arg_list& args)
    {
      nascent_test_factory factory{{"semantic", "allocation", "behavioural"}};
      auto nascent{factory.make(genus, runner.proj_paths(), runner.copyright(), runner.code_indent(), runner.stream())};

      std::visit(
        overloaded{
          [&args,&species = species](nascent_semantics_test& nascent) {
            nascent.test_type(species);
            nascent.qualified_name(args[0]);
            nascent.add_equivalent_type(args[1]);
          },
          [&args,&species = species](nascent_allocation_test& nascent) {
            nascent.test_type(species);
            nascent.forename(args[0]);
          },
          [&args,&species = species](nascent_behavioural_test& nascent) {
            nascent.test_type(species);
            nascent.header(args[0]);
          }
        },
        nascent);

      nascent_tests.emplace_back(std::move(nascent));
    }

    [[nodiscard]]
    std::string to_async_option(concurrency_mode mode)
    {
      switch(mode)
      {
      case concurrency_mode::serial:
        return " -a null";
      case concurrency_mode::dynamic:
        return "";
      case concurrency_mode::family:
        return " -a family";
      case concurrency_mode::unit:
        return " -a unit";
      }

      throw std::logic_error{"Illegal option for concurrency_mode"};
    }

    const std::string& convert(const std::string& s) { return s; }
    std::string convert(const std::filesystem::path& p) { return p.generic_string(); }

    /*using time_type = std::filesystem::file_time_type;

    struct time_stamps
    {
      using stamp = std::optional<time_type>;

      static auto from_file(const std::filesystem::path& stampFile) -> stamp
      {
        if(fs::exists(stampFile))
        {
          return fs::last_write_time(stampFile);
        }

        return std::nullopt;
      }

      stamp ondisk, executable;
      time_type current{std::chrono::file_clock::now()};
    };*/

    class test_tracker
    {
    public:
      explicit test_tracker(const project_paths& projPaths, std::optional<std::size_t> id, is_filtered filtered)
        : m_ProjPaths{projPaths}
        , m_Id{id}
        , m_PruneInfo{.stamps{time_stamps::from_file(m_ProjPaths.prune().stamp()), time_stamps::from_file(m_ProjPaths.executable())},
                      .filtered{filtered}}
      {}

      void increment_depth() noexcept { ++m_Depth; }

      void decrement_depth() noexcept
      {
        if(--m_Depth == npos)
        {
          for(const auto& update : m_Updateables)
          {
            soft_update(update.workingMaterials, update.predictions);
          }

          update_prune_info();
        }
      }

      void process_test(const test_paths& files, const log_summary& summary, update_mode updateMode)
      {
        m_ExecutedTests.push_back(files.test_file);

        if(summary.soft_failures() || summary.critical_failures())
          m_FailedTests.push_back(files.test_file);

        to_file(files.summary, summary);

        if(updateMode != update_mode::none)
        {
          if(summary.soft_failures())
          {
            if(fs::exists(files.workingMaterials) && fs::exists(files.predictions))
            {
              m_Updateables.insert(files);
            }
          }
        }
      }
    private:
      constexpr static int npos{-1};

      int m_Depth{npos};
      project_paths m_ProjPaths;
      std::optional<std::size_t> m_Id{};
      prune_info m_PruneInfo{};

      std::vector<std::filesystem::path> m_FailedTests{}, m_ExecutedTests{};
      std::set<test_paths, paths_comparator> m_Updateables{};
      std::set<std::filesystem::path> m_FilesWrittenTo{};

      [[nodiscard]]
      prune_mode pruned() const noexcept
      {
        return m_PruneInfo.mode;
      }

      void to_file(const std::filesystem::path& filename, const log_summary& summary)
      {
        if(filename.empty()) return;

        auto mode{std::ios_base::out};
        if(auto found{m_FilesWrittenTo.find(filename)}; found != m_FilesWrittenTo.end())
        {
          mode = std::ios_base::app;
        }
        else
        {
          m_FilesWrittenTo.insert(filename);
        }

        std::filesystem::create_directories(filename.parent_path());

        if(std::ofstream file{filename, mode})
        {
          file << summarize(summary, "", summary_detail::failure_messages, no_indent, no_indent);
        }
        else
        {
          throw std::runtime_error{report_failed_write(filename)};
        }
      }

      void update_prune_info() const
      {
        if((pruned() == prune_mode::passive) && (m_PruneInfo.filtered == is_filtered::yes))
        {
          update_prune_files(m_ProjPaths, m_ExecutedTests, m_FailedTests, m_Id);
        }
        else
        {
          update_prune_files(m_ProjPaths, m_FailedTests, m_PruneInfo.stamps.current, m_Id);
        }
      }
    };
  }

  [[nodiscard]]
  bool path_equivalence::operator()(const normal_path& selectedSource, const normal_path& filepath) const
  {
    if(filepath.path().empty() || selectedSource.path().empty() || (back(selectedSource) != back(filepath)))
      return false;

    if(filepath == selectedSource) return true;

    // filepath is relative to where compilation was performed which
    // cannot be known here. Therefore fallback to assuming the 'selected sources'
    // live in the test repository

    if(!m_Repo.empty())
    {
      if(rebase_from(selectedSource, m_Repo) == rebase_from(filepath, m_Repo))
        return true;

      if(const auto path{find_in_tree(m_Repo, selectedSource)}; !path.empty())
      {
        if(rebase_from(path, m_Repo) == rebase_from(filepath, m_Repo))
          return true;
      }
    }

    return false;
  }

  [[nodiscard]]
  std::string report_time(const family_summary& s)
  {
    return report_time(s.log, s.execution_time);
  }

  //=========================================== test_runner ===========================================//

  test_runner::test_runner(int argc,
                           char** argv,
                           std::string copyright,
                           std::string codeIndent,
                           std::ostream& stream)
    : test_runner{argc,
                  argv,
                  std::move(copyright),
                    {main_paths::default_main_cpp_from_root(), {}, main_paths::default_main_cpp_from_root()},
                  std::move(codeIndent),
                  stream}
  {}

  test_runner::test_runner(int argc,
                           char** argv,
                           std::string copyright,
                           const project_paths::initializer& pathsFromProjectRoot,
                           std::string codeIndent,
                           std::ostream& stream)
    : m_Copyright{std::move(copyright)}
    , m_ProjPaths{project_paths{argc, argv, pathsFromProjectRoot}}
    , m_CodeIndent{std::move(codeIndent)}
    , m_Stream{&stream}
    , m_Filter{test_to_path{}, path_equivalence{proj_paths().tests().repo()}}
  {
    check_indent(m_CodeIndent);

    process_args(argc, argv);

    fs::create_directory(proj_paths().output().dir());
    fs::create_directory(proj_paths().output().diagnostics());
    fs::create_directory(proj_paths().output().test_summaries());
  }

  void test_runner::process_args(int argc, char** argv)
  {
    using namespace parsing::commandline;

    std::vector<nascent_test_vessel> nascentTests{};
    std::vector<project_data> nascentProjects{};

    const option familyOption{"--family", {"-f"}, {"family"},
      [&nascentTests](const arg_list& args){
        if(nascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        std::visit(overloaded{[&args](auto& nascent){ nascent.family(args[0]);}}, nascentTests.back());
      }
    };

    const option diagnosticsOption{"--framework-diagnostics", {"--diagnostics"}, {},
      [&nascentTests](const arg_list&) {
        if(nascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        std::visit(overloaded{[](auto& nascent) { nascent.flavour(nascent_test_flavour::framework_diagnostics); }}, nascentTests.back());
      }
    };

    const option equivOption{"--equivalent-type", {"-e"}, {"equivalent_type"},
      [&nascentTests](const arg_list& args){
        if(nascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        auto visitor{
          overloaded{
            [&args](nascent_semantics_test& nascent){ nascent.add_equivalent_type(args[0]); },
            [](auto&){}
          }
        };

        std::visit(visitor, nascentTests.back());
      }
    };

    const option headerOption{"--header", {"-h"}, {"header of class to test"},
      [&nascentTests](const arg_list& args){
        if(nascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        std::visit(overloaded{[&args](auto& nascent){ nascent.header(args[0]); }}, nascentTests.back());
      }
    };

    const option forenameOption{"--test-class-forename", {"--forename"}, {"test class is named <forename>_..."},
      [&nascentTests](const arg_list& args){
        if(nascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        std::visit(overloaded{[&args](auto& nascent){ nascent.forename(args[0]); }}, nascentTests.back());
      }
    };

    const option genFreeSourceOption{"gen-source", {"g"}, {"namespace"},
      [&nascentTests](const arg_list& args) {
        if(nascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        using src_opt = nascent_test_base::gen_source_option;

        auto visitor{
          overloaded{
            [&args](nascent_behavioural_test& nascent) {
              nascent.generate_source_files(src_opt::yes);
              if(args[0] != "::") nascent.set_namespace(args[0]);
            },
            [](auto&) {}
          }
        };

        std::visit(visitor, nascentTests.back());
      }
    };

    const option genSemanticsSourceOption{"gen-source", {"g"}, {"source dir"},
      [&nascentTests](const arg_list& args) {
        if(nascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        using src_opt = nascent_test_base::gen_source_option;

        auto visitor{
          overloaded{
            [&args](nascent_semantics_test& nascent) {
              nascent.generate_source_files(src_opt::yes);
              nascent.source_dir(args[0]);
            },
            [](auto&) {}
          }
        };

        std::visit(visitor, nascentTests.back());
      }
    };

    const std::initializer_list<maths::tree_initializer<option>> semanticsOptions{{equivOption}, {familyOption}, {headerOption}, {genSemanticsSourceOption}};
    const std::initializer_list<maths::tree_initializer<option>> allocationOptions{{familyOption}, {headerOption}};
    const std::initializer_list<maths::tree_initializer<option>> performanceOptions{{familyOption}};
    const std::initializer_list<maths::tree_initializer<option>> freeOptions{{familyOption}, {forenameOption}, {genFreeSourceOption}, {diagnosticsOption}};

    const auto help{
      parse_invoke_depth_first(argc, argv,
                { {{{"test", {"t"}, {"test family name"},
                    [this](const arg_list& args) {
                      m_RunnerMode |= runner_mode::test;
                      m_Filter.add_selected_suite(args.front());
                    }}
                  }},
                  {{{"select", {"s"}, {"source file name"},
                    [this](const arg_list& args) {
                      m_RunnerMode |= runner_mode::test;
                      m_Filter.add_selected_item(fs::path{args.front()});
                    }}
                  }},
                  {{{"prune", {"p"}, {},
                    [this](const arg_list&) {
                      m_RunnerMode |= runner_mode::test;
                      m_PruneInfo.enable_prune();
                    },
                    {}},
                    {{{"--cutoff", {"-c"}, {"Cutoff for #include search e.g. 'namespace'"},
                      [this](const arg_list& args) {
                        m_PruneInfo.include_cutoff = args[0];
                      }}}
                    }
                  }},
                  {{{"create", {"c"}, {},
                        [](const arg_list&) {},
                        [this,&nascentTests](const arg_list&) {
                          if(!nascentTests.empty())
                          {
                            m_RunnerMode |= runner_mode::create;
                            overloaded visitor{ [](auto& nascent) { nascent.finalize(); } };
                            std::visit(visitor, nascentTests.back());
                          }
                        }},
                    { {{"regular_test", {"regular"}, {"qualified::class_name<class T>", "equivalent type"},
                        nascent_test_data{"semantic", "regular", *this, nascentTests}, {}}, semanticsOptions
                      },
                      {{"move_only_test", {"move_only"}, {"qualified::class_name<class T>", "equivalent type"},
                        nascent_test_data{"semantic", "move_only", *this, nascentTests}, {}}, semanticsOptions
                      },
                      {{"regular_allocation_test", {"regular_allocation", "allocation_test"}, {"raw class name"},
                        nascent_test_data{"allocation", "regular_allocation", *this, nascentTests}, {}}, allocationOptions
                      },
                      {{"move_only_allocation_test", {"move_only_allocation"}, {"raw class name"},
                        nascent_test_data{"allocation", "move_only_allocation", *this, nascentTests}, {}}, allocationOptions
                      },
                      {{"free_test", {"free"}, {"header"},
                        nascent_test_data{"behavioural", "free", *this, nascentTests}, {}}, freeOptions
                      },
                      {{"performance_test", {"performance"}, {"header"},
                         nascent_test_data{"behavioural", "performance", *this, nascentTests}, {}}, performanceOptions
                      }
                    }
                  }},
                  {{{"init", {"i"}, {"copyright owner", "path ending with project name", "code indent"},
                    [this,&nascentProjects](const arg_list& args) {
                      m_RunnerMode |= runner_mode::init;

                      const auto ind{
                        [](std::string arg) {

                          replace_all(arg, "\\t", "\t");
                          return indentation{arg};
                        }
                      };

                      nascentProjects.push_back(project_data{args[0], args[1], ind(args[2])});
                    },
                    {}},
                    { {{"--no-build", {}, {},
                        [&nascentProjects](const arg_list&) { nascentProjects.back().do_build = build_invocation::no; }}},
                      {{"--to-files",  {}, {"filename (A file of this name will appear in multiple directories)"},
                        [&nascentProjects](const arg_list& args) { nascentProjects.back().output = args[0]; }}},
                      {{"--no-ide", {}, {},
                        [&nascentProjects](const arg_list&) {
                          auto& build{nascentProjects.back().do_build};
                          if(build == build_invocation::launch_ide) build = build_invocation::yes;
                        }
                      }}
                    }
                  }},
                  {{{"update-materials", {"u"}, {},
                    [this](const arg_list&) {
                      m_RunnerMode |= runner_mode::test;
                      m_UpdateMode = update_mode::soft;
                    }
                  }}},
                  {{{"locate-instabilities", {"locate"}, {"number of repetitions >= 2"},
                    [this](const arg_list& args) {
                      using parsing::commandline::error;
                      const int i{
                        [arg{args.front()}] (){
                          try
                          {
                            return std::stoi(arg);
                          }
                          catch(const std::exception&)
                          {
                            throw std::runtime_error{"locate-instabilities: unable to interpret '" + arg + "' as an integer number of repetitions"};
                          }
                        }()
                      };

                      if(i < 2)
                        throw std::runtime_error{error("Number of repetitions must be >= 2")};

                      m_InstabilityMode = instability_mode::single_instance;
                      m_NumReps = i;
                    },
                    {}},
                    { {{"--sandbox", {"-s"}, {},
                        [this](const arg_list&) {
                          m_InstabilityMode = instability_mode::coordinator;
                        }}},
                      {{"--runner-id", {}, {"private option, best avoided"},
                        [this](const arg_list& args) {
                          m_RunnerID = std::stoi(args.front());
                          m_InstabilityMode = instability_mode::sandbox;
                        }}}
                    }
                  }},
                  {{{"recover", {}, {},
                    [this, recovery{proj_paths().output().recovery()}](const arg_list&) {
                      if(!std::filesystem::create_directory(recovery.dir()))
                      {
                        std::filesystem::remove(recovery.recovery_file());
                      }
                      m_RecoveryMode |= recovery_mode::recovery;
                      if(m_ConcurrencyMode == concurrency_mode::dynamic)
                        m_ConcurrencyMode = concurrency_mode::serial;
                    }
                  }}},
                  {{{"dump", {}, {},
                    [this, recovery{proj_paths().output().recovery()}](const arg_list&) {
                      if(!std::filesystem::create_directory(recovery.dir()))
                      {
                        std::filesystem::remove(recovery.dump_file());
                      }
                      m_RecoveryMode |= recovery_mode::dump;
                      if(m_ConcurrencyMode == concurrency_mode::dynamic)
                        m_ConcurrencyMode = concurrency_mode::serial;
                    }
                  }}},
                  {{{"--async-depth", {"-a"}, {"depth [null,family,unit]"},
                    [this](const arg_list& args) {
                      const auto& depth{args.front()};
                      if(depth == "null")
                      {
                        m_ConcurrencyMode = concurrency_mode::serial;
                      }
                      else if(depth == "family")
                      {
                        m_ConcurrencyMode = concurrency_mode::family;
                      }
                      else if(depth == "unit")
                      {
                        m_ConcurrencyMode = concurrency_mode::unit;
                      }
                      else
                      {
                        using parsing::commandline::warning;

                        stream() << warning(std::string{"Unrecognized async depth option "}.append(depth).append(" should be one of [null,family,unit]\n"));
                      }
                    }
                  }}},
                  {{{"--verbose",  {"-v"}, {}, [this](const arg_list&) { m_OutputMode = output_mode::verbose; }}}}
                },
                [this](std::string_view){})
        };

    if(!help.empty())
    {
      m_RunnerMode &= runner_mode::help;
      stream() << help;
    }
    else
    {
      if(m_RunnerMode == runner_mode::none)
        m_RunnerMode = runner_mode::test;

      check_argument_consistency();

      if(mode(runner_mode::create))
        cmake_nascent_tests(proj_paths(), stream());
  
      if(mode(runner_mode::init))
        init_projects(proj_paths(), nascentProjects, stream());

      if(mode(runner_mode::test))
      {
        if((m_InstabilityMode == instability_mode::single_instance) || (m_InstabilityMode == instability_mode::coordinator))
        {
          setup_instability_analysis_prune_folder(proj_paths());
        }

        // Note: this needs to be done here, before test families are added
        prune();
      }
    }
  }

  void test_runner::check_argument_consistency()
  {
    using parsing::commandline::warning;
    using parsing::commandline::error;

    if((m_InstabilityMode != instability_mode::none) && (m_UpdateMode == update_mode::soft))
    {
      m_UpdateMode = update_mode::none;
      stream() << warning("Update of materials suppressed when checking for instabilities\n");
    }

    if((m_ConcurrencyMode != concurrency_mode::serial) && (m_RecoveryMode != recovery_mode::none))
      throw std::runtime_error{error("Can't run asynchronously in recovery/dump mode\n")};

    if((m_PruneInfo.mode == prune_mode::active) && !m_Filter.empty())
    {
      m_PruneInfo.mode = prune_mode::passive;
      stream() << "'prune' ignored if either test families or test source files are specified\n";
    }
  }

  void test_runner::check_for_missing_tests()
  {
    if(m_PruneInfo.mode == prune_mode::active) return;

    auto check{
      [this](auto first, auto last, std::string_view type, auto fn) {
        for(; first != last; ++first)
        {
          const auto [id, found]{*first};
          if(!found)
          {
            using namespace parsing::commandline;
            stream() << warning(std::string{"Test "}.append(type)
                                                    .append(" '")
                                                    .append(convert(id))
                                                    .append("' not found\n")
                                                    .append(fn(id)));
          }
        }
      }
    };

    check(m_Filter.begin_selected_suites(), m_Filter.end_selected_suites(), "Family", [](const std::string& name) -> std::string {
      if(auto pos{name.rfind('.')}; pos < std::string::npos)
      {
        return "    If trying to select a source file use 'select' rather than 'test'\n";
      }

      return "";
      }
    );

    check(m_Filter.begin_selected_items(), m_Filter.end_selected_items(), "File", [](const std::filesystem::path& p) -> std::string {
      if(!p.has_extension())
      {
        return "    If trying to test a family use 'test' rather than 'select'\n";
      }

      return "";
      }
    );
  }

  /*[[nodiscard]]
  family_summary test_runner::process_family(family_results results)
  {
    family_summary familySummary{results.execution_time};
    std::string output{};
    const auto detail{summary_detail::failure_messages | summary_detail::timings};
    for(const auto& s : results.logs)
    {
      if(m_OutputMode == output_mode::verbose) output += summarize(s, detail, tab, tab);
      familySummary.log += s;
    }

    if(m_OutputMode == output_mode::verbose)
    {
      output.insert(0, report_time(familySummary));
    }
    else
    {
      output += summarize(familySummary, detail, tab, no_indent);
    }

    stream() << output;


    familySummary.failed_tests = std::move(results.failed_tests);

    return familySummary;
  }*/

  void test_runner::execute([[maybe_unused]] timer_resolution r)
  {
    if(!mode(runner_mode::test))
      return;

    fs::create_directories(proj_paths().prune().dir());
    check_for_missing_tests();

    if(nothing_to_do()) return;

    finalize_concurrency_mode();

    if((m_InstabilityMode != instability_mode::sandbox))
    {
      fs::remove_all(proj_paths().output().instability_analysis());
    }

    if(m_InstabilityMode == instability_mode::coordinator)
    {
      if(proj_paths().executable().empty())
        throw std::runtime_error{"Unable to run in sandbox mode, as executable cannot be found"};

      const auto specified{
        [&filter=m_Filter] () -> std::string {
          std::string srcs{};
          // TO DO: use ranges when supported by libc++
          for(auto i{filter.begin_selected_items()}; i != filter.end_selected_items(); ++i)
          {
            if(i->second) srcs.append(" select " + i->first.path().generic_string());
          }

          for(auto i{filter.begin_selected_suites()}; i != filter.end_selected_suites(); ++i)
          {
            if(i->second) srcs.append(" test " + i->first);
          }

          return srcs;
        }()
      };

      for(std::size_t i{}; i < m_NumReps; ++i)
      {
        invoke(runtime::shell_command(proj_paths().executable().string().append(" locate ").append(std::to_string(m_NumReps))
                                                               .append(" --runner-id ").append(std::to_string(i)).append(specified)
                                                               .append(to_async_option(m_ConcurrencyMode))));
      }
    }
    else
    {
      if(m_InstabilityMode == instability_mode::sandbox)
      {
        run_tests(m_RunnerID);
      }
      else
      {
        for(std::size_t i{}; i < m_NumReps; ++i)
        {
          if(i) reset_tests();

          const auto optIndex{m_NumReps > 1 ? std::optional<std::size_t>{i} : std::nullopt};
          run_tests(optIndex);
        }
      }
    }

    if(   (m_InstabilityMode == instability_mode::single_instance)
       || (m_InstabilityMode == instability_mode::coordinator))
    {
      aggregate_instability_analysis_prune_files(proj_paths(), m_PruneInfo.mode, m_PruneInfo.stamps.current, m_NumReps);
      const auto outputDir{proj_paths().output().instability_analysis()};
      stream() << instability_analysis(outputDir, m_NumReps);
    }
  }

  void test_runner::finalize_concurrency_mode()
  {
    // TO DO: fix this
    if(m_ConcurrencyMode == concurrency_mode::dynamic)
    {
      const bool serial{(m_Suites.order() == 3)};
      const bool small{std::distance(m_Suites.cbegin_edges(0), m_Suites.cend_edges(0)) < 4};
      m_ConcurrencyMode = serial ? concurrency_mode::serial
                        : small  ? concurrency_mode::unit
                                 : concurrency_mode::family;
    }
  }

  void test_runner::run_tests(const std::optional<std::size_t> id)
  {
    const timer t{};
    log_summary summary{};
    //std::vector<fs::path> failedTests;

    /*auto update{
      [&summary, &failedTests](const family_summary& familySummary) {
        summary += familySummary.log;
        failedTests.insert(failedTests.end(), familySummary.failed_tests.begin(), familySummary.failed_tests.end());
      }
    };*/

    stream() << "\nRunning tests...\n\n";
    /*if(!concurrent_execution())
    {
      for(auto& family : m_Selector)
      {
        stream() << family.name() << ":\n";
        update(process_family(family.execute(m_UpdateMode, m_ConcurrencyMode, id)));
      }
    }
    else
    {
      stream() << "\n\t--Using asynchronous execution, level: "
               << to_string(m_ConcurrencyMode)
               << "\n\n";

      std::vector<std::pair<std::string, std::future<family_results>>> results{};
      results.reserve(m_Selector.size());

      for(auto& family : m_Selector)
      {
        results.emplace_back(family.name(),
          std::async([&family, umode=m_UpdateMode, cmode=m_ConcurrencyMode, id](){
            return family.execute(umode, cmode, id); }));
      }

      for(auto& res : results)
      {
        stream() << res.first << ":\n";
        update(process_family(res.second.get()));
      }
    }*/

    const auto detail{summary_detail::failure_messages | summary_detail::timings};

    if(concurrent_execution())
    {
      stream() << "\n\t--Using asynchronous execution, level: "
               << to_string(m_ConcurrencyMode)
               << "\n\n";
    }

    if(m_Suites.order())
    {
      test_tracker tracker{proj_paths(), id, m_Filter.empty() ? is_filtered::no : is_filtered::yes};

      using namespace maths;
      auto nodeEarly{
        [&s=m_Suites,&tracker,id](auto n) {
          tracker.increment_depth();
          s.mutate_node_weight(
            std::next(s.cbegin_node_weights(), n),
            [id](suite_node& wt) {
                if(wt.optTest) { wt.summary = wt.optTest->execute(id); }
            }
          );
        }
      };

      auto nodeLate{
        [this,&tracker](auto n) {
          m_Suites.mutate_node_weight(
            std::next(m_Suites.cbegin_node_weights(), n),
            [this, &tracker,n](suite_node& wt) {
              for(auto i{m_Suites.cbegin_edges(n)}; i != m_Suites.cend_edges(n); ++i)
                wt.summary += std::next(m_Suites.cbegin_node_weights(), i->target_node())->summary;

              if(wt.optTest)
              {
                auto pathsMaker{
                    [this](auto& test) -> test_paths {
                      return {test.source_filename(),
                              test.working_materials(),
                              test.predictive_materials(),
                              proj_paths()};
                    }
                };

                tracker.process_test(pathsMaker(wt.optTest.value()), wt.summary, m_UpdateMode);
              }
            }
          );

          tracker.decrement_depth();
        }
      };

      traverse(depth_first, m_Suites, find_disconnected_t{}, nodeEarly, nodeLate, null_func_obj{});

      if(m_OutputMode == output_mode::verbose)
      {
        indentation indent0{no_indent}, indent1{tab};
        auto printNode{
          [&s=m_Suites,&indent0,&indent1,&stream=stream(),detail](auto n) {
            if(n)
            {
              const auto& wt{s.cbegin_node_weights()[n]};
              if(wt.optTest)
              {
                stream << summarize(wt.summary, "", detail, indent0, indent1);
              }
              else
              {
                auto message{sequoia::indent(wt.summary.name() + ":", indent0)};
                stream << append_indented(message, report_time(wt.summary, wt.summary.execution_time()), indent0);
              }

              indent0.append("\t");
            }
          }
        };

        auto decreaseIndent{ [&indent0](auto n) { if(n) indent0.trim(1); } };

        traverse(depth_first, m_Suites, find_disconnected_t{}, printNode, decreaseIndent, null_func_obj{});
      }
      else
      {
        for(auto i{m_Suites.cbegin_edges(0)}; i != m_Suites.cend_edges(0); ++i)
        {
          auto targetNodeIter{std::next(m_Suites.cbegin_node_weights(), i->target_node())};
          stream() << summarize(targetNodeIter->summary, ":", detail, no_indent, tab);
        }
      }

      stream() << "\n-----------Grand Totals-----------\n";
      stream() << summarize(m_Suites.cbegin_node_weights()->summary, "", t.time_elapsed(), summary_detail::absent_checks | summary_detail::timings, indentation{"\t"}, no_indent);
    }
    else
    {

      stream() << "\n-----------Grand Totals-----------\n";
      stream() << summarize(summary, "", t.time_elapsed(), summary_detail::absent_checks | summary_detail::timings, indentation{"\t"}, no_indent);
    }
  }

  [[nodiscard]]
  bool test_runner::nothing_to_do()
  {
    if(!m_Suites.order())
    {
      stream() << "Nothing to do: try creating some tests!\nRun with --help to see options\n";
      return true;
    }
    else if(m_Suites.order() == 1)
    {
      if(m_PruneInfo.mode == prune_mode::active)
        stream() << "Nothing to do: no changes since the last run, therefore 'prune' has pruned all tests\n";

      return true;
    }

    return false;
  }

  void test_runner::reset_tests()
  {
    using namespace maths;

    std::vector<std::filesystem::path> materialsPaths{};
    std::string suiteName{};

    auto resetFn{
      [&,this](auto n){
        m_Suites.mutate_node_weight(std::next(m_Suites.cbegin_node_weights(), n),
          [&,this](auto& wt){
            if(wt.optTest)
            {
              auto& test{*wt.optTest};

              test.set_filesystem_data(proj_paths(), suiteName);
              test.set_recovery_paths(make_active_recovery_paths(m_RecoveryMode, proj_paths()));
              test.set_materials(set_materials(test.source_filename(), materialsPaths));
            }
            else
            {
              suiteName = wt.summary.name();
            }

            wt.summary = log_summary{wt.summary.name()};
          }
        );
      }
    };

    traverse(depth_first, m_Suites, find_disconnected_t{}, resetFn, null_func_obj{}, null_func_obj{});
  }

  void test_runner::prune()
  {
    if(m_PruneInfo.mode == prune_mode::passive) return;

    // Do this here: if pruning throws an exception, this output should make it clearer what's going on
    stream() << "\nAnalyzing dependencies...\n";
    const timer t{};

    switch(do_prune())
    {
    case prune_outcome::not_attempted:
      break;
    case prune_outcome::no_time_stamp:
      {
        using parsing::commandline::warning;
        stream() << warning({"Time stamp of previous run does not exist, so unable to prune.",
                            "This should be automatically rectified for the next successful run.",
                            "No action required."});
      }
      break;
    case prune_outcome::success:
      {
        const auto [dur, unit] {testing::stringify(t.time_elapsed())};
        stream() << "[" << dur << unit << "]\n\n";
      }
      break;
    }
  }

  [[nodiscard]]
  prune_outcome test_runner::do_prune()
  {
    if(m_PruneInfo.mode == prune_mode::passive) return prune_outcome::not_attempted;

    if(auto maybeToRun{tests_to_run(proj_paths(), m_PruneInfo.include_cutoff)})
    {
      for(const auto& src : maybeToRun.value())
      {
        m_Filter.add_selected_item(src);
      }

      return prune_outcome::success;
    }

    m_PruneInfo.mode = prune_mode::passive;

    return prune_outcome::no_time_stamp;
  }

  [[nodiscard]]
  individual_materials_paths test_runner::set_materials(const std::filesystem::path& sourceFile, std::vector<std::filesystem::path>& materialsPaths) const
  {
    individual_materials_paths materials{sourceFile, proj_paths()};
    if(!fs::exists(materials.original_materials())) return {};

    const auto workingCopy{materials.working()};
    if(std::find(materialsPaths.cbegin(), materialsPaths.cend(), workingCopy) == materialsPaths.cend())
    {
      fs::remove_all(materials.temporary_materials());
      fs::create_directories(materials.temporary_materials());

      if(const auto originalWorking{materials.original_working()}; fs::exists(originalWorking))
      {
        fs::copy(originalWorking, workingCopy, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
      }
      else
      {
        fs::create_directory(workingCopy);
      }

      if(const auto originalAux{materials.original_auxiliary()}; fs::exists(originalAux))
      {
        fs::copy(originalAux, materials.auxiliary(), fs::copy_options::recursive | fs::copy_options::overwrite_existing);
      }

      materialsPaths.emplace_back(workingCopy);
    }

    return materials;
  }

  [[nodiscard]]
  std::string test_runner::duplication_message(std::string_view familyName, std::string_view testName, const fs::path& source)
  {
    using namespace parsing::commandline;

    return error(std::string{"Family/Test: \""}
                  .append(familyName).append("/").append(testName).append("\"\n")
                  .append("Source file: \"").append(source.generic_string()).append("\"\n")
                  .append("Please do not include tests in the same family"
                    " which both have the same name and are defined"
                    " in the same source file.\n"));
  }
}
