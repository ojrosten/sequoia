////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Utilities for grouping tests into families.

 */

#include "sequoia/TestFramework/FreeTestCore.hpp"
#include "sequoia/TestFramework/FileSystem.hpp"
#include "sequoia/TestFramework/Summary.hpp"

#include <chrono>
#include <future>
#include <set>
#include <tuple>
#include <vector>

namespace sequoia::testing
{
  enum class update_mode { none=0, soft};

  /*! \brief Specifies the granularity at which concurrent execution is applied */
  enum class concurrency_mode {
    serial,    /// serial execution
    dynamic,   /// determined at runtime
    family,    /// families of tests are executed concurrently
    test,      /// tests are executed concurrently, independently of their families
  };

  [[nodiscard]]
  std::string to_string(concurrency_mode mode);

  struct materials_info
  {
    std::filesystem::path working, prediction, auxiliary;
  };

  class family_info
  {
  public:
    class [[nodiscard]] materials_setter
    {
    public:
      explicit materials_setter(family_info& info);

      materials_setter(const materials_setter&)     = delete;
      materials_setter(materials_setter&&) noexcept = default;

      materials_setter& operator=(const materials_setter&)     = delete;
      materials_setter& operator=(materials_setter&&) noexcept = default;

      [[nodiscard]]
      materials_info set_materials(const std::filesystem::path& sourceFile);
    private:
      family_info* m_pInfo;
      std::vector<std::filesystem::path> m_MaterialsPaths{};
    };
    
    family_info(std::string_view name,
                std::filesystem::path testRepo,
                std::filesystem::path testMaterialsRepo,
                std::filesystem::path outputDir,
                recovery_paths recovery);

    [[nodiscard]]
    const std::string& name() const noexcept { return m_Name; }

    [[nodiscard]]
    const std::filesystem::path& test_repo() const noexcept { return m_TestRepo; }

    [[nodiscard]]
    const std::filesystem::path& output_dir() const noexcept { return m_OutputDir; }

    [[nodiscard]]
    const recovery_paths& recovery() const noexcept { return m_Recovery; }
  private:
    friend materials_setter;
    
    std::string m_Name{};
    std::filesystem::path m_TestRepo{}, m_TestMaterialsRepo{}, m_OutputDir{};
    recovery_paths m_Recovery;
  };

  struct family_results
  {
    log_summary::duration execution_time{};
    std::vector<log_summary> logs;
    std::vector<std::filesystem::path> failed_tests{};
  };


  struct paths
  {
#ifdef _MSC_VER
    // TO DO: remove once there's a Workaround for msvc bug which manifests when
    // a type lacking a default constructor is used in a std::future.
    // https://developercommunity.visualstudio.com/content/problem/60897/c-shared-state-futuresstate-default-constructs-the.html
    paths() = default;
#endif

    paths(const std::filesystem::path& sourceFile,
          const std::filesystem::path& workingMaterials,
          const std::filesystem::path& predictiveMaterials,
          const std::filesystem::path& outputDir,
          const std::filesystem::path& testRepo);

    update_mode mode{update_mode::none};
    std::filesystem::path test_file, summary, workingMaterials, predictions;
  };


  struct paths_comparator
  {
    [[nodiscard]]
    bool operator()(const paths& lhs, const paths& rhs) const noexcept
    {
      return lhs.workingMaterials < rhs.workingMaterials;
    }
  };


  struct family_summary
  {
    explicit family_summary(log_summary::duration t) : execution_time{t}
    {}

    log_summary::duration execution_time{};
    log_summary log{};
  };

  class family_processor
  {
  public:
    explicit family_processor(update_mode mode);

    void process(log_summary summary, const paths& files);

    [[nodiscard]]
    family_results finalize_and_acquire();
  private:
    update_mode m_Mode{};
    timer m_Timer{};
    std::set<std::filesystem::path> m_FilesWrittenTo{};
    std::set<paths, paths_comparator> m_Updateables{};
    family_results m_Results{};

    void to_file(const std::filesystem::path& filename, const log_summary& summary);
  };

  /*! \brief Allows tests to be grouped together into a family of related tests */

  template<concrete_test... Tests>
    requires (sizeof...(Tests) > 0)
  class test_family
  {
  public:
    test_family(std::string name,
                std::filesystem::path testRepo,
                std::filesystem::path testMaterialsRepo,
                std::filesystem::path outputDir,
                recovery_paths recovery,
                Tests&&... tests)
      : m_Info{std::move(name), std::move(testRepo), std::move(testMaterialsRepo), std::move(outputDir), std::move(recovery)}
      , m_Tests{std::forward<Tests>(tests)...}
    {
      family_info::materials_setter setter{m_Info};
      std::apply(
        [&setter,this](auto&... t) { ( set_materials(setter, t), ... ); },
        m_Tests
      );
    }

    test_family(std::string name,
                std::filesystem::path testRepo,
                std::filesystem::path testMaterialsRepo,
                std::filesystem::path outputDir,
                recovery_paths recovery)
      : m_Info{std::move(name), std::move(testRepo), std::move(testMaterialsRepo), std::move(outputDir), std::move(recovery)}
    {}

    test_family(const test_family&)     = delete;
    test_family(test_family&&) noexcept = default;

    test_family& operator=(const test_family&)     = delete;
    test_family& operator=(test_family&&) noexcept = default;

    [[nodiscard]]
    family_info::materials_setter make_materials_setter()
    {
      return family_info::materials_setter{m_Info};
    }

    template<concrete_test T>
    void add_test(family_info::materials_setter& setter, T&& test)
    {
      using test_type = std::optional<std::remove_cvref_t<T>>;

      auto& t{std::get<test_type>(m_Tests)};
      t = std::forward<T>(test);
      set_materials(setter, t);
    }

    [[nodiscard]]
    family_results execute(update_mode updateMode, concurrency_mode concurrenyMode)
    {
      family_processor processor{updateMode};
      auto pathsMaker{
        [&info=m_Info](auto& test) -> paths {
          return {test.source_filename(),
                  test.working_materials(),
                  test.predictive_materials(),
                  info.output_dir(),
                  info.test_repo()};
        }
      };

      if(concurrenyMode < concurrency_mode::test)
      {
        auto process{
          [&processor,pathsMaker](auto& optTest) {
            if(optTest.has_value())
            {
              const auto summary{optTest->execute()};
              processor.process(summary, pathsMaker(*optTest));
            }
          }
        };

        std::apply([process](auto&... optTest) { (process(optTest), ...); }, m_Tests);
      }
      else
      {
        using data = std::pair<log_summary, paths>;
        std::vector<std::future<data>> results{};

        auto generator{
          [&results,pathsMaker](auto& optTest) {
            if(optTest.has_value())
            {
              results.emplace_back(
                std::async([&test=*optTest, pathsMaker](){
                  return std::make_pair(test.execute(), pathsMaker(test)); })
              );
            }
          }
        };

        std::apply([generator](auto&... optTest) { (generator(optTest), ...); }, m_Tests);

        for(auto& r : results)
        {
          const auto[summary, paths]{r.get()};
          processor.process(summary, paths);
        }
      }

      return processor.finalize_and_acquire();
    }

    [[nodiscard]]
    const std::string& name() const noexcept { return m_Info.name(); }

    [[nodiscard]]
    bool empty() const noexcept
    {
      auto has_test{
        [](auto& optTest) {
          return optTest != std::nullopt;
        }
      };

      return !std::apply(
        [has_test](const auto&... optTest){
          return (has_test(optTest) || ...);
        },
        m_Tests);
    }

    void reset()
    {
      family_info::materials_setter setter{m_Info};
      
      auto reset{
        [&setter,this](auto& optTest){
          if(optTest)
          {
            using type = typename std::remove_cvref_t<decltype(optTest)>::value_type;
            *optTest = type{optTest->name()};
            set_materials(setter, optTest);
          }
        }
      };
      
      std::apply([reset](auto&... optTest){ (reset(optTest), ...); }, m_Tests);
    }
  private:
    family_info m_Info;
    std::tuple<std::optional<Tests>...> m_Tests;

    template<concrete_test T>
    void set_materials(family_info::materials_setter& setter, std::optional<T>& t)
    {
      if(t.has_value())
      {
        t->set_filesystem_data(m_Info.test_repo(), m_Info.output_dir(), name());
        t->set_recovery_paths(m_Info.recovery());

        const auto info{setter.set_materials(t->source_filename())};

        t->set_materials(info.working, info.prediction, info.auxiliary);
      }
    }
  };

  class family_vessel
  {
  public:
    template<concrete_test... Tests>
    family_vessel(test_family<Tests...>&& f)
      : m_pFamily{std::make_unique<essence<test_family<Tests...>>>(std::forward<test_family<Tests...>>(f))}
    {}

    family_vessel(const family_vessel&) = delete;
    family_vessel(family_vessel&&) noexcept = default;

    family_vessel& operator=(const family_vessel&) = delete;
    family_vessel& operator=(family_vessel&&) noexcept = default;

    [[nodiscard]]
    const std::string& name() const noexcept
    {
      return m_pFamily->name();
    }

    [[nodiscard]]
    family_results execute(update_mode updateMode, concurrency_mode concurrenyMode)
    {
      return m_pFamily->execute(updateMode, concurrenyMode);
    }

    void reset()
    {
      m_pFamily->reset();
    }
  private:
    struct soul
    {
      virtual ~soul() = default;
      virtual const std::string& name() const noexcept = 0;
      virtual family_results execute(update_mode updateMode, concurrency_mode concurrenyMode) = 0;
      virtual void reset() = 0;
    };

    template<class Family>
    struct essence final : soul
    {
      essence(Family&& f) : m_Family{std::forward<Family>(f)}
      {}

      const std::string& name() const noexcept final
      {
        return m_Family.name();
      }

      [[nodiscard]]
      family_results execute(update_mode updateMode, concurrency_mode concurrenyMode) final
      {
        return m_Family.execute(updateMode, concurrenyMode);
      }

      void reset()
      {
        m_Family.reset();
      }

      Family m_Family;
    };

    std::unique_ptr<soul> m_pFamily{};
  };

  [[nodiscard]]
  std::string summarize(const family_summary& summary, summary_detail suppression, indentation ind_0, indentation ind_1);
}
