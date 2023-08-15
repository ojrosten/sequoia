////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "BucketedSequenceRegularTest.hpp"
#include "PartitionedDataGenericTests.hpp"

namespace sequoia::testing
{
  namespace
  {
    using namespace partitioned_data;

    template<class PartitionedData>
    struct bucketed_operations : partitioned_data_operations<PartitionedData>
    {
      using data_t = typename partitioned_data_operations<PartitionedData>::data_t;

      static void execute(regular_test& t)
      {
        auto trg{partitioned_data_operations<PartitionedData>::make_transition_graph(t)};

        // begin 'empty'
        trg.join(data_description::empty,
                 data_description::empty,
                 t.report_line(""),
                 [&t](data_t d) -> data_t {
                   t.check_exception_thrown<std::out_of_range>(t.report_line("begin_partition throws for empty container"), [&d]() { return d.begin_partition(0); });
                   return d;
                 }
          );

        trg.join(data_description::empty,
                 data_description::empty,
                 t.report_line(""),
                 [&t](data_t d) -> data_t {
                   t.check_exception_thrown<std::out_of_range>(t.report_line("end_partition throws for empty container"), [&d]() { return d.end_partition(0); });
                   return d;
                 }
          );

        trg.join(data_description::empty,
                 data_description::empty,
                 t.report_line(""),
                 [&t](const data_t d) -> data_t {
                   t.check_exception_thrown<std::out_of_range>(t.report_line("begin_partition throws for empty container in const context"), [&d]() { return d.begin_partition(0); });
                   return d;
                 }
          );

        trg.join(data_description::empty,
                 data_description::empty,
                 t.report_line(""),
                 [&t](const data_t d) -> data_t {
                   t.check_exception_thrown<std::out_of_range>(t.report_line("end_partition throws for empty container in const context"), [&d]() { return d.end_partition(0); });
                   return d;
                 }
          );

        trg.join(data_description::empty,
                 data_description::empty,
                 t.report_line(""),
                 [&t](data_t d) -> data_t {
                   t.check_exception_thrown<std::out_of_range>(t.report_line("rbegin_partition throws for empty container"), [&d]() { return d.rbegin_partition(0); });
                   return d;
                 }
          );

        trg.join(data_description::empty,
                 data_description::empty,
                 t.report_line(""),
                 [&t](data_t d) -> data_t {
                   t.check_exception_thrown<std::out_of_range>(t.report_line("rend_partition throws for empty container"), [&d]() { return d.rend_partition(0); });
                   return d;
                 }
          );

        trg.join(data_description::empty,
                 data_description::empty,
                 t.report_line(""),
                 [&t](const data_t d) -> data_t {
                   t.check_exception_thrown<std::out_of_range>(t.report_line("rbegin_partition throws for empty container in const context"), [&d]() { return d.rbegin_partition(0); });
                   return d;
                 }
          );

        trg.join(data_description::empty,
                 data_description::empty,
                 t.report_line(""),
                 [&t](const data_t d) -> data_t {
                   t.check_exception_thrown<std::out_of_range>(t.report_line("rend_partition throws for empty container in const context"), [&d]() { return d.rend_partition(0); });
                   return d;
                 }
          );

        trg.join(data_description::empty,
                 data_description::empty,
                 t.report_line(""),
                 [&t](data_t d) -> data_t {
                   t.check_exception_thrown<std::out_of_range>(t.report_line("cbegin_partition throws for empty container"), [&d]() { return d.cbegin_partition(0); });
                   return d;
                 }
          );

        trg.join(data_description::empty,
                 data_description::empty,
                 t.report_line(""),
                 [&t](data_t d) -> data_t {
                   t.check_exception_thrown<std::out_of_range>(t.report_line("cend_partition throws for empty container"), [&d]() { return d.cend_partition(0); });
                   return d;
                 }
          );

        trg.join(data_description::empty,
                 data_description::empty,
                 t.report_line(""),
                 [&t](data_t d) -> data_t {
                   t.check_exception_thrown<std::out_of_range>(t.report_line("crbegin_partition throws for empty container"), [&d]() { return d.crbegin_partition(0); });
                   return d;
                 }
          );

        trg.join(data_description::empty,
                 data_description::empty,
                 t.report_line(""),
                 [&t](data_t d) -> data_t {
                   t.check_exception_thrown<std::out_of_range>(t.report_line("crend_partition throws for empty container"), [&d]() { return d.crend_partition(0); });
                   return d;
                 }
          );

        trg.join(data_description::empty,
                 data_description::empty,
                 t.report_line(""),
                 [&t](data_t d) -> data_t {
                   t.check_exception_thrown<std::out_of_range>(t.report_line("partition throws for empty container"), [&d]() { return d.partition(0); });
                   return d;
                 }
          );

        trg.join(data_description::empty,
                 data_description::empty,
                 t.report_line(""),
                 [&t](const data_t d) -> data_t {
                   t.check_exception_thrown<std::out_of_range>(t.report_line("partition throws for empty container in const context"), [&d]() { return d.partition(0); });
                   return d;
                 }
          );

        trg.join(data_description::empty,
                 data_description::empty,
                 t.report_line(""),
                 [&t](data_t d) -> data_t {
                   t.check_exception_thrown<std::out_of_range>(t.report_line("cpartition throws for empty container"), [&d]() { return d.cpartition(0); });
                   return d;
                 }
          );

        trg.join(data_description::empty,
                 data_description::empty,
                 t.report_line(""),
                 [&t](data_t d) -> data_t {
                   t.check_exception_thrown<std::out_of_range>(t.report_line("Erasing from non-existent partition throws"), [&d]() { return d.erase_from_partition(d.cbegin_partition(0)); });
                   return d;
                 }
          );

        trg.join(data_description::empty,
                 data_description::empty,
                 t.report_line(""),
                 [&t](data_t d) -> data_t {
                   t.check_exception_thrown<std::out_of_range>(t.report_line("Erasing from non-existent partition throws"), [&d]() { return d.erase_from_partition(0, 0); });
                   return d;
                 }
        );

        // end 'empty'
        // begin 'empty_partition'

        trg.join(data_description::empty_partition,
                 data_description::empty_partition,
                 t.report_line(""),
                 [&t](data_t d) -> data_t {
                   auto i{d.begin_partition(1)};
                   t.check(equality, report_line(""), i, {d.end_partition(0).base_iterator(), PartitionedData::npos});
                   return d;
                 }
          );

        trg.join(data_description::empty_partition,
                 data_description::empty_partition,
                 t.report_line(""),
                 [&t](data_t d) -> data_t {
                   auto i{d.erase_from_partition(d.cbegin_partition(1))};
                   t.check(equality, report_line(""), i, {d.end_partition(0).base_iterator(), PartitionedData::npos});
                   return d;
                 }
          );

        // end 'empty_partition'

        auto checker{
            [&t](std::string_view description, const data_t& obtained, const data_t& prediction, const data_t& parent, std::size_t host, std::size_t target) {
              t.check(equality, description, obtained, prediction);
              if(host != target) t.check_semantics(description, prediction, parent);
            }
        };

        transition_checker<data_t>::check(report_line(""), trg, checker);
      }
    };
  }

  [[nodiscard]]
  std::filesystem::path bucketed_sequence_regular_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void bucketed_sequence_regular_test::run_tests()
  {
    using namespace data_structures;
    bucketed_operations<bucketed_sequence<int, object::by_value<int>>>::execute(*this);
  }
}
