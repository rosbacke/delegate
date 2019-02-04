namespace test_ns
{
#include "delegate/delegate.hpp"
}

using test_ns::delegate;
using test_ns::mem_fkn;

#include <functional>

#include <cassert>
#include <cstdint>
#include <limits>

#include <gtest/gtest.h>

#if __cplusplus < 201103L
#error "Require at least C++11 to compile delegate"
#endif

static int
freeFkn(int x)
{
    return x + 5;
}

static int s_obj = 0;
void
testFkn12()
{
    s_obj = 1;
}

void
testFkn12(int)
{
    s_obj = 2;
}

// Ensure the language uses the signature in the delegate type to disambiguate
// function names.
TEST(delegate, languageAllowPtrOverloadSet)
{
    delegate<void()> cb;
    cb.set<testFkn12>();
    cb();
    EXPECT_EQ(s_obj, 1);

    delegate<void(int)> cb2;
    cb2.set<testFkn12>();
    cb2(1);
    EXPECT_EQ(s_obj, 2);
}

TEST(delegate, Free_static_function_set_make)
{
    auto del = delegate<int(int)>::make<freeFkn>();
    EXPECT_EQ(del(1), 6);

    del = nullptr;
    EXPECT_EQ(del(1), 0);

    del.set<freeFkn>();
    EXPECT_EQ(del(1), 6);
}

TEST(delegate, Free_dynamic_function_constructor)
{
    using Del = delegate<int(int)>;
    Del del;
    EXPECT_FALSE(del);
    Del del2(freeFkn);
    EXPECT_EQ(del2(1), 6);

    // Handle implicit conversion from stateless lambda to fkn ptr.
    Del del3([](int x) -> int { return x + 7; });
    EXPECT_EQ(del3(1), 8);

    auto del4 = Del{[](int x) -> int { return x + 9; }};
    EXPECT_EQ(del4(1), 10);
}

TEST(delegate, implicit_conversion_lambda_construct_assign)
{
    auto lam = []() { return 42; };
    delegate<int()> del1{lam};
    delegate<int()> del2; //= lam;
    del2 = lam;

    // This is weird. Doen't work, even though del3{ lam } works and del2 = lam
    // works... Does it have something to do with Dan Saks 'friends' talk?
    // delegate<int()> del3 = lam;
    // delegate<int()> del12 = []() { return 42; };

    // Workaround do:
    delegate<int()> del10{lam};
    delegate<int()> del11 = {lam};
    delegate<int()> del12 = {[]() { return 42; }};

    auto const lam2 = []() { return 42; };
    delegate<int()> del4{lam2};
    delegate<int()> del5; // = lam2;
    del5 = lam2;
}

TEST(delegate, Free_dynamic_function_set_make)
{
    auto del = delegate<int(int)>::make(freeFkn);
    EXPECT_EQ(del(1), 6);

    del = nullptr;
    EXPECT_EQ(del(1), 0);

    del.set(freeFkn);
    EXPECT_EQ(del(1), 6);

    // Make sure vanilla usage of lambda works. Rely on conversion to ordinary
    // function pointer. (Requires explicit casting.)
    del.set(static_cast<int (*)(int)>([](int x) -> int { return x + 9; }));
    EXPECT_EQ(del(1), 10);

    del = delegate<int(int)>::make(
        static_cast<int (*)(int)>([](int x) -> int { return x + 9; }));
    EXPECT_EQ(del(1), 10);

    // Use another member name to get implicit function conversion.
    del.set_fkn([](int x) -> int { return x + 9; });
    EXPECT_EQ(del(1), 10);

    del = delegate<int(int)>::make_fkn([](int x) -> int { return x + 9; });
    EXPECT_EQ(del(1), 10);
}

TEST(delegate, value_semantics)
{
    // Default construct.
    delegate<int(int)> del1;
    EXPECT_TRUE(del1.null());
    EXPECT_EQ(del1(0), 0);

    auto del2 = delegate<int(int)>::make<freeFkn>();
    EXPECT_EQ(del2(1), 6);

    // Copy construct
    auto del3{del2};
    EXPECT_EQ(del3(1), 6);
    EXPECT_FALSE(del3.null());

    // Assignment.
    delegate<int(int)> del4;
    del4 = del3;
    EXPECT_EQ(del4(1), 6);

    // Equality
    EXPECT_TRUE(del4 == del3);
    EXPECT_FALSE(del4 == del1);

    EXPECT_FALSE(del4 != del3);
    EXPECT_TRUE(del4 != del1);
}

TEST(delegate, is_trivially_copyable)
{
    // Note: being constexpr and trivial is mutually exclusive.
    // Trivial require default initialization do not set a value,
    // constexpr require to set a value.
    // We can fulfill _trivially_copyable -> this class can safely be
    // memcpy:ied between places.

    using Del = delegate<int(int)>;
    EXPECT_TRUE(std::is_trivially_copyable<Del>::value);
    EXPECT_TRUE(std::is_standard_layout<Del>::value);
}

TEST(mem_fkn, is_trivially_copyable)
{
    // Note: being constexpr and trivial is mutually exclusive.
    // Trivial require default initialization do not set a value,
    // constexpr require to set a value.
    // We can fulfill _trivially_copyable -> this class can safely be
    // memcpy:ied between places.

    struct Obj
    {
    };

    using MemFknT = mem_fkn<Obj, true, int(int)>;
    EXPECT_TRUE(std::is_trivially_copyable<MemFknT>::value);
    EXPECT_TRUE(std::is_standard_layout<MemFknT>::value);

    using MemFknF = mem_fkn<Obj, false, int(int)>;
    EXPECT_TRUE(std::is_trivially_copyable<MemFknF>::value);
    EXPECT_TRUE(std::is_standard_layout<MemFknF>::value);
}

TEST(delegate, nulltests)
{
    // Default construct, Can call default constructed.
    delegate<void()> del;

    EXPECT_FALSE(del);

    EXPECT_TRUE(del == nullptr);
    EXPECT_TRUE(nullptr == del);

    EXPECT_FALSE(del != nullptr);
    EXPECT_FALSE(nullptr != del);

    bool t = static_cast<bool>(del);
    EXPECT_FALSE(t);

    delegate<void()> del2(nullptr);
    EXPECT_FALSE(del2);

    delegate<void()> del3(0);
    EXPECT_FALSE(del3);

    struct Functor
    {
        void operator()() {}
    };

    Functor f;
    del.set<Functor>(f);

    EXPECT_TRUE(del);

    EXPECT_FALSE(del == nullptr);
    EXPECT_FALSE(nullptr == del);

    EXPECT_TRUE(del != nullptr);
    EXPECT_TRUE(nullptr != del);

    bool t2 = static_cast<bool>(del);
    EXPECT_TRUE(t2);

    del.clear();
    EXPECT_FALSE(del);

    del.set(f);
    EXPECT_TRUE(del);

    del = delegate<void()>{};
    EXPECT_FALSE(del);
}

struct MemberCheck
{
    int member(int i)
    {
        return i + 1;
    }
    int cmember(int i) const
    {
        return i + 2;
    }
};

struct Functor
{
    int operator()(int x)
    {
        return x + 3;
    };
    int operator()(int x) const
    {
        return x + 4;
    };

    // No const version available.
    void operator()(){};
};

TEST(delegate, Functor_const_variants_set)
{
    Functor f;
    const Functor cf;

    delegate<int(int)> del;
    del.set(f);
    EXPECT_EQ(del(2), 5);
    del.set(cf);
    EXPECT_EQ(del(2), 6);

    // Must not compile. require operator() const
    // delegate<void()> del2; del2.set(cf);

    // Must not compile. No storage of pointer to temporary.
    // del.set(Functor{});
    // del.set(const Functor{});
}

TEST(delegate, Functor_const_variants_make)
{
    Functor f;
    const Functor cf;

    delegate<int(int)> del;
    del = delegate<int(int)>::make(f);
    EXPECT_EQ(del(2), 5);
    del = delegate<int(int)>::make(cf);
    EXPECT_EQ(del(2), 6);

    // Short notation.
    auto del2 = del.make(f);
    EXPECT_EQ(del2(2), 5);

    // Must not compile. require operator() const
    // delegate<void()> del3 = delegate<void()>::make(cf);

    // Must not compile. No storage of pointer to temporary.
    // del.make(Functor{});
    // del.make(const Functor{});
}

TEST(delegate, Member_const_variants_set)
{
    MemberCheck mc;
    const MemberCheck cmc;

    delegate<int(int)> del;
    del.set<MemberCheck, &MemberCheck::member>(mc);
    int res = del(1);
    EXPECT_EQ(res, 2);

    // Must not compile. Need const member for const object.
    // del.set<MemberCheck, &MemberCheck::member>(cmc);

    del.set<MemberCheck, &MemberCheck::cmember>(mc);
    res = del(1);
    EXPECT_EQ(res, 3);

    del.set<MemberCheck, &MemberCheck::cmember>(cmc);
    res = del(1);
    EXPECT_EQ(res, 3);

    // Must not compile. Do not allow storing pointer to temporary.
    // del.set<MemberCheck, &MemberCheck::member>(MemberCheck{});
    // del.set<MemberCheck, &MemberCheck::cmember>(MemberCheck{});
    // del.set<MemberCheck, &MemberCheck::member>(const MemberCheck{});
    // del.set<MemberCheck, &MemberCheck::cmember>(const MemberCheck{});
}

#if __cplusplus >= 201703
TEST(delegate, Member_short_const_variants_set)
{
    MemberCheck mc;
    const MemberCheck cmc;

    delegate<int(int)> del;
    del.set<&MemberCheck::member>(mc);
    int res = del(1);
    EXPECT_EQ(res, 2);

    // Must not compile. Need const member for const object.
    // del.set<&MemberCheck::member>(cmc);

    del.set<&MemberCheck::cmember>(mc);
    res = del(1);
    EXPECT_EQ(res, 3);

    del.set<&MemberCheck::cmember>(cmc);
    res = del(1);
    EXPECT_EQ(res, 3);

    // Must not compile. Do not allow storing pointer to temporary.
    // del.set<&MemberCheck::member>(MemberCheck{});
    // del.set<&MemberCheck::cmember>(MemberCheck{});
    // del.set<&MemberCheck::member>(const MemberCheck{});
    // del.set<&MemberCheck::cmember>(const MemberCheck{});
}
#endif

TEST(delegate, Member_const_variants_make)
{
    MemberCheck mc;
    const MemberCheck cmc;

    delegate<int(int)> del =
        delegate<int(int)>::make<MemberCheck, &MemberCheck::member>(mc);
    EXPECT_EQ(del(1), 2);

    // Must not compile. Need const member for const object.
    // del.make<MemberCheck, &MemberCheck::member>(cmc);

    del = delegate<int(int)>::make<MemberCheck, &MemberCheck::cmember>(mc);
    EXPECT_EQ(del(1), 3);

    del = delegate<int(int)>::make<MemberCheck, &MemberCheck::cmember>(cmc);
    EXPECT_EQ(del(1), 3);

    // Must not compile. Do not allow storing pointer to temporary.
    //     del = delegate<int(int)>::make<MemberCheck,
    //     &MemberCheck::member>(MemberCheck{}); del =
    //     delegate<int(int)>::make<MemberCheck,
    //     &MemberCheck::cmember>(MemberCheck{}); del =
    //     delegate<int(int)>::make<MemberCheck, &MemberCheck::member>(const
    //     MemberCheck{}); del = delegate<int(int)>::make<MemberCheck,
    //     &MemberCheck::cmember>(const MemberCheck{});
}

#if __cplusplus >= 201703
TEST(delegate, Member_short_const_variants_make)
{
    MemberCheck mc;
    const MemberCheck cmc;

    delegate<int(int)> del = delegate<int(int)>::make<&MemberCheck::member>(mc);
    EXPECT_EQ(del(1), 2);

    // Must not compile. Need const member for const object.
    // del.make<MemberCheck, &MemberCheck::member>(cmc);

    del = delegate<int(int)>::make<&MemberCheck::cmember>(mc);
    EXPECT_EQ(del(1), 3);

    del = delegate<int(int)>::make<&MemberCheck::cmember>(cmc);
    EXPECT_EQ(del(1), 3);

    // Must not compile. Do not allow storing pointer to temporary.
    //     del = delegate<int(int)>::make<&MemberCheck::member>(MemberCheck{});
    //     del = delegate<int(int)>::make<&MemberCheck::cmember>(MemberCheck{});
    //     del = delegate<int(int)>::make<&MemberCheck::member>(const
    //     MemberCheck{}); del =
    //     delegate<int(int)>::make<&MemberCheck::cmember>(const MemberCheck{});
}
#endif

// Make sure we can store member function pointer with correct const
// correctness.
TEST(delegate, MemFkn_member_intermediate_storage)
{
    MemberCheck mc;
    const MemberCheck cmc;

    delegate<int(int)> del;

    mem_fkn<MemberCheck, false, int(int)> memberFkn;
    memberFkn.set<&MemberCheck::member>();

    // Must not compile. Do not want to deduce non const member fkn with true
    // cnst.
    // MemFkn<decltype(del), true> cmemberFkn{
    //    delegate<int(int)>::memFkn<MemberCheck, &MemberCheck::member>()};

    del.set(memberFkn, mc);
    int res = del(1);
    EXPECT_EQ(res, 2);

    del.clear();

    del = delegate<int(int)>::make(memberFkn, mc);
    res = del(1);
    EXPECT_EQ(res, 2);

    // Must not compile. Non const member fkn, const obj.
    // del.set(memberFkn, cmc);
    // del.set(memberFkn, MemberCheck{});
    // del = delegate<int(int)>::make(memberFkn, cmc);
    // del = delegate<int(int)>::make(memberFkn, MemberCheck{});

    auto cmemberFkn =
        mem_fkn<MemberCheck, true, int(int)>::make<&MemberCheck::cmember>();
    del.set(cmemberFkn, mc);
    res = del(1);
    EXPECT_EQ(res, 3);

    del.clear();

    del = delegate<int(int)>::make(cmemberFkn, mc);
    res = del(1);
    EXPECT_EQ(res, 3);

    del.set(cmemberFkn, cmc);
    res = del(1);
    EXPECT_EQ(res, 3);

    del = delegate<int(int)>::make(cmemberFkn, cmc);
    res = del(1);
    EXPECT_EQ(res, 3);

    // Must not compile. Const member fkn,
    // del.set(cmemberFkn, MemberCheck{});
    // del = delegate<int(int)>::make(cmemberFkn, MemberCheck{});

#if 0
    // Make sure convenience notation works.
    auto memberFkn3 = del.memFkn<MemberCheck, &MemberCheck::member>();
    del.set(memberFkn3, mc);
    res = del(1);
    EXPECT_EQ(res, 2);

    auto memberFkn4 = del.memFkn<MemberCheck, &MemberCheck::cmember>();
    del.set(memberFkn4, cmc);
    res = del(1);
    EXPECT_EQ(res, 3);
#endif
}

#if __cplusplus >= 201703
// Make sure we can store member function pointer with correct const
// correctness.
TEST(delegate, MemFkn_short_member_intermediate_storage)
{
    MemberCheck mc;
    const MemberCheck cmc;

    delegate<int(int)> del;
    mem_fkn<MemberCheck, false, int(int)> memberFkn;
    memberFkn.set<&MemberCheck::member>();

    // Must not compile. Do not want to deduce non const member fkn with true
    // cnst.
    // MemFkn<decltype(del), true> cmemberFkn{
    //    delegate<int(int)>::memFkn<&MemberCheck::member>()};

    del.set(memberFkn, mc);
    int res = del(1);
    EXPECT_EQ(res, 2);

    del.clear();

    del = delegate<int(int)>::make(memberFkn, mc);
    res = del(1);
    EXPECT_EQ(res, 2);

    mem_fkn<MemberCheck, true, int(int)> cmemberFkn;
    cmemberFkn.set<&MemberCheck::cmember>();

    del.set(cmemberFkn, mc);
    res = del(1);
    EXPECT_EQ(res, 3);

    del.set(cmemberFkn, cmc);
    res = del(1);
    EXPECT_EQ(res, 3);

    del.clear();

#if 0
    // Make sure convenience notation works.
    auto memberFkn3 = del.memFkn<&MemberCheck::member>();
    del.set(memberFkn3, mc);
    res = del(1);
    EXPECT_EQ(res, 2);

    auto memberFkn4 = del.memFkn<&MemberCheck::cmember>();
    del.set(memberFkn4, cmc);
    res = del(1);
    EXPECT_EQ(res, 3);
#endif
}

TEST(delegate, can_be_called_by_invoke)
{
    MemberCheck mc;
    const MemberCheck cmc;

    auto del = delegate<int(int)>::make<&MemberCheck::member>(mc);
    auto res = std::invoke(del, 1);
    EXPECT_EQ(res, 2);

    del.set<&MemberCheck::cmember>(cmc);
    EXPECT_EQ(std::invoke(del, 1), 3);
}

#endif

void
testFkn()
{
}

#if __cplusplus >= 201402L
#define CXX_14CONSTEXPR constexpr
#else
#define CXX_14CONSTEXPR
#endif

struct TestMember
{
    void member() {}
    void cmember() const {}
};
static TestMember tm;
static const TestMember ctm;

static Functor s_f;

struct CFunctor
{
    void operator()() const {};
};

static const CFunctor s_cf;

TEST(delegate, test_constexpr)
{
    // Default construct.
    auto constexpr del = delegate<void()>{};
    (void)del;

    // Set:
    // Free function
    auto CXX_14CONSTEXPR del2 = delegate<void()>{}.set<testFkn>();
    (void)del2;

    // Member function
    auto CXX_14CONSTEXPR del4 =
        delegate<void()>{}.set<TestMember, &TestMember::member>(tm);
    (void)del4;
    auto CXX_14CONSTEXPR del6 =
        delegate<void()>{}.set<TestMember, &TestMember::cmember>(ctm);
    (void)del6;

    // Functor
    auto CXX_14CONSTEXPR del8 = delegate<void()>{}.set(s_f);
    (void)del8;

    auto CXX_14CONSTEXPR del10 = delegate<void()>{}.set(s_cf);
    (void)del10;

    // mem_fkn
    auto constexpr memFkn =
        mem_fkn<TestMember, false, void()>::make<&TestMember::member>();
    auto constexpr memFkn2 =
        mem_fkn<TestMember, true, void()>::make<&TestMember::cmember>();

    auto CXX_14CONSTEXPR del12 = delegate<void()>{}.set(memFkn, tm);
    (void)del12;

    auto CXX_14CONSTEXPR del14 = delegate<void()>{}.set(memFkn2, ctm);
    (void)del14;

    // Make:
    // Free function
    auto constexpr del3 = delegate<void()>::make<testFkn>();
    (void)del3;

    // Member function
    auto constexpr del5 =
        delegate<void()>::make<TestMember, &TestMember::member>(tm);
    (void)del5;

    auto constexpr del7 =
        delegate<void()>::make<TestMember, &TestMember::cmember>(ctm);
    (void)del7;

    // Functor
    auto constexpr del9 = delegate<void()>::make(s_f);
    (void)del9;
    auto constexpr del11 = delegate<void()>::make(s_cf);
    (void)del11;

#if 0
    // MemFkn
    auto constexpr del13 = delegate<void()>::make(memFkn, s_f);
    (void)del13;
    auto constexpr del15 = delegate<void()>::make(memFkn2, s_cf);
    (void)del15;
#endif

    // Member function, short c++17 notation.
#if __cplusplus >= 201703
    auto constexpr del16 = delegate<void()>{}.set<&TestMember::member>(tm);
    (void)del16;
    auto constexpr del18 = delegate<void()>{}.set<&TestMember::cmember>(ctm);
    (void)del18;

    auto constexpr del17 = delegate<void()>::make<&TestMember::member>(tm);
    (void)del17;
    auto constexpr del19 = delegate<void()>::make<&TestMember::cmember>(ctm);
    (void)del19;

#if 0
    auto constexpr memFkn3 = delegate<void()>::memFkn<&TestMember::member>();
    (void)memFkn3;
    auto constexpr memFkn4 = delegate<void()>::memFkn<&TestMember::cmember>();
    (void)memFkn4;
#endif

#endif
}

struct Base
{
    virtual int memb(int i)
    {
        return i + 1;
    }
    virtual int cmemb(int i) const
    {
        return i + 2;
    }
};

struct Derived : public Base
{
    virtual int memb(int i)
    {
        return i + 3;
    }
    virtual int cmemb(int i) const
    {
        return i + 4;
    }
};

TEST(delegate, test_virtual_dispatch)
{
    Derived d;
    Base& b = d;

    delegate<int(int)> del;
    del.set<Base, &Base::memb>(b);
    EXPECT_EQ(del(1), 4);

    const Base& cb = d;
    del.set<Base, &Base::cmemb>(cb);
    // derived called.
    EXPECT_EQ(del(1), 5);
}

struct TestReturn
{
    ~TestReturn()
    {
        val++;
    }
    static int val;
};

int TestReturn::val = 0;

TEST(delegate, ensure_nullptr_return_default_constructed_object)
{
    // Default construct, Can call default constructed.
    delegate<void()> del;
    // And call.
    del();

    // Make sure we get a default constructed return value.
    delegate<TestReturn()> del2;

    EXPECT_EQ(TestReturn::val, 0);
    del2();
    EXPECT_EQ(TestReturn::val, 1);
}

TEST(delegate, ensure_return_type_is_the_correct_type)
{
    delegate<std::uint16_t(uint32_t)> cb;
    EXPECT_FALSE(cb);

    // delegate supports calls to a null delegate. will do nothing.
    // and return a default constructed value.
    auto t = cb(4);

    // try to deduce that we got a proper uint16_t type back.
    using ArgT = decltype(t);
    EXPECT_TRUE(std::numeric_limits<ArgT>::min() == 0);
    EXPECT_TRUE(std::numeric_limits<ArgT>::max() == 0xffff);
    EXPECT_TRUE(std::numeric_limits<ArgT>::is_signed == false);
    EXPECT_TRUE(std::numeric_limits<ArgT>::is_exact == true);
    EXPECT_TRUE(std::numeric_limits<ArgT>::is_integer == true);
    EXPECT_TRUE(t == 0);
}

TEST(delegate, testMemberDisambiguateConst)
{
    struct ConstCheck
    {
        int member(int i)
        {
            return i + 1;
        }
        int member(int i) const
        {
            return i + 2;
        }
    };

    ConstCheck obj;
    const ConstCheck cobj;

    delegate<int(int)> del;

    // Ambiguous name w.r.t. const. Disambiguate on object.
    del.set<ConstCheck, &ConstCheck::member>(obj);
    int res = del(1);
    EXPECT_EQ(res, 2);

    // Ambiguous name w.r.t. const. Disambiguate on object.
    del.set<ConstCheck, &ConstCheck::member>(cobj);
    res = del(1);
    EXPECT_EQ(res, 3);
}

TEST(delegate, test_lambda_support)
{
    struct Functor_
    {
        int operator()(int x, int y)
        {
            return x + y;
        }
    };

    Functor_ fkn;

    // Create simple callback object with operator().
    auto cb = delegate<int(int, int)>::make(fkn);
    EXPECT_EQ(cb(5, 3), 8);

    // Lambda should be similar.
    auto t = [](int x, int y) -> int { return x + y; };
    auto cb3 = delegate<int(int, int)>::make(t);
    EXPECT_EQ(cb3(5, 3), 8);
}

TEST(delegate, testLambdaConstFunctionConst2)
{
    struct FunctorC
    {
        int operator()(int x, int y) const
        {
            return x + y;
        }
    };

    const FunctorC fkn;
    const FunctorC fkn2 = fkn;
    (void)fkn2;

    // Create simple callback object with operator().
    auto cb = delegate<int(int, int)>::make(fkn);

    auto res = cb(5, 3);
    assert(res == 8);

    auto t = [](int x, int y) -> int { return x + y; };
    auto cb2 = delegate<int(int, int)>::make(t);

    res = cb2(6, 5);
    assert(res == 11);

    auto lambda = [](int x, int y) -> int { return x + y; };
    auto cb3 = delegate<int(int, int)>::make(lambda);

    res = cb3(6, 5);
    assert(res == 11);
}

TEST(delegate, Make_sure_our_union_is_a_void_size)
{
    delegate<int(int)> del;
    EXPECT_EQ(sizeof del, 2 * sizeof(void*));
    // If this fails, it could also be because sizeof fknptr != sizeof (void*)
}

#include <memory>

TEST(delegate, special_case_that_should_work_uniqueptr)
{
    // Make sure we can construct unique_ptr which keep tight control on making
    // copies of itself.
    delegate<std::unique_ptr<int>(int)> del;
    auto tmp = del(10);
    EXPECT_EQ(tmp, nullptr);

    auto t = [](int x) -> std::unique_ptr<int> // No make_unique in C++11.
    {
        std::unique_ptr<int> t2{new int};
        *t2 = x;
        return t2;
    };
    del.set(t);

    std::unique_ptr<int> up = del(12);
    EXPECT_EQ(*up, 12);
}

#include <set>

static int
freeFkn2(int x)
{
    return x + 6;
}

TEST(delegate, can_store_in_a_set)
{
    using Del = delegate<int(int)>;
    auto del = Del::make<freeFkn>();
    auto del2 = Del::make<freeFkn2>();
    EXPECT_NE(del, del2);
    EXPECT_NE(del.less(del, del2), del2.less(del2, del));

    std::set<Del, Del::Less> testSet;
    testSet.insert(Del::make<freeFkn>());
    testSet.insert(Del::make<freeFkn2>());
    EXPECT_EQ(testSet.size(), 2u);
}

struct TestObj
{
    TestObj() = default;
    TestObj(int x) : m_val(x) {}

    int m_val = 3;

    int add(int x)
    {
        return m_val + x;
    }

    int addc(int x) const
    {
        return m_val + x;
    }
};

int
adder(TestObj& o, int val)
{
    return o.m_val + val;
}

void
testFreeFunctionWithPtr()
{
    TestObj o;

    // Create simple callback to a normal free function.
    auto cb = delegate<int(int)>::make_free_with_object<TestObj, adder>(o);

    o.m_val = 6;
    int res = cb(3);
    assert(res == 9);

    o.m_val = 3;
    res = cb(9);
    assert(res == 12);
}

TEST(oldtests, testrun)
{
    testFreeFunctionWithPtr();
}

struct MCheck
{
    int member(int i)
    {
        return i + 1;
    }
    int cmember(int i) const
    {
        return i + 2;
    }
    int constcheck(int i)
    {
        return i + 1;
    }
    int constcheck(int i) const
    {
        return i + 2;
    }
};

TEST(mem_fkn, develop)
{
    MCheck mc;
    const MCheck cmc;

    mem_fkn<MCheck, false, int(int)> mf;

    mf = mf.make<&MCheck::member>();
    EXPECT_EQ(mf.invoke(mc, 1), 2);

    mf = mf.make_from_const<&MCheck::cmember>();
    EXPECT_EQ(mf.invoke(mc, 1), 3);

    mf = mf.make<&MCheck::member>();
    // EXPECT_EQ(mf.invoke(cmc, 1), 2);

    // Not ambiguous when taking address.
    mf = mf.make<&MCheck::constcheck>();
    EXPECT_EQ(mf.invoke(mc, 1), 2);

    mem_fkn<MCheck, true, int(int)> cmf;

    // cmf = cmf.make<&MemberCheck::member>();
    cmf = cmf.make<&MCheck::cmember>();
    EXPECT_EQ(cmf.invoke(mc, 1), 3);
    EXPECT_EQ(cmf.invoke(cmc, 1), 3);

    // Not ambiguous when taking address.
    cmf = cmf.make<&MCheck::constcheck>();
    EXPECT_EQ(cmf.invoke(mc, 1), 3);
    EXPECT_EQ(cmf.invoke(cmc, 1), 3);
}

TEST(mem_fkn, value_based)
{
    mem_fkn<MCheck, false, int(int)> mf;
    EXPECT_FALSE(mf);
    EXPECT_TRUE(!mf);
    EXPECT_TRUE(mf == mf);
    EXPECT_FALSE(mf != mf);
    EXPECT_FALSE(mf < mf);
    EXPECT_TRUE(mf <= mf);
    EXPECT_TRUE(mf >= mf);
    EXPECT_FALSE(mf > mf);

    mem_fkn<MCheck, false, int(int)> mf2 = mf2.make<&MCheck::member>();
    EXPECT_FALSE(!mf2);
    EXPECT_TRUE(mf2);

    EXPECT_TRUE(mf != mf2);
    EXPECT_FALSE(mf == mf2);
    EXPECT_TRUE(mf < mf2);
    EXPECT_TRUE(mf <= mf2);
    EXPECT_FALSE(mf >= mf2);
    EXPECT_FALSE(mf > mf2);

    EXPECT_TRUE(mf2 != mf);
    EXPECT_FALSE(mf2 == mf);
    EXPECT_FALSE(mf2 <= mf);
    EXPECT_FALSE(mf2 < mf);
    EXPECT_TRUE(mf2 > mf);
    EXPECT_TRUE(mf2 >= mf);

    mem_fkn<MCheck, false, int(int)> mf3 = mf2.make<&MCheck::constcheck>();

    EXPECT_TRUE(mf2 != mf3);
    EXPECT_FALSE(mf2 == mf3);
    EXPECT_FALSE(mf2 <= mf3 && mf2 >= mf3);
    EXPECT_TRUE(mf2 <= mf3 || mf2 >= mf3);
    EXPECT_FALSE(mf2 < mf3 && mf2 > mf3);
    EXPECT_TRUE(mf2 < mf3 || mf2 > mf3);
}

TEST(mem_fkn, value_based_const)
{
    mem_fkn<MCheck, true, int(int)> mf;
    EXPECT_FALSE(mf);
    EXPECT_TRUE(!mf);
    EXPECT_TRUE(mf == mf);
    EXPECT_FALSE(mf != mf);
    EXPECT_FALSE(mf < mf);
    EXPECT_TRUE(mf <= mf);
    EXPECT_TRUE(mf >= mf);
    EXPECT_FALSE(mf > mf);

    mem_fkn<MCheck, true, int(int)> mf2 = mf2.make<&MCheck::cmember>();
    EXPECT_FALSE(!mf2);
    EXPECT_TRUE(mf2);

    EXPECT_TRUE(mf != mf2);
    EXPECT_FALSE(mf == mf2);
    EXPECT_TRUE(mf < mf2);
    EXPECT_TRUE(mf <= mf2);
    EXPECT_FALSE(mf >= mf2);
    EXPECT_FALSE(mf > mf2);

    EXPECT_TRUE(mf2 != mf);
    EXPECT_FALSE(mf2 == mf);
    EXPECT_FALSE(mf2 <= mf);
    EXPECT_FALSE(mf2 < mf);
    EXPECT_TRUE(mf2 > mf);
    EXPECT_TRUE(mf2 >= mf);

    mem_fkn<MCheck, true, int(int)> mf3 = mf2.make<&MCheck::constcheck>();

    EXPECT_TRUE(mf2 != mf3);
    EXPECT_FALSE(mf2 == mf3);
    EXPECT_FALSE(mf2 <= mf3 && mf2 >= mf3);
    EXPECT_TRUE(mf2 <= mf3 || mf2 >= mf3);
    EXPECT_FALSE(mf2 < mf3 && mf2 > mf3);
    EXPECT_TRUE(mf2 < mf3 || mf2 > mf3);
}

// See that the set/make for aiding in reworking old C code works.
// Assume some driver (act as a service) which offer callbacks to be registered.
// Assume it offers a void* context pointer that will be passed on.
// In the case where a context pointer is not needed/unwanted a normal function
// pointer would work better. Do note it is required for member function calls
// anyway.
//
// The delegate should help with this workflow:
// 0. Make sure the driver / user code actually offers this context pointer in
// callbacks.
//    Get your C code to compile with C++ compiler.
// 1. Ensure the driver do not expose the void*/fkn pointer directly. Have a
// setter function
//    accepting void* / free function pointer.
// 2. Replace the raw function pointer / void* pointer in driver with  a
// delegate.
//    Set it in the setter function above. Use 'set_freefkn_with_void()' in
//    setter function. Let the driver call delegate instead of old pointers.
// 3. Possibly convert the driver to a class if it is not done yet.
// 4. Offer member function to register delegates. (expose internal
// delegate/offer to copy in
//    a full delegate.)
//
// At this point the driver is converted to a C++ class, while the user code
// Is still mostly C. You can now convert User code one at a time.
// Converting user code to classes can be done in 2 steps:
// 1. Convert the struct to a class, but keep external functions external for
// now.
// 2. Convert all usage of void* ctx into the proper class type. Use
//    'set_free_with_object' to get type safety in the callbacks.
// 3. Convert free functions to member functions as needed to enforce
// invariants.
//    Once a callback target becomes a member change registration to
//    a normal 'set' with member function name.

int
fknWithObject(MemberCheck& mc, int val)
{
    return mc.member(val);
}

int
fknWithConstObject(MemberCheck const& cmc, int val)
{
    return cmc.cmember(val);
}

int
fknWithVoid(void* ctx, int val)
{
    return static_cast<MemberCheck*>(ctx)->member(val);
}

int
fknWithConstVoid(void const* cctx, int val)
{
    return static_cast<MemberCheck const*>(cctx)->cmember(val);
}

TEST(delegate, with_void)
{
    MemberCheck mc;
    MemberCheck const cmc;

    using Del = delegate<int(int)>;
    Del del;

    del.set_free_with_void<fknWithVoid>(static_cast<void*>(&mc));
    EXPECT_EQ(del(1), 2);

    // del.set_free_with_void<fknWithVoid>(static_cast<const void*>(&cmc));
    // EXPECT_EQ(del(1), 2);

    del.set_free_with_void<fknWithConstVoid>(static_cast<const void*>(&mc));
    EXPECT_EQ(del(1), 3);

    del.set_free_with_void<fknWithConstVoid>(static_cast<void const*>(&cmc));
    EXPECT_EQ(del(1), 3);

    del = Del::make_free_with_void<fknWithVoid>(static_cast<void*>(&mc));
    EXPECT_EQ(del(1), 2);

    // del = Del::make_free_with_void<fknWithVoid>(static_cast<const
    // void*>(&cmc)); EXPECT_EQ(del(1), 2);

    del = Del::make_free_with_void<fknWithConstVoid>(
        static_cast<const void*>(&mc));
    EXPECT_EQ(del(1), 3);

    del = Del::make_free_with_void<fknWithConstVoid>(
        static_cast<void const*>(&cmc));
    EXPECT_EQ(del(1), 3);
}

TEST(delegate, with_object)
{
    MemberCheck mc;
    MemberCheck const cmc;

    using Del = delegate<int(int)>;
    Del del;

    del.set_free_with_object<MemberCheck, fknWithObject>(mc);
    EXPECT_EQ(del(1), 2);

    // del.set_free_with_object<MemberCheck, fknWithObject>(cmc);
    // EXPECT_EQ(del(1), 2);

    del.set_free_with_object<MemberCheck, fknWithConstObject>(cmc);
    EXPECT_EQ(del(1), 3);

    del.set_free_with_object<MemberCheck, fknWithConstObject>(cmc);
    EXPECT_EQ(del(1), 3);

    del = Del::make_free_with_object<MemberCheck, fknWithObject>(mc);
    EXPECT_EQ(del(1), 2);

    // del = Del::make_free_with_object<MemberCheck, fknWithObject>(cmc);
    // EXPECT_EQ(del(1), 2);

    del = Del::make_free_with_object<MemberCheck, fknWithConstObject>(cmc);
    EXPECT_EQ(del(1), 3);

    del = Del::make_free_with_object<MemberCheck, fknWithConstObject>(cmc);
    EXPECT_EQ(del(1), 3);
}

static int
testAdapter(const delegate<int(int)>::DataPtr& v, int val)
{
    int* p = static_cast<int*>(v.v_ptr);
    std::swap(*p, val);
    return val;
}

static delegate<int(int)>
make_exchange(int& store)
{
    return delegate<int(int)>{&testAdapter, static_cast<void*>(&store)};
}

TEST(delegate, use_extension)
{
    int t = 0;
    delegate<int(int)> del = make_exchange(t);
    t = 2;
    EXPECT_EQ(del(5), 2);
    EXPECT_EQ(t, 5);
}
