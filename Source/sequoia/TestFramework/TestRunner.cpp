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

#include "sequoia/Core/Concurrency/ConcurrencyModels.hpp"
#include "sequoia/Parsing/CommandLineArguments.hpp"
#include "sequoia/PlatformSpecific/Preprocessor.hpp"
#include "sequoia/Runtime/ShellCommands.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

#include <fstream>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    const auto entry_time_stamp{std::chrono::file_clock::now()};

    [[nodiscard]]
    std::string running_tests_message(concurrency_mode mode)
    {
      std::string mess{"\nRunning tests"};
      if(mode == concurrency_mode::serial) mess.append(", synchronously");

      return mess.append("...\n\n");
    }

    struct thread_pool_policy
    {
      std::size_t num{8};
    };

    template<class Weight, class UnaryFn>
    void accelerate(thread_pool_policy p, std::span<Weight> weights, UnaryFn fn)
    {
      if(const auto num{weights.size()}; num > 1)
      {
        concurrency::thread_pool<void> pool{std::ranges::min(num, p.num)};
        auto futures{
              std::views::transform(weights, [&pool, fn](auto& wt){ return pool.push([&wt, fn](){ return fn(wt); }); })
            | std::ranges::to<std::vector>()
        };

        for(auto& f : futures) f.get();
      }
      else if(num > 0)
      {
        fn(weights.front());
      }
    }

    template<class ExecutionPolicy, class Weight, class UnaryFn>
    void accelerate(ExecutionPolicy&& policy, std::span<Weight> weights, UnaryFn f)
    {
      if constexpr(!with_clang_v)
      {
        std::for_each(std::forward<ExecutionPolicy>(policy), weights.begin(), weights.end(), std::move(f));
      }
      else
      {
        accelerate(thread_pool_policy{.num{8}}, weights, std::move(f));
      }
    }

    struct test_paths
    {
      test_paths(const std::filesystem::path& sourceFile,
                 const test_summary_path& summaryFile,
                 const std::filesystem::path& workingMaterials,
                 const std::filesystem::path& predictiveMaterials,
                 const project_paths& projPaths)
        : summary{summaryFile}
        , test_file{rebase_from(sourceFile, projPaths.tests().repo())}
        , workingMaterials{workingMaterials}
        , predictions{predictiveMaterials}
      {}

      test_summary_path summary;

      std::filesystem::path
        test_file,
        workingMaterials,
        predictions;
    };

    struct paths_comparator
    {
      [[nodiscard]]
      bool operator()(const test_paths& lhs, const test_paths& rhs) const noexcept
      {
        return lhs.workingMaterials < rhs.workingMaterials;
      }
    };

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
      nascent_test_factory factory{"semantic", "allocation", "behavioural"};
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
    std::string to_async_option(concurrency_mode mode, std::size_t threadPoolSize)
    {
      switch(mode)
      {
      case concurrency_mode::serial:
        return " --serial";
      case concurrency_mode::dynamic:
        return "";
      case concurrency_mode::fixed:
        return " --thread-pool " + std::to_string(threadPoolSize);
      }

      throw std::logic_error{"Illegal option for concurrency_mode"};
    }

    const std::string& convert(const std::string& s) { return s; }
    std::string convert(const std::filesystem::path& p) { return p.generic_string(); }

    enum class is_filtered { no, yes };

    class test_tracker
    {
    public:
      explicit test_tracker(const project_paths& projPaths, std::optional<std::size_t> id, is_filtered isFiltered)
        : m_ProjPaths{projPaths}
        , m_Id{id}
        , m_Filtered{isFiltered}
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
      is_filtered m_Filtered{};

      std::vector<std::filesystem::path> m_FailedTests{}, m_ExecutedTests{};
      std::set<test_paths, paths_comparator> m_Updateables{};
      std::set<std::filesystem::path> m_FilesWrittenTo{};

      void to_file(const test_summary_path& summaryFile, const log_summary& summary)
      {
        const auto& filename{summaryFile.file_path()};
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
        if(m_Filtered == is_filtered::yes)
        {
          update_prune_files(m_ProjPaths, m_ExecutedTests, m_FailedTests, m_Id);
        }
        else
        {
          update_prune_files(m_ProjPaths, m_FailedTests, entry_time_stamp, m_Id);
        }
      }
    };
  }

  individual_materials_paths set_materials(const std::filesystem::path& sourceFile, const project_paths& projPaths, std::vector<std::filesystem::path>& materialsPaths)
  {
    individual_materials_paths materials{sourceFile, projPaths};
    if(!fs::exists(materials.original_materials())) return {};

    const auto workingCopy{materials.working()};
    if(std::ranges::find(materialsPaths, workingCopy) == materialsPaths.cend())
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


  void test_vessel::versioned_write(const std::filesystem::path& file, std::string_view text)
  {
    if(!text.empty() || std::filesystem::exists(file))
    {
      write_to_file(file, text);
    }
  }

  void test_vessel::versioned_write(const std::filesystem::path& file, const failure_output& output)
  {
    for(const auto& info : output)
    {
      versioned_write(file, info.message);
    }
  }

  //=========================================== test_runner ===========================================//

  [[nodiscard]]
  bool test_runner::path_equivalence::operator()(const normal_path& selectedSource, const normal_path& filepath) const
  {
    if(filepath.path().empty() || selectedSource.path().empty() || (back(selectedSource) != back(filepath)))
      return false;

    if(filepath == selectedSource) return true;

    // filepath is relative to where compilation was performed which
    // cannot be known here. Therefore fallback to assuming the 'selected sources'
    // live in the test repository

    if(auto repo{*m_Repo}; !repo.empty())
    {
      if(rebase_from(selectedSource, repo) == rebase_from(filepath, repo))
        return true;

      if(const auto path{find_in_tree(repo, selectedSource)}; !path.empty())
      {
        if(rebase_from(path, repo) == rebase_from(filepath, repo))
          return true;
      }
    }

    return false;
  }

  test_runner::test_runner(int argc,
                           char** argv,
                           std::string copyright,
                           std::string codeIndent,
                           const project_paths::customizer& projectPathsCustomization,
                           std::ostream& stream)
    : m_Copyright{std::move(copyright)}
    , m_ProjPaths{project_paths{argc, argv, projectPathsCustomization}}
    , m_CodeIndent{std::move(codeIndent)}
    , m_Stream{&stream}
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

    const option suiteOption{"--suite", {"-s"}, {"suite name"},
      [&nascentTests](const arg_list& args){
        if(nascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        std::visit(overloaded{[&args](auto& nascent){ nascent.suite(args[0]);}}, nascentTests.back());
      }
    };

    const option diagnosticsOption{"--framework-diagnostics", {"--diagnostics"}, {},
      [&nascentTests](const arg_list&) {
        if(nascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        std::visit(overloaded{[](auto& nascent) { nascent.flavour(nascent_test_flavour::framework_diagnostics); }}, nascentTests.back());
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

    const std::initializer_list<maths::tree_initializer<option>> semanticsOptions{{suiteOption}, {headerOption}, {genSemanticsSourceOption}};
    const std::initializer_list<maths::tree_initializer<option>> allocationOptions{{suiteOption}, {headerOption}};
    const std::initializer_list<maths::tree_initializer<option>> performanceOptions{{suiteOption}};
    const std::initializer_list<maths::tree_initializer<option>> freeOptions{{suiteOption}, {forenameOption}, {genFreeSourceOption}, {diagnosticsOption}};

    const auto help{
      parse_invoke_depth_first(argc, argv,
                { {{{"test", {"t"}, {"test suite name"},
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
                      m_PruneInfo.mode = prune_mode::active;
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
                      {{"--no-git", {}, {},
                        [&nascentProjects](const arg_list&) { nascentProjects.back().use_git = git_invocation::no; }}},
                      {{"--to-files",  {}, {"output filename"},
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
                        throw std::runtime_error{error("Number of repetitions, must be >= 2")};

                      m_InstabilityMode = instability_mode::single_instance;
                      m_NumReps = i;
                    },
                    {}},
                    { {{"--sandbox", {}, {},
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
                  {{{"--serial",  {}, {}, [this](const arg_list&) { m_ConcurrencyMode = concurrency_mode::serial; }}}},
                  {{{"--thread-pool", {}, {"Number of threads, must be >= 1"},
                    [this](const arg_list& args) {
                      if(const auto num{std::stoi(args.front())}; num > 0)
                      {
                        m_ConcurrencyMode = concurrency_mode::fixed;
                        m_PoolSize = num;
                      }
                      else
                      {
                        stream() << warning(std::string{"Thread pool size must be non-zero"});
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

      if(in_mode(runner_mode::create))
        cmake_nascent_tests(proj_paths(), stream());
  
      if(in_mode(runner_mode::init))
        init_projects(proj_paths(), nascentProjects, stream());

      if(in_mode(runner_mode::test))
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

    if((m_PruneInfo.mode == prune_mode::active) && m_Filter)
    {
      m_PruneInfo.mode = prune_mode::passive;
      stream() << warning("'prune' ignored if either test families or test source files are specified\n");
    }
  }

  void test_runner::check_for_missing_tests()
  {
    if(m_PruneInfo.mode == prune_mode::active) return;

    auto check{
      [this](auto&& r, std::string_view type, auto fn) {
        if(!r) return;

        for(const auto& [id, found] : *r)
        {
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

    check(m_Filter.selected_suites(), "Suite", [](const std::string& name) -> std::string {
      if(auto pos{name.rfind('.')}; pos < std::string::npos)
      {
        return "    If trying to select a source file use 'select' rather than 'test'\n";
      }

      return "";
      }
    );

    check(m_Filter.selected_items(), "File", [](const std::filesystem::path& p) -> std::string {
      if(!p.has_extension())
      {
        return "    If trying to test a suite use 'test' rather than 'select'\n";
      }

      return "";
      }
    );
  }

  void test_runner::execute([[maybe_unused]] timer_resolution r)
  {
    if(!in_mode(runner_mode::test))
      return;

    fs::create_directories(proj_paths().prune().dir());
    check_for_missing_tests();

    if(nothing_to_do()) return;

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

          if(auto items{filter.selected_items()})
          {
            for(const auto&[file, found] : *items)
            {
              if(found) srcs.append(" select " + file.path().generic_string());
            }
          }

          if(auto suites{filter.selected_suites()})
          {
            for(const auto&[name, found] : *suites)
            {
              if(found) srcs.append(" test " + name);
            }
          }

          return srcs;
        }()
      };

      for(std::size_t i{}; i < m_NumReps; ++i)
      {
        invoke(runtime::shell_command(proj_paths().executable().string().append(" locate ").append(std::to_string(m_NumReps))
                                                               .append(" --runner-id ").append(std::to_string(i)).append(specified)
                                                               .append(to_async_option(m_ConcurrencyMode, m_PoolSize))));
      }
    }
    else
    {
      if(concurrent_execution()) sort_tests();

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
      aggregate_instability_analysis_prune_files(proj_paths(), m_PruneInfo.mode, entry_time_stamp, m_NumReps);
      const auto outputDir{proj_paths().output().instability_analysis()};
      stream() << instability_analysis(outputDir, m_NumReps);
    }
  }

  void test_runner::sort_tests()
  {
    // TO DO: replace this with a stable_sort
    m_Suites.sort_nodes(1, m_Suites.order(), [&s = m_Suites](auto i, auto j) {
      auto& lhs{s.cbegin_node_weights()[i]};
      auto& rhs{s.cbegin_node_weights()[j]};

      if(!lhs.optTest && !rhs.optTest) return i < j;

      if(!lhs.optTest && rhs.optTest) return true;
      if(lhs.optTest && !rhs.optTest) return false;

      if(!lhs.optTest->parallelizable() && rhs.optTest->parallelizable()) return true;
      if(lhs.optTest->parallelizable() && !rhs.optTest->parallelizable()) return false;

      return i < j;
      });
  }

  void test_runner::run_tests(const std::optional<std::size_t> id)
  {
    const timer t{};

    stream() << running_tests_message(m_ConcurrencyMode);

    std::optional<log_summary::duration> asyncDuration{};
    if(concurrent_execution())
    {
      auto first{std::ranges::find_if(m_Suites.begin_node_weights(), m_Suites.end_node_weights(), [](const auto& wt) -> bool { return wt.optTest != std::nullopt; })};
      auto next{std::ranges::find_if(first, m_Suites.end_node_weights(), [](const auto& wt) -> bool { return wt.optTest->parallelizable(); })};

      auto executor{[&s=m_Suites, id](auto& wt){ wt.summary = wt.optTest->execute(id); }};

      const timer asyncTimer{};
      std::ranges::for_each(first, next, executor);

      switch(m_ConcurrencyMode)
      {
        using enum concurrency_mode;
      case dynamic:
        accelerate(sequoia::execution::par, std::span{next, m_Suites.end_node_weights()}, executor);
        break;
      case fixed:
        accelerate(thread_pool_policy{.num{m_PoolSize}}, std::span{next, m_Suites.end_node_weights()}, executor);
        break;
      default:
        throw std::logic_error{"Unexpected concurrency_mode"};
      }

      asyncDuration = asyncTimer.time_elapsed();
    }

    test_tracker tracker{proj_paths(), id, m_Filter ? is_filtered::yes : is_filtered::no};

    using namespace maths;
    auto nodeEarly{
      [&s = m_Suites,&tracker,id,serial{!concurrent_execution()}](auto n) {
        tracker.increment_depth();
        if(serial)
        {
          auto& wt{s.begin_node_weights()[n]};
          if(wt.optTest) { wt.summary = wt.optTest->execute(id); }
        }
      }
    };

    auto nodeLate{
      [this,&tracker](auto n) {
        m_Suites.mutate_node_weight(
          std::ranges::next(m_Suites.cbegin_node_weights(), n),
          [this, &tracker,n](suite_node& wt) {
            for(const auto& edge : m_Suites.cedges(n))
              wt.summary += std::ranges::next(m_Suites.cbegin_node_weights(), edge.target_node())->summary;

            if(wt.optTest)
            {
              auto pathsMaker{
                  [this](auto& test) -> test_paths {
                    return {test.source_file(),
                            test.summary_file_path(),
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
        [&s=m_Suites,&indent0,&indent1,&stream=stream(),serial{!concurrent_execution()}](auto n) {
          if(n)
          {
            const auto& wt{s.cbegin_node_weights()[n]};
            if(wt.optTest)
            {
              stream << summarize(wt.summary, "", summary_detail::failure_messages | summary_detail::timings, indent0, indent1);
            }
            else
            {
              if(serial)
              {
                const auto message{sequoia::indent(wt.summary.name() + ":", indent0)};
                stream << append_indented(message, report_time(wt.summary, std::nullopt), indent0);
              }
              else
              {
                stream << sequoia::indent(wt.summary.name() + ":", indent0) << '\n';
              }
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
      for(const auto& edge : m_Suites.cedges(0))
      {
        const auto detail{!concurrent_execution() ? summary_detail::failure_messages | summary_detail::timings : summary_detail::failure_messages};
        auto targetNodeIter{std::ranges::next(m_Suites.cbegin_node_weights(), edge.target_node())};
        stream() << summarize(targetNodeIter->summary, ":", detail, no_indent, tab);
      }
    }

    if(asyncDuration) m_Suites.begin_node_weights()->summary.execution_time(*asyncDuration);

    stream() << "\n-----------Grand Totals-----------\n";
    stream() << summarize(m_Suites.cbegin_node_weights()->summary, "", t.time_elapsed(), summary_detail::absent_checks | summary_detail::timings, indentation{"\t"}, no_indent);
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
        m_Suites.mutate_node_weight(std::ranges::next(m_Suites.cbegin_node_weights(), n),
          [&,this](auto& wt){
            if(wt.optTest)
            {
              wt.optTest->reset(proj_paths(), materialsPaths);
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

      if(!m_Filter)
      {
        using suite_t = filter_type::optional_suite_selection::value_type;
        using items_t = filter_type::optional_item_selection::value_type;
        m_Filter = filter_type{{suite_t{}}, {items_t{}}, path_equivalence{proj_paths().tests().repo()}, test_to_path{}};
      }

      return prune_outcome::success;
    }

    m_PruneInfo.mode = prune_mode::passive;

    return prune_outcome::no_time_stamp;
  }

  [[nodiscard]]
  std::string test_runner::duplication_message(std::string_view suiteName, std::string_view testName, const fs::path& source)
  {
    using namespace parsing::commandline;

    return error(std::string{"Suite/Test: \""}
                  .append(suiteName).append("/").append(testName).append("\"\n")
                  .append("Source file: \"").append(source.generic_string()).append("\"\n")
                  .append("Please do not include tests in the same suite"
                    " which both have the same name and are defined"
                    " in the same source file.\n"));
  }

  [[nodiscard]]
  active_recovery_files test_runner::make_active_recovery_paths(recovery_mode mode, const project_paths& projPaths)
  {
    active_recovery_files paths{};
    if((mode & recovery_mode::recovery) == recovery_mode::recovery)
      paths.recovery_file = projPaths.output().recovery().recovery_file();

    if((mode & recovery_mode::dump) == recovery_mode::dump)
      paths.dump_file = projPaths.output().recovery().dump_file();

    return paths;
  }
}
