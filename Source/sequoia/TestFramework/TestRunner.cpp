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
    : test_runner{argc, argv, std::move(copyright), {"TestAll/TestAllMain.cpp", {}, "TestAll/TestAllMain.cpp"}, std::move(codeIndent), stream}
  {}

  test_runner::test_runner(int argc,
                           char** argv,
                           std::string copyright,
                           const project_paths::initializer& pathsFromProjectRoot,
                           std::string codeIndent,
                           std::ostream& stream)
    : m_Copyright{std::move(copyright)}
    , m_Selector{project_paths{argc, argv, pathsFromProjectRoot}}
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

    const option familyOption{"--family", {"-f"}, {"family"},
      [&nascentTests](const arg_list& args){
        if(nascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        std::visit(variant_visitor{[&args](auto& nascent){ nascent.family(args[0]);}}, nascentTests.back());
      }
    };

    const option diagnosticsOption{"--framework-diagnostics", {"--diagnostics"}, {},
      [&nascentTests](const arg_list&) {
        if(nascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        std::visit(variant_visitor{[](auto& nascent) { nascent.flavour(nascent_test_flavour::framework_diagnostics); }}, nascentTests.back());
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

    const option headerOption{"--header", {"-h"}, {"header of class to test"},
      [&nascentTests](const arg_list& args){
        if(nascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        std::visit(variant_visitor{[&args](auto& nascent){ nascent.header(args[0]); }}, nascentTests.back());
      }
    };

    const option forenameOption{"--test-class-forename", {"--forename"}, {"test class is named <forename>_..."},
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

    const std::initializer_list<maths::tree_initializer<option>> semanticsOptions{{equivOption}, {familyOption}, {headerOption}, {genSemanticsSourceOption}};
    const std::initializer_list<maths::tree_initializer<option>> allocationOptions{{familyOption}, {headerOption}};
    const std::initializer_list<maths::tree_initializer<option>> performanceOptions{{familyOption}};
    const std::initializer_list<maths::tree_initializer<option>> freeOptions{{familyOption}, {forenameOption}, {genFreeSourceOption}, {diagnosticsOption}};

    const auto help{
      parse_invoke_depth_first(argc, argv,
                { {{{"test", {"t"}, {"test family name"},
                    [this](const arg_list& args) {
                      m_RunnerMode |= runner_mode::test;
                      m_Selector.select_family(args.front());
                    }}
                  }},
                  {{{"select", {"s"}, {"source file name"},
                    [this](const arg_list& args) {
                      m_RunnerMode |= runner_mode::test;
                      m_Selector.select_source_file(fs::path{args.front()});
                    }}
                  }},
                  {{{"prune", {"p"}, {},
                    [this](const arg_list&) {
                      m_RunnerMode |= runner_mode::test;
                      m_Selector.enable_prune();
                    },
                    {}},
                    {{{"--cutoff", {"-c"}, {"Cutoff for #include search e.g. 'namespace'"},
                      [this](const arg_list& args) {
                        m_Selector.set_prune_cutoff(args[0]);
                      }}}
                    }
                  }},
                  {{{"create", {"c"}, {},
                        [](const arg_list&) {},
                        [this,&nascentTests](const arg_list&) {
                          if(!nascentTests.empty())
                          {
                            m_RunnerMode |= runner_mode::create;
                            variant_visitor visitor{ [](auto& nascent) { nascent.finalize(); } };
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
        if((m_InstabilityMode == instability_mode::single_instance) || m_InstabilityMode == instability_mode::coordinator)
        {
          const auto dir{proj_paths().prune().instability_analysis()};
          fs::remove_all(dir);
          fs::create_directories(dir);
        }

        m_Selector.prune(stream());
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

    stream() << m_Selector.check_argument_consistency();
  }

  [[nodiscard]]
  family_summary test_runner::process_family(const family_results& results)
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

    std::copy(results.failed_tests.begin(), results.failed_tests.end(), std::back_inserter(m_FailedTestSourceFiles));

    return familySummary;
  }

  void test_runner::execute([[maybe_unused]] timer_resolution r)
  {
    if(!mode(runner_mode::test)) return;

    fs::create_directories(proj_paths().prune().dir());

    stream() << m_Selector.check_for_missing_tests();

    if(m_Selector.empty())
    {
      if(m_Selector.pruned())
      {
        stream() << "Nothing to do: no changes since the last run, therefore 'prune' has pruned all tests\n";
      }
      else if(!m_Selector.bespoke_selection())
      {
        stream() << "Nothing to do; try creating some tests!\nRun with --help to see options\n";
      }

      return;
    }

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
        [&selector=m_Selector] () -> std::string {
          std::string srcs{};
          // TO DO: use ranges when supported by libc++
          for(auto i{selector.begin_selected_sources()}; i != selector.end_selected_sources(); ++i)
          {
            if(i->second) srcs.append(" select " + i->first.path().generic_string());
          }

          for(auto i{selector.begin_selected_families()}; i != selector.end_selected_families(); ++i)
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
          if(i)
          {
            for(auto& f : m_Selector) f.reset();
          }

          const auto optIndex{m_NumReps > 1 ? std::optional<std::size_t>{i} : std::nullopt};
          run_tests(optIndex);
        }
      }
    }

    if(   (m_InstabilityMode == instability_mode::single_instance)
       || (m_InstabilityMode == instability_mode::coordinator))
    {
      aggregate_instability_analysis_prune_files(m_Selector.proj_paths(), m_NumReps);
      const auto outputDir{m_Selector.proj_paths().output().instability_analysis()};
      stream() << instability_analysis(outputDir, m_NumReps);
    }
  }

  void test_runner::finalize_concurrency_mode()
  {
    if(m_ConcurrencyMode == concurrency_mode::dynamic)
    {
      const bool serial{(m_Selector.size() == 1) && (m_Selector.begin()->size() == 1)};
      const bool small{(m_Selector.size() < 4)};
      m_ConcurrencyMode = serial ? concurrency_mode::serial
                        : small  ? concurrency_mode::unit
                                 : concurrency_mode::family;
    }
  }

  void test_runner::run_tests(const std::optional<std::size_t> id)
  {
    const timer t{};
    log_summary summary{};

    stream() << "\nRunning tests...\n\n";
    if(!concurrent_execution())
    {
      for(auto& family : m_Selector)
      {
        stream() << family.name() << ":\n";
        summary += process_family(family.execute(m_UpdateMode, m_ConcurrencyMode, id)).log;
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
        summary += process_family(res.second.get()).log;
      }
    }
    stream() << "\n-----------Grand Totals-----------\n";
    stream() << summarize(summary, t.time_elapsed(), summary_detail::absent_checks | summary_detail::timings, indentation{"\t"}, no_indent);

    m_Selector.update_prune_info(std::move(m_FailedTestSourceFiles), id);
    m_FailedTestSourceFiles.clear();
  }
}
