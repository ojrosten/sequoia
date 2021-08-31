////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for TestRunner.hpp.
*/

#include "sequoia/TestFramework/TestRunner.hpp"

#include "sequoia/Parsing/CommandLineArguments.hpp"

#include "sequoia/TestFramework/DependencyAnalyzer.hpp"
#include "sequoia/TestFramework/FileEditors.hpp"
#include "sequoia/TestFramework/ProjectCreator.hpp"
#include "sequoia/TestFramework/Summary.hpp"
#include "sequoia/TestFramework/TestCreator.hpp"

#include "sequoia/TextProcessing/Substitutions.hpp"

#include <fstream>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

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
      nascent_test_factory factory{{"semantic", "allocation", "behavioural"}, runner.paths(), runner.copyright(), runner.code_indent(), runner.stream()};
      auto nascent{factory.create(genus)};

      std::visit(
        variant_visitor{
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
  }

  [[nodiscard]]
  std::string report_time(const test_family::summary& s)
  {
    return report_time(s.log, s.execution_time);
  }

  //=========================================== test_runner ===========================================//

  auto test_runner::time_stamps::from_file(const std::filesystem::path& stampFile) -> stamp
  {
    if(fs::exists(stampFile))
    {
      return fs::last_write_time(stampFile);
    }

    return std::nullopt;
  }

  test_runner::test_runner(int argc, char** argv, std::string copyright, project_paths paths, std::string codeIndent, std::ostream& stream)
    : m_Copyright{std::move(copyright)}
    , m_Paths{std::move(paths)}
    , m_CodeIndent{std::move(codeIndent)}
    , m_Stream{&stream}
  {
    check_indent(m_CodeIndent);

    process_args(argc, argv);

    fs::create_directory(m_Paths.output());
    fs::create_directory(diagnostics_output_path(m_Paths.output()));
    fs::create_directory(test_summaries_path(m_Paths.output()));
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

        std::visit(variant_visitor{[&args](auto& nascent){ nascent.family(args[0]);}}, nascentTests.back());
      }
    };

    const option equivOption{"--equivalent-type", {"-e"}, {"equivalent_type"},
      [&nascentTests](const arg_list& args){
        if(nascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        auto visitor{
          variant_visitor{
            [&args](nascent_semantics_test& nascent){ nascent.add_equivalent_type(args[0]); },
            [](auto&){}
          }
        };

        std::visit(visitor, nascentTests.back());
      }
    };

    const option headerOption{"--class-header", {"-ch"}, {"header of class to test"},
      [&nascentTests](const arg_list& args){
        if(nascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        std::visit(variant_visitor{[&args](auto& nascent){ nascent.header(args[0]); }}, nascentTests.back());
      }
    };

    const option nameOption{"--forename", {"-name"}, {"forename"},
      [&nascentTests](const arg_list& args){
        if(nascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        std::visit(variant_visitor{[&args](auto& nascent){ nascent.forename(args[0]); }}, nascentTests.back());
      }
    };

    const option genFreeSourceOption{"gen-source", {"g"}, {"namespace"},
      [&nascentTests](const arg_list& args) {
        if(nascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        using src_opt = nascent_test_base::gen_source_option;

        auto visitor{
          variant_visitor{
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
          variant_visitor{
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

    const std::vector<option> semanticsOptions{equivOption, familyOption, headerOption, genSemanticsSourceOption};
    const std::vector<option> allocationOptions{familyOption, headerOption};

    const auto help{
      parse_invoke_depth_first(argc, argv,
                { {"test", {"t"}, {"test family name"},
                    [this](const arg_list& args) {
                      m_RunnerMode |= runner_mode::test;
                      m_SelectedFamilies.emplace(args.front(), false);
                    }
                  },
                  {"select", {"s"}, {"source file name"},
                    [this](const arg_list& args) {
                      m_RunnerMode |= runner_mode::test;
                      m_SelectedSources.emplace_back(fs::path{args.front()}.lexically_normal(), false);
                    }
                  },
                  {"prune", {"p"}, {},
                    [this](const arg_list&) {
                      m_RunnerMode |= runner_mode::test;
                      m_PruneInfo.stamps.ondisk = time_stamps::from_file(prune_path(m_Paths.output(), m_Paths.main_cpp_dir()));
                    },
                    {{"--cutoff", {"-c"}, {"Cutoff for #include search e.g. 'namespace'"},
                      [this](const arg_list& args) {
                        m_PruneInfo.include_cutoff = args[0];
                      }}
                    }
                  },
                  {"create", {"c"}, {}, [](const arg_list&) {},
                   { {"regular_test", {"regular"}, {"qualified::class_name<class T>", "equivalent type"},
                      nascent_test_data{"semantic", "regular", *this, nascentTests}, semanticsOptions
                     },
                     {"move_only_test", {"move_only"}, {"qualified::class_name<class T>", "equivalent type"},
                      nascent_test_data{"semantic", "move_only", *this, nascentTests}, semanticsOptions
                     },
                     {"regular_allocation_test", {"regular_allocation", "allocation_test"}, {"raw class name"},
                      nascent_test_data{"allocation", "regular_allocation", *this, nascentTests}, allocationOptions
                     },
                     {"move_only_allocation_test", {"move_only_allocation"}, {"raw class name"},
                      nascent_test_data{"allocation", "move_only_allocation", *this, nascentTests}, allocationOptions
                     },
                     {"free_test", {"free"}, {"header"},
                      nascent_test_data{"behavioural", "free", *this, nascentTests}, {familyOption, nameOption, genFreeSourceOption}
                     },
                     {"performance_test", {"performance"}, {"header"},
                       nascent_test_data{"behavioural", "performance", *this, nascentTests}, {familyOption}
                     }
                   },
                   [this,&nascentTests](const arg_list&) {
                      if(!nascentTests.empty())
                      {
                        m_RunnerMode |= runner_mode::create;
                        variant_visitor visitor{ [](auto& nascent) { nascent.finalize(); } };
                        std::visit(visitor, nascentTests.back());
                      }
                    }
                  },
                  {"init", {"i"}, {"copyright owner", "path ending with project name", "code indent"},
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
                    { {"--no-build", {}, {},
                        [&nascentProjects](const arg_list&) { nascentProjects.back().do_build = build_invocation::no; }},
                      {"--to-files",  {}, {"filename (A file of this name will appear in multiple directories)"},
                        [&nascentProjects](const arg_list& args) { nascentProjects.back().output = args[0]; }},
                      {"--no-ide", {}, {},
                        [&nascentProjects](const arg_list&) {
                          auto& build{nascentProjects.back().do_build};
                          if(build == build_invocation::launch_ide) build = build_invocation::yes;
                        }
                      }
                    }
                  },
                  {"update-materials", {"u"}, {},
                    [this](const arg_list&) {
                      m_RunnerMode |= runner_mode::test;
                      m_UpdateMode = update_mode::soft;
                    }
                  },
                  {"dump", {}, {},
                    [this, recoveryDir{recovery_path(m_Paths.output())}](const arg_list&) {
                      std::filesystem::create_directory(recoveryDir);
                      m_Recovery.dump_file = recoveryDir / "Dump.txt";
                      std::filesystem::remove(m_Recovery.dump_file);
                    }
                  },
                  {"--async", {"-a"}, {},
                    [this](const arg_list&) {
                      if(m_ConcurrencyMode == concurrency_mode::serial)
                        m_ConcurrencyMode = concurrency_mode::dynamic;
                    }
                  },
                  {"--async-depth", {"-ad"}, {"depth [0,1]"},
                    [this](const arg_list& args) {
                      const int i{std::clamp(std::stoi(args.front()), 0, 1)};
                      m_ConcurrencyMode = i ? concurrency_mode::test : concurrency_mode::family;
                    }
                  },
                  {"--verbose",  {"-v"}, {}, [this](const arg_list&) { m_OutputMode = output_mode::verbose; }},
                  {"--recovery", {"-r"}, {},
                    [this,recoveryDir{recovery_path(m_Paths.output())}] (const arg_list&) {
                      std::filesystem::create_directory(recoveryDir);
                      m_Recovery.recovery_file = recoveryDir / "Recovery.txt";
                      std::filesystem::remove(m_Recovery.recovery_file);
                    }
                  }
                },
                [this](std::string_view exe){
                  const auto exePath{
                    [exe]() -> fs::path {
                      const fs::path attempt{exe};
                      if(attempt.is_absolute())
                      {
                        return attempt;
                      }

                      return working_path_v / attempt;
                    }()
                  };

                  if(fs::exists(exePath))
                    m_PruneInfo.stamps.executable = fs::last_write_time(exePath);
                })
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
        cmake_nascent_tests(m_Paths.main_cpp_dir(), m_Paths.cmade_build_dir(), stream());
  
      if(mode(runner_mode::init))
        init_projects(m_Paths.project_root(), nascentProjects, stream());
    }
  }

  void test_runner::check_argument_consistency()
  {
    using parsing::commandline::error;
    using parsing::commandline::warning;

    if(concurrent_execution() && (!m_Recovery.recovery_file.empty() || !m_Recovery.dump_file.empty()))
      throw std::runtime_error{error("Can't run asynchronously in recovery/dump mode\n")};

    if(pruned())
    {
      if((!m_SelectedFamilies.empty() || !m_SelectedSources.empty()))
      {
        stream() << warning("'prune' ignored if either test families or test source files are specified");
        m_PruneInfo.stamps.ondisk = std::nullopt;
      }
      else
      {
        stream() << "\nAnalyzing dependencies...\n";
        const auto start{std::chrono::steady_clock::now()};

        if(auto maybeToRun{tests_to_run(m_Paths.source_root(), m_Paths.tests(), m_Paths.test_materials(), m_PruneInfo.stamps.ondisk, m_PruneInfo.stamps.executable, m_PruneInfo.include_cutoff)})
        {
          auto& toRun{maybeToRun.value()};

          if(std::ifstream ifile{prune_path(m_Paths.output(), m_Paths.main_cpp_dir())})
          {
            while(ifile)
            {
              fs::path source{};
              ifile >> source;
              toRun.push_back(source);
            }
          }

          std::sort(toRun.begin(), toRun.end());
          auto last{std::unique(toRun.begin(), toRun.end())};
          toRun.erase(last, toRun.end());

          std::transform(toRun.begin(), toRun.end(), std::back_inserter(m_SelectedSources),
            [](const fs::path& file) -> std::pair<fs::path, bool> { return {file, false}; });
        }

        const auto end{std::chrono::steady_clock::now()};

        const auto [dur, unit]{testing::stringify(end - start)};
        stream() << '[' << dur << unit << "]\n\n";
      }
    }
  }

  [[nodiscard]]
  bool test_runner::pruned() const noexcept
  {
    return m_PruneInfo.stamps.ondisk.has_value();
  }

  bool test_runner::mark_family(std::string_view name)
  {
    if(m_SelectedFamilies.empty()) return true;

    auto i{m_SelectedFamilies.find(name)};
    if(i != m_SelectedFamilies.end())
    {
      i->second = true;
      return true;
    }

    return false;
  }

  [[nodiscard]]
  test_family::summary test_runner::process_family(const test_family::results& results)
  {
    test_family::summary familySummary{results.execution_time};
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

    std::copy(results.failed_tests.begin(), results.failed_tests.end(), std::back_inserter(m_FailedTestSourceFiles));

    return familySummary;
  }

  namespace
  {
    const std::string& convert(const std::string& s) { return s; }
    std::string convert(const std::filesystem::path& p) { return p.generic_string(); }
  }

  void test_runner::check_for_missing_tests()
  {
    if(pruned()) return;

    auto check{
      [&stream=stream()](const auto& tests, std::string_view type, auto fn) {
        for(const auto& test : tests)
        {
          if(!test.second)
          {
            using namespace parsing::commandline;
            stream << warning(std::string{"Test "}.append(type)
                              .append(" '")
                              .append(convert(test.first))
                              .append("' not found\n")
                              .append(fn(test.first)));
          }
        }
      }
    };

    check(m_SelectedFamilies, "Family", [](const std::string& name) -> std::string {
        if(auto pos{name.rfind('.')}; pos < std::string::npos)
        {
          return "--If trying to select a source file use 'select' rather than 'test'\n";
        }

        return "";
      }
    );

    check(m_SelectedSources, "File", [](const std::filesystem::path& p) -> std::string {
        if(!p.has_extension())
        {
          return "--If trying to test a family use 'test' rather than 'select'\n";
        }
  
        return "";
      }
    );
  }

  [[nodiscard]]
  auto test_runner::find_filename(const std::filesystem::path& filename) -> source_list::iterator
  {
    return std::find_if(m_SelectedSources.begin(), m_SelectedSources.end(),
                 [&filename, &repo=m_Paths.tests(), &root=m_Paths.project_root()](const auto& element){
                   const auto& source{element.first};

                   if(filename.empty() || source.empty() || ((*--source.end()) != (*--filename.end())))
                     return false;

                   if(filename == source) return true;

                   if(filename.is_absolute())
                   {
                     if(rebase_from(filename, root) == rebase_from(source, working_path()))
                       return true;
                   }

                   // filename is relative to where compilation was performed which
                   // cannot be known here. Therefore fallback to assuming the 'selected sources'
                   // live in the test repository

                   if(!repo.empty())
                   {
                     if(rebase_from(source, repo) == rebase_from(filename, repo))
                       return true;

                     if(const auto path{find_in_tree(repo, source)}; !path.empty())
                     {
                       if(rebase_from(path, repo) == rebase_from(filename, repo))
                         return true;
                     }
                   }

                   return false;
                 });
  }

  void test_runner::execute([[maybe_unused]] timer_resolution r)
  {
    if(!mode(runner_mode::help))
    {
      run_tests();
    }
  }

  void test_runner::finalize_concurrency_mode()
  {
    if(m_ConcurrencyMode == concurrency_mode::dynamic)
    {
      m_ConcurrencyMode = (m_Families.size() < 4) ? concurrency_mode::test : concurrency_mode::family;
    }
  }

  void test_runner::run_tests()
  {
    using namespace std::chrono;
    const auto time{steady_clock::now()};

    finalize_concurrency_mode();

    if(mode(runner_mode::test))
    {
      const bool selected{!m_SelectedFamilies.empty() || !m_SelectedSources.empty()};
      log_summary summary{};
      if(!m_Families.empty())
      {
        stream() << "\nRunning tests...\n\n";
        if(!concurrent_execution())
        {
          for(auto& family : m_Families)
          {
            stream() << family.name() << ":\n";
            summary += process_family(family.execute(m_UpdateMode, m_ConcurrencyMode)).log;
          }
        }
        else
        {
          stream() << "\n\t--Using asynchronous execution, level: " << to_string(m_ConcurrencyMode) << "\n\n";
          std::vector<std::pair<std::string, std::future<test_family::results>>> results{};
          results.reserve(m_Families.size());

          for(auto& family : m_Families)
          {
            results.emplace_back(family.name(),
              std::async([&family, umode{m_UpdateMode}, cmode{m_ConcurrencyMode}](){
              return family.execute(umode, cmode); }));
          }

          for(auto& res : results)
          {
            stream() << res.first << ":\n";
            summary += process_family(res.second.get()).log;
          }
        }
        stream() << "\n-----------Grand Totals-----------\n";
        stream() << summarize(summary, steady_clock::now() - time, summary_detail::absent_checks | summary_detail::timings, indentation{"\t"}, no_indent);
      }
      else if(pruned())
      {
        stream() << "Nothing to do: no changes since the last run, therefore 'prune' has pruned all tests\n";
      }
      else if(!selected)
      {
        stream() << "Nothing to do; try creating some tests!\nRun with --help to see options\n";
      }


      if(!selected || pruned())
      {
        const auto stampFile{prune_path(m_Paths.output(), m_Paths.main_cpp_dir())};
        if(std::ofstream ostream{stampFile})
        {
          for(const auto& source : m_FailedTestSourceFiles)
          {
            ostream << source.generic_string() << "\n";
          }
        }

        fs::last_write_time(stampFile, m_PruneInfo.stamps.current);
      }
    }

    check_for_missing_tests();
  }
}
