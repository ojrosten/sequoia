#pragma once

#include "UnitTestUtils.hpp"
#include "TestThreadingHelper.hpp"

#include <thread>
#include <limits>

#include <iostream>

namespace sequoia
{
  namespace unit_testing
  {
    class test_threading_models : public unit_test
    {
    public:
      using unit_test::unit_test;

    private:
      void run_tests() override;

      void void_return_type_tests();
      void return_value_tests();
      void functor_update_tests();

      template<class Model>
      void update_vector_tests()
      {
	Model threadModel;

	UpdatableFunctor functor;

	threadModel.push(functor, 0);

	threadModel.get();
	
	check_equality(std::vector<int>{0}, functor.get_data());
      }

      template<class Model, const std::size_t nTasks, class... Args>
      void testWaitingTask(Args&&... args)
      {
        Model threadModel{std::forward<Args>(args)...};

        Wait waiter;
        auto wait = [waiter]() { waiter(10); };

        for(std::size_t i=0; i < nTasks; ++i)
        {
          threadModel.push(wait);
        }

        threadModel.get();
      }

      template<class Model, class... Args>
      auto test_lval_task(TriangularNumbers sillyTask, TriangularNumbers sillyTask2, Args&&... args)
      {
        Model threadModel{std::forward<Args>(args)...};;

        threadModel.push(sillyTask);
        threadModel.push(sillyTask2);
        return threadModel.get();
      }

      template<class Model, std::size_t nTasks, class... Args>
      auto test_rval_task(const std::size_t upper, Args&&... args)
      {
        Model threadModel{std::forward<Args>(args)...};;

        for(std::size_t i=0; i < nTasks; ++i)
        {
          threadModel.push(TriangularNumbers(upper - i));
        }

        return threadModel.get();
      }

      template<class Model, class Exception, class... Args>
      void test_exceptions(Args&&... args)
      {
        Model threadModel{std::forward<Args>(args)...};;

        threadModel.push([]() { throw std::runtime_error("Error!"); });

        check_exception_thrown<Exception>([&threadModel]() { threadModel.get(); });
      }

      struct Wait
      {
        void operator()(const std::size_t millisecs) const
        {
          if(millisecs > 0)
          {
            std::this_thread::sleep_for(std::chrono::milliseconds(millisecs));
          }
        }
      };
    };
  }
}
