////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

#include "ErasedFunctionRegularTest.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace sequoia::testing
{
  namespace
  {
    /** Counts its own copies, moves and destructions, so that a wrapper's lifetime behaviour can
        be observed from outside rather than inferred from the fact that nothing crashed.
     */
    struct counter
    {
      inline static int copies{}, moves{}, destructions{};

      static void reset() { copies = moves = destructions = 0; }

      counter() = default;
      counter(const counter&) { ++copies; }
      counter(counter&&) noexcept { ++moves; }
      counter& operator=(const counter&) { ++copies; return *this; }
      counter& operator=(counter&&) noexcept { ++moves; return *this; }
      ~counter() { ++destructions; }
    };

    /** Large enough to defeat the small buffer, which is three pointers, and counted, so that a
        copy of the wrapper which shared the payload rather than cloning it would be seen.
     */
    struct big_payload
    {
      std::array<double, 8> values{};
      counter count{};
    };

    [[nodiscard]]
    int free_function() { return 42; }

    /** Reports which of its call operators was chosen, one per qualification. */
    struct qualifier_probe
    {
      [[nodiscard]]
      constexpr int operator()() & { return 1; }

      [[nodiscard]]
      constexpr int operator()() const & { return 2; }

      [[nodiscard]]
      constexpr int operator()() && { return 3; }

      [[nodiscard]]
      constexpr int operator()() const && { return 4; }
    };

    struct lvalue_only
    {
      [[nodiscard]]
      int operator()() & { return 1; }
    };

    struct rvalue_only
    {
      [[nodiscard]]
      int operator()() && { return 1; }
    };

    struct const_only
    {
      [[nodiscard]]
      int operator()() const { return 1; }
    };

    struct mutable_only
    {
      [[nodiscard]]
      int operator()() { return 1; }
    };

    /** Whether `Function` may be invoked as an lvalue, a const lvalue, an rvalue and a const rvalue. */
    template<class Function>
    [[nodiscard]]
    constexpr std::array<bool, 4> invocable_categories()
    {
      return {std::is_invocable_v<Function&>,
              std::is_invocable_v<const Function&>,
              std::is_invocable_v<Function&&>,
              std::is_invocable_v<const Function&&>};
    }

    /** Whether `Function` is constructible from `lvalue_only`, `rvalue_only`, `const_only` and `mutable_only`. */
    template<class Function>
    [[nodiscard]]
    constexpr std::array<bool, 4> constructible_targets()
    {
      return {std::is_constructible_v<Function, lvalue_only>,
              std::is_constructible_v<Function, rvalue_only>,
              std::is_constructible_v<Function, const_only>,
              std::is_constructible_v<Function, mutable_only>};
    }

    // Each expectation is written out by hand, and, where the standard library has
    // `std::copyable_function`, held against it too, so that neither is only a restatement of the other.
    template<class Signature>
    [[nodiscard]]
    constexpr bool admits_as_expected(std::array<bool, 4> expected)
    {
      return (invocable_categories<erased_function<Signature>>() == expected)
    #if defined(__cpp_lib_copyable_function)
          && (invocable_categories<std::copyable_function<Signature>>() == expected)
    #endif
        ;
    }

    template<class Signature>
    [[nodiscard]]
    constexpr bool constructs_as_expected(std::array<bool, 4> expected)
    {
      return (constructible_targets<erased_function<Signature>>() == expected)
    #if defined(__cpp_lib_copyable_function)
          && (constructible_targets<std::copyable_function<Signature>>() == expected)
    #endif
        ;
    }

    struct adder
    {
      int base{};

      [[nodiscard]]
      constexpr int operator()(int x) const { return base + x; }
    };

    struct summer
    {
      std::vector<int> values;
      int extra{};

      summer(std::initializer_list<int> list, int e)
        : values{list}
        , extra{e}
      {}

      [[nodiscard]]
      int operator()() const { return std::ranges::fold_left(values, extra, std::plus{}); }
    };

    struct throws_on_copy
    {
      throws_on_copy() = default;
      throws_on_copy(const throws_on_copy&) { throw std::runtime_error{"The target's copy"}; }
      throws_on_copy(throws_on_copy&&) noexcept = default;
    };

    struct throws_on_destruction
    {
      ~throws_on_destruction() noexcept(false) {}

      [[nodiscard]]
      int operator()() const { return 1; }
    };

    struct small_target_with_throwing_move
    {
      counter count{};

      small_target_with_throwing_move() = default;
      small_target_with_throwing_move(const small_target_with_throwing_move&) = default;
      small_target_with_throwing_move(small_target_with_throwing_move&&) noexcept(false) = default;

      [[nodiscard]]
      int operator()() const { return 1; }
    };

    static_assert(!std::is_nothrow_move_constructible_v<small_target_with_throwing_move>);

    struct counted_target
    {
      counter count{};

      [[nodiscard]]
      int operator()() const { return 1; }
    };

    enum class unerasable { value };

    template<class G>
    concept exposes_caller_type = requires { typename G::caller_type; };

    template<class G>
    concept exposes_callable = requires { G::template callable_v<int(*)()>; };

    template<class G>
    concept exposes_qualified_object = requires { typename G::template qualified_object<int>; };

    template<class G>
    concept exposes_admission = requires { G::template admits_v<G&>; };

    template<class B>
    concept copy_assignable_through = requires(B& b, const B& c) { b = c; };

    struct member_holder
    {
      [[nodiscard]]
      int value() const { return 42; }
    };
  }

  [[nodiscard]]
  std::filesystem::path erased_function_regular_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void erased_function_regular_test::run_tests()
  {
    test_constraints();
    test_admission();
    test_qualified_construction();
    test_noexcept();
    test_interface();
    test_constant_evaluation();
    test_semantics();
    test_empty_invocation();
    test_self_move_assignment();
    test_throwing_copy_assignment();
    test_small_target();
    test_large_target();
    test_small_target_with_throwing_move();
    test_arguments();
    test_conversion();
    test_qualified_invocation();
    test_construction_from_lvalue();
    test_in_place_construction();
    test_null();
    test_swap();
  }

  void erased_function_regular_test::test_constraints()
  {
    using function_t = erased_function<int() const>;

    STATIC_CHECK(std::constructible_from<function_t, int(*)()>);
    STATIC_CHECK(std::copyable<function_t>);
    STATIC_CHECK(std::default_initializable<function_t>);
    STATIC_CHECK(invocable_r<const function_t&, int>);

    // Targets which cannot satisfy the signature are rejected rather than failing at the call.
    STATIC_CHECK(!std::constructible_from<function_t, int>);
    STATIC_CHECK(!std::constructible_from<function_t, void(*)()>);
    STATIC_CHECK(!std::constructible_from<erased_function<int(int) const>, int(*)(int, int)>);
    STATIC_CHECK(!std::constructible_from<erased_function<void() const>, int(*)()>);

    // A pointer to member satisfies `invocable_r` and is excluded by name
    using member_pointer = decltype(&member_holder::value);
    STATIC_CHECK(invocable_r<member_pointer, int, const member_holder&>);
    STATIC_CHECK(!std::constructible_from<erased_function<int(const member_holder&) const>, member_pointer>);

    // A mutable lambda fails a const-qualified signature only if it captures: a captureless one
    // converts to a function pointer through a const conversion operator.
    STATIC_CHECK(!std::constructible_from<function_t, decltype([i = 0]() mutable { return i; })>);
    STATIC_CHECK(std::constructible_from<function_t, decltype([]() mutable { return 1; })>);

    STATIC_CHECK(!std::constructible_from<function_t, decltype([p = std::unique_ptr<int>{}]() { return 1; })>);

    // Alike but for the destructor
    STATIC_CHECK(std::constructible_from<function_t, const_only>);
    STATIC_CHECK(!std::constructible_from<function_t, throws_on_destruction>);
  }

  void erased_function_regular_test::test_admission()
  {
    // Invocable as {lvalue, const lvalue, rvalue, const rvalue}
    STATIC_CHECK(admits_as_expected<int()>(         {true,  false, true,  false}));
    STATIC_CHECK(admits_as_expected<int() const>(   {true,  true,  true,  true }));
    STATIC_CHECK(admits_as_expected<int() &>(       {true,  false, false, false}));
    STATIC_CHECK(admits_as_expected<int() const &>( {true,  true,  true,  true }));
    STATIC_CHECK(admits_as_expected<int() &&>(      {false, false, true,  false}));
    STATIC_CHECK(admits_as_expected<int() const &&>({false, false, true,  true }));
  }

  void erased_function_regular_test::test_qualified_construction()
  {
    // Constructible from {lvalue_only, rvalue_only, const_only, mutable_only}
    STATIC_CHECK(constructs_as_expected<int()>(         {false, false, true, true }));
    STATIC_CHECK(constructs_as_expected<int() const>(   {false, false, true, false}));
    STATIC_CHECK(constructs_as_expected<int() &>(       {true,  false, true, true }));
    STATIC_CHECK(constructs_as_expected<int() const &>( {false, false, true, false}));
    STATIC_CHECK(constructs_as_expected<int() &&>(      {false, true,  true, true }));
    STATIC_CHECK(constructs_as_expected<int() const &&>({false, false, true, false}));
  }

  void erased_function_regular_test::test_noexcept()
  {
    using function_t       = erased_function<int() const>;
    using nothrow_function = erased_function<int() const noexcept>;

    STATIC_CHECK(std::is_nothrow_invocable_v<const nothrow_function&>);
    STATIC_CHECK(!std::is_nothrow_invocable_v<const function_t&>);
    STATIC_CHECK(std::constructible_from<nothrow_function, decltype([]() noexcept { return 1; })>);
    STATIC_CHECK(!std::constructible_from<nothrow_function, decltype([]() { return 1; })>);

    STATIC_CHECK(!std::is_nothrow_copy_constructible_v<function_t>);
    STATIC_CHECK(!std::is_nothrow_copy_assignable_v<function_t>);
    STATIC_CHECK(std::is_nothrow_move_constructible_v<function_t>);
    STATIC_CHECK(std::is_nothrow_move_assignable_v<function_t>);
  }

  void erased_function_regular_test::test_interface()
  {
    STATIC_CHECK(erasable_signature<int()>);
    STATIC_CHECK(erasable_signature<int(double) const>);
    STATIC_CHECK(erasable_signature<int() &>);
    STATIC_CHECK(erasable_signature<int() const &>);
    STATIC_CHECK(erasable_signature<int() &&>);
    STATIC_CHECK(erasable_signature<int() const && noexcept>);
    STATIC_CHECK(!erasable_signature<int>);
    STATIC_CHECK(!erasable_signature<unerasable>);
    STATIC_CHECK(!erasable_signature<counter>);
    STATIC_CHECK(!erasable_signature<int() volatile>);
    STATIC_CHECK(!erasable_signature<int(...)>);

    using function_t    = erased_function<int() const>;
    using call_operator = sequoia::impl::call_operator_for_t<int() const>;

    STATIC_CHECK(!exposes_caller_type<function_t>);
    STATIC_CHECK(!exposes_callable<function_t>);
    STATIC_CHECK(function_t::callable_through_signature_v<const_only>);
    STATIC_CHECK(!function_t::callable_through_signature_v<mutable_only>);
    STATIC_CHECK(!exposes_qualified_object<function_t>);
    STATIC_CHECK(!exposes_admission<function_t>);
    STATIC_CHECK(!copy_assignable_through<call_operator>);
    STATIC_CHECK(!std::is_destructible_v<call_operator>);
  }

  void erased_function_regular_test::test_constant_evaluation()
  {
    // Every operation, over a target whose copy allocates and one trivially managed at run time,
    // both held behind the pointer here. The evaluator refuses a double delete and a leak, so it
    // witnesses the lifetime; the results pin invocation and swap. Whether the source of a move is
    // disengaged is asked at run time only, since gcc under ubsan cannot evaluate `operator bool`
    // here (GCC bug 71962).
    constexpr auto lifetime{
      []() {
        using function_t = erased_function<int(int) const>;

        const std::string captured{"twelve chars"};
        function_t f{[captured](int x) { return x + static_cast<int>(captured.size()); }};
        function_t copy{f};
        function_t moved{std::move(f)};
        function_t negating{[](int x) { return -x; }};
        function_t assigned{};
        assigned = copy;
        std::ranges::swap(copy, negating);

        const function_t& alias{assigned};
        assigned = alias;

        int sideEffect{};
        const erased_function<void(int) const> setting{[&sideEffect](int x) { sideEffect = x; }};
        setting(3);

        // A result which is a class type with a non-trivial destructor: the shape gcc rejects when the
        // thunk is a lambda called through a function pointer (GCC bug 125000)
        const erased_function<std::string() const> generating{[captured]() { return captured; }};

        const auto generatedSize{static_cast<int>(generating().size())};
        return std::array{assigned(1), copy(1), negating(1), moved(1), sideEffect, generatedSize};
      }
    };

    constexpr std::array expected{13, -1, 13, 13, 3, 12};

    // The cast back from void* in a constant evaluation is C++26's (P2738); a build without it
    // runs the same lifetime at run time, as one check either way, so that neither count in the
    // versioned summary depends on the build
#if __cpp_constexpr >= 202306L
    STATIC_CHECK(lifetime() == expected);
#else
    check("The lifetime at run time; the constant evaluation is untried below C++26", lifetime() == expected);
#endif
  }

  void erased_function_regular_test::test_semantics()
  {
    using result = std::optional<int>;

    constexpr result empty{};
    const int captured{7};
    const big_payload big{.values{0.0, 0.0, 0.0, 2.5}}, otherBig{.values{0.0, 0.0, 0.0, 3.0}};

    const auto trivial{[captured]() { return captured; }};
    const auto byReference{[&captured]() { return captured + 1; }};
    const auto ownManager{[p = std::make_shared<const int>(9)]() { return *p; }};
    const auto otherOwnManager{[p = std::make_shared<const int>(10)]() { return *p; }};
    const auto onHeap{[big]() { return static_cast<int>(big.values[3] * 2); }};
    const auto otherOnHeap{[otherBig]() { return static_cast<int>(otherBig.values[3] * 2); }};

    const auto checkPair{
      [this, &empty](std::string description,
                     observed_function x,
                     observed_function y,
                     result xResult,
                     result yResult) {
        check_semantics(std::move(description), x, y, xResult, yResult, empty, empty);
      }
    };

    checkPair("Empty and trivially managed",                     {},               {trivial},         {}, 7);
    checkPair("Two trivially managed",                           {trivial},        {byReference},     7,  8);
    checkPair("Trivially managed and with a manager of its own", {trivial},        {ownManager},      7,  9);
    checkPair("Two with managers of their own",                  {ownManager},     {otherOwnManager}, 9,  10);
    checkPair("With a manager of its own and on the heap",       {ownManager},     {onHeap},          9,  5);
    checkPair("Two on the heap",                                 {onHeap},         {otherOnHeap},     5,  6);
    checkPair("A function pointer and on the heap",              {&free_function}, {onHeap},          42, 5);
    checkPair("Empty and on the heap",                           {},               {onHeap},          {}, 5);
  }

  void erased_function_regular_test::test_empty_invocation()
  {
    using function_t = erased_function<int() const>;

    // The message of a bad_function_call is the standard library's own, and the three spell it differently
    constexpr auto libraryMessage{
      [](const project_paths&, std::string) -> std::string { return "<the standard library's message>"; }
    };

    check_exception_thrown<std::bad_function_call>(
      "Invoking an empty function",
      []() { return function_t{}(); },
      libraryMessage
    );

    function_t source{[]() { return 1; }};
    const function_t moved{std::move(source)};
    check_exception_thrown<std::bad_function_call>(
      "Invoking the source of a move",
      [&source]() { return source(); },
      libraryMessage
    );
  }

  void erased_function_regular_test::test_self_move_assignment()
  {
    const int captured{7};
    const big_payload big{.values{0.0, 0.0, 0.0, 2.5}};
    const auto ownManager{[p = std::make_shared<const int>(9)]() { return *p; }};

    std::array cases{
      std::tuple{"Trivially managed", observed_function{[captured]() { return captured; }}, 7},
      std::tuple{"With a manager of its own", observed_function{ownManager}, 9},
      std::tuple{"On the heap", observed_function{[big]() { return static_cast<int>(big.values[3] * 2); }}, 5},
      std::tuple{"A function pointer", observed_function{&free_function}, 42}
    };

    for(auto& [description, function, expected] : cases)
    {
      observed_function& alias{function};
      function = std::move(alias);
      check(equivalence, append_lines(description, "Self-move-assignment"), function, std::optional<int>{expected});
    }
  }

  void erased_function_regular_test::test_throwing_copy_assignment()
  {
    using result = std::optional<int>;

    constexpr result empty{};
    const int captured{7};
    const big_payload big{.values{0.0, 0.0, 0.0, 2.5}};

    const auto trivial{[captured]() { return captured; }};
    const auto ownManager{[c = counter{}]() { return 9; }};
    const auto onHeap{[big]() { return static_cast<int>(big.values[3] * 2); }};

    const observed_function throwingInBuffer{[t = throws_on_copy{}]() { return 1; }};
    const observed_function throwingOnHeap{
      [padding = std::array<double, 8>{}, t = throws_on_copy{}]() { return 2 + static_cast<int>(padding[0]); }
    };

    const auto checkAssignment{
      [this](std::string_view description,
             const observed_function& source,
             observed_function target,
             result prediction) {
        counter::reset();
        check_exception_thrown<std::runtime_error>(
          append_lines(description, "The throw from the target's copy propagates"),
          [&source, &target]() { target = source; }
        );

        check(equality,
              append_lines(description, "The assigned-to function's target is not destroyed"),
              counter::destructions,
              0);

        check(equivalence, append_lines(description, "The assigned-to function is unchanged"), target, prediction);
      }
    };

    checkAssignment("In the buffer, over an empty function",             throwingInBuffer, {},           empty);
    checkAssignment("In the buffer, over a trivially managed one",       throwingInBuffer, {trivial},    7);
    checkAssignment("In the buffer, over one with a manager of its own", throwingInBuffer, {ownManager}, 9);
    checkAssignment("In the buffer, over one on the heap",               throwingInBuffer, {onHeap},     5);
    checkAssignment("On the heap, over an empty function",               throwingOnHeap,   {},           empty);
    checkAssignment("On the heap, over a trivially managed one",         throwingOnHeap,   {trivial},    7);
    checkAssignment("On the heap, over one with a manager of its own",   throwingOnHeap,   {ownManager}, 9);
    checkAssignment("On the heap, over one on the heap",                 throwingOnHeap,   {onHeap},     5);
  }

  void erased_function_regular_test::test_small_target()
  {
    const erased_function<int() const> byName{free_function};
    check(equality, "A function passed by name is stored as a pointer to it", byName(), 42);

    // The wrapper must copy and destroy its target exactly as the target expects, which is what a
    // counting payload can show and a lambda cannot.
    counter::reset();
    {
      counter c{};
      erased_function<int() const> f{[c]() { return 1; }};
      check(equality, "Capturing by value copies the payload; the wrapper does not copy it again", counter::copies, 1);
      check(equality, "Constructing from a temporary closure moves the payload", counter::moves, 1);

      const auto g{f};
      check(equality, "Copying the wrapper copies the payload", counter::copies, 2);

      const auto h{std::move(f)};
      check(equality, "Moving the wrapper does not copy", counter::copies, 2);
      check(equality, "Moving the wrapper moves the payload", counter::moves, 2);
    }
    const int constructions{counter::copies + counter::moves + 1};
    check(equality, "Every payload constructed is destroyed", counter::destructions, constructions);

    {
      const erased_function<int() const> replacement{[c = counter{}]() { return 2; }};
      erased_function<int() const> f{[c = counter{}]() { return 1; }};
      counter::reset();
      f = replacement;
      check(equality, "Copy assignment copies the payload once", counter::copies, 1);
      check(equality, "Copy assignment moves the payload once", counter::moves, 1);
      check(equality,
            "Assignment over an engaged small target destroys as many payloads as it constructs",
            counter::destructions,
            counter::copies + counter::moves);

      erased_function<int() const> source{[c = counter{}]() { return 3; }};
      counter::reset();
      f = std::move(source);
      check(equality, "Move assignment does not copy the payload", counter::copies, 0);
      check(equality, "Move assignment moves the payload once", counter::moves, 1);
    }
  }

  void erased_function_regular_test::test_large_target()
  {
    big_payload big{};
    big.values[3] = 2.5;

    // A payload too large for the small buffer is held by pointer, so copying must deep-copy it
    // rather than share it, which the payload's counter sees.
    const erased_function<int() const> f{[big]() { return static_cast<int>(big.values[3] * 2); }};
    counter::reset();
    const auto g{f};
    check(equality, "Copying a wrapper holding a large target copies the payload", counter::copies, 1);
    check(equality, "A large target survives being copied", g(), 5);
    check(equality, "The original is unaffected", f(), 5);

    erased_function<int() const> assignedOver{[big]() { return 0; }};
    counter::reset();
    assignedOver = f;
    check(equality,
          "Assignment over an engaged large target destroys as many payloads as it constructs",
          counter::destructions,
          counter::copies + counter::moves);
  }

  void erased_function_regular_test::test_small_target_with_throwing_move()
  {
    erased_function<int() const> f{small_target_with_throwing_move{}};
    counter::reset();
    const auto g{std::move(f)};
    check(equality, "Moving the function does not move a target whose move may throw", counter::moves, 0);
    check(equality, "The target survives the move", g(), 1);
  }

  void erased_function_regular_test::test_arguments()
  {
    const erased_function<int(int) const> doubler{[](int i) { return 2 * i; }};
    check(equality, "One argument", doubler(21), 42);

    const erased_function<int(int, int) const> adder{[](int i, int j) { return i + j; }};
    check(equality, "Two arguments", adder(20, 22), 42);

    const erased_function<void(int&) const> incrementer{[](int& i) { ++i; }};
    int value{41};
    incrementer(value);
    check(equality, "An argument taken by reference is not copied on the way through", value, 42);

    const erased_function<std::string(const std::string&) const> exclaim{
      [](const std::string& s) { return s + "!"; }
    };
    check(equality, "An argument taken by const reference", exclaim("hello"), std::string{"hello!"});

    const erased_function<int(std::unique_ptr<int>) const> deref{[](std::unique_ptr<int> p) { return *p; }};
    check(equality, "A move-only argument is forwarded", deref(std::make_unique<int>(42)), 42);
  }

  void erased_function_regular_test::test_conversion()
  {
    const std::string held{"held"};
    const erased_function<std::string() const> byReference{[&held]() -> const std::string& { return held; }};
    check(equality, "A target returning a reference satisfies a signature returning a value", byReference(), held);

    const erased_function<double() const> widening{[]() { return 3; }};
    check(equality, "A target returning int satisfies a signature returning double", widening(), 3.0);
  }

  void erased_function_regular_test::test_qualified_invocation()
  {
    erased_function<int()>          unqualified{qualifier_probe{}};
    erased_function<int() const>    constQualified{qualifier_probe{}};
    erased_function<int() &>        lvalueQualified{qualifier_probe{}};
    erased_function<int() const &>  constLvalueQualified{qualifier_probe{}};
    erased_function<int() &&>       rvalueQualified{qualifier_probe{}};
    erased_function<int() const &&> constRvalueQualified{qualifier_probe{}};

    check(equality, "An unqualified signature invokes the target as an lvalue", unqualified(), 1);
    check(equality, "A const signature invokes the target as a const lvalue", constQualified(), 2);
    check(equality, "An & signature invokes the target as an lvalue", lvalueQualified(), 1);
    check(equality, "A const & signature invokes the target as a const lvalue", constLvalueQualified(), 2);
    check(equality, "An && signature invokes the target as an rvalue", std::move(rvalueQualified)(), 3);
    check(equality, "A const && signature invokes the target as a const rvalue", std::move(constRvalueQualified)(), 4);
  }

  void erased_function_regular_test::test_construction_from_lvalue()
  {
    counter c{};
    const auto closure{[c]() { return 1; }};
    counter::reset();
    const erased_function<int() const> f{closure};
    check(equality, "Construction from an lvalue copies the target once", counter::copies, 1);
    check(equality, "Construction from an lvalue does not move the target", counter::moves, 0);
  }

  void erased_function_regular_test::test_in_place_construction()
  {
    const erased_function<int(int) const> f{std::in_place_type<adder>, 5};
    check(equality, "In-place construction from arguments", f(1), 6);

    const erased_function<int() const> g{std::in_place_type<summer>, {1, 2, 3}, 10};
    check(equality, "In-place construction from an initializer list and arguments", g(), 16);

    counter::reset();
    const erased_function<int() const> h{std::in_place_type<counted_target>};
    check(equality, "In-place construction neither copies nor moves the target", counter::copies + counter::moves, 0);
  }

  void erased_function_regular_test::test_null()
  {
    using function_t = erased_function<int() const>;

    constexpr function_t fromNull{nullptr};
    check("Construction from nullptr gives an empty function", !static_cast<bool>(fromNull));

    constexpr int (*nullPointer)(){};
    const function_t fromNullPointer{nullPointer};
    check("Construction from a null function pointer gives an empty function", !static_cast<bool>(fromNullPointer));

    const erased_function<long() const> fromEmpty{function_t{}};
    check("Construction from an empty erased_function gives an empty function", !static_cast<bool>(fromEmpty));

    const erased_function<long() const> fromEngaged{function_t{[]() { return 7; }}};
    check("Construction from an engaged erased_function gives an engaged function", static_cast<bool>(fromEngaged));
    check(equality, "Construction from an engaged erased_function wraps it", fromEngaged(), 7L);

    function_t emptied{[c = counter{}]() { return 1; }};
    counter::reset();
    emptied = nullptr;
    check("Assigning nullptr empties a function", !static_cast<bool>(emptied));
    check(equality, "Assigning nullptr destroys the target", counter::destructions, 1);

    constexpr function_t empty{};
    const function_t engaged{[]() { return 1; }};

    check("An empty function compares equal to nullptr", empty == nullptr);
    check("nullptr compares equal to an empty function", nullptr == empty);
    check("An engaged function compares unequal to nullptr", engaged != nullptr);
    check("nullptr compares unequal to an engaged function", nullptr != engaged);
  }

  void erased_function_regular_test::test_swap()
  {
    using function_t = erased_function<int() const>;

    function_t counted{[c = counter{}]() { return 3; }};
    counter::reset();
    std::ranges::swap(counted, counted);
    check(equality,
          "Swapping a function with itself destroys every payload it moves",
          counter::destructions,
          counter::moves);

    function_t neither{}, alsoNeither{};
    std::ranges::swap(neither, alsoNeither);
    check("Swapping two empty functions leaves the first empty",  !static_cast<bool>(neither));
    check("Swapping two empty functions leaves the second empty", !static_cast<bool>(alsoNeither));
  }

}
