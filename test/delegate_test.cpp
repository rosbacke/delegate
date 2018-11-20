#include "delegate/delegate.hpp"

#include <cassert>
#include <cstdint>
#include <limits>

#include <gtest/gtest.h>

#if __cplusplus < 201103L
#error "Require at least C++11 to compile delegate"
#endif

TEST(Test_framework, test1)
{
    EXPECT_TRUE(true);
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

TEST(delegate, construction1)
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

TEST(delegate, nulltests)
{
    // Default construct, Can call default constructed.
    delegate<void()> del;

    EXPECT_FALSE(del);

    EXPECT_TRUE(del == nullptr);
    EXPECT_TRUE(nullptr == del);

    EXPECT_FALSE(del != nullptr);
    EXPECT_FALSE(nullptr != del);

    bool t = del;
    EXPECT_FALSE(t);

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

    bool t2 = del;
    EXPECT_TRUE(t2);

    del.clear();
    EXPECT_FALSE(del);

    del.set(f);
    EXPECT_TRUE(del);

    del = delegate<void()>{};
    EXPECT_FALSE(del);
}

TEST(delegate, Functor_const_variants)
{
    struct Functor
    {
        void operator()(){};
    };

    Functor f;
    delegate<void()> del;
    del.set(f);
    del();
    del.set<Functor>(f);
    del();

    // Must not compile. require operator() const
    const Functor f2;
    (void)f2;
    // del.set(f2);
    // del.set<Functor>(f2);

    // Must not compile. No storage of pointer to temporary.
    // del.set<Functor>(Functor{});
    // del.set(Functor{});

    // Test with const.
    struct cFunctor
    {
        void operator()() const {};
    };

    // Ok, got operator() const
    cFunctor f3;
    del.set<cFunctor>(f3);
    del();
    del.set(f3);
    del();

    const cFunctor f4;
    del.set<cFunctor>(f4);
    del();
    del.set(f4);
    del();

    // Must not compile. No storage of pointer to temporary.
    // del.set<Functor2>(Functor2{});
    // del.set(Functor2{});
}

TEST(delegate, Member_const_variants)
{
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
}

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

struct Functor
{
    void operator()(){};
};

static Functor s_f;

struct CFunctor
{
    void operator()() const {};
};

static const CFunctor s_cf;

TEST(delegate, test_constexpr)
{
    auto constexpr del = delegate<void()>{};

    auto CXX_14CONSTEXPR del2 = delegate<void()>{}.set<testFkn>();
    auto CXX_14CONSTEXPR del4 =
        delegate<void()>{}.set<TestMember, &TestMember::member>(tm);
    auto CXX_14CONSTEXPR del6 =
        delegate<void()>{}.set<TestMember, &TestMember::cmember>(ctm);
    auto CXX_14CONSTEXPR del8 = delegate<void()>{}.set(s_f);
    auto CXX_14CONSTEXPR del10 = delegate<void()>{}.set(s_cf);

    auto constexpr del3 = delegate<void()>::make<testFkn>();
    auto constexpr del5 =
        delegate<void()>::make<TestMember, &TestMember::member>(tm);
    auto constexpr del7 =
        delegate<void()>::make<TestMember, &TestMember::cmember>(ctm);
    auto constexpr del9 = delegate<void()>::make(s_f);
    auto constexpr del11 = delegate<void()>::make(s_cf);
}

void
test_construction()
{
    delegate<void()> cb;

    // Make sure default constructed tests as false. (similar to normal
    // pointers.)
    assert(!cb);

    // Make sure default constructed tests as equal to nullptr;
    assert(cb == nullptr);

    assert(nullptr == cb);

    // Ensure copy construction.
    auto cb2 = cb;

    // Ensure assignment.
    cb2 = cb;

    assert(cb2 == cb);

    assert(cb2 == false);

    // Make sure default constructed tests as equal to nullptr;
    assert(cb2 == nullptr);
}

void
test_nullable()
{
    delegate<void()> cb;
    assert(!cb);
    cb(); // delegate supports calls to a null delegate. will do nothing.

    delegate<std::uint16_t(uint32_t)> cb2;
    assert(!cb2);

    // delegate supports calls to a null delegate. will do nothing.
    // and return a default constructed value.
    auto t = cb2(4);

    // try to deduce that we got a proper uint16_t type back.
    using ArgT = decltype(t);
    assert(std::numeric_limits<ArgT>::min() == 0);
    assert(std::numeric_limits<ArgT>::max() == 0xffff);
    assert(std::numeric_limits<ArgT>::is_signed == false);
    assert(std::numeric_limits<ArgT>::is_exact == true);
    assert(std::numeric_limits<ArgT>::is_integer == true);
    assert(t == 0);
}

static int
testAdd(int x, int y)
{
    return x + y;
}

static int
testDiff(int x, int y)
{
    return x - y;
}

void
testFreeFunction()
{
    // Create simple callback to a normal free function.
    auto cb = delegate<int(int, int)>::make<testAdd>();

    // Try making a call. Check result.
    int res = cb(2, 3);
    assert(res == 5);

    // Test copy constructor.
    auto cb2(cb);
    int res2 = cb2(3, 4);
    assert(res2 == 7);

    // Test assign.
    cb = delegate<int(int, int)>::make<testDiff>();
    res = cb(5, 2);
    assert(res == 3);

    cb = cb;
    res = cb(5, 2);
    assert(res == 3);
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
adder(TestObj* o, int val)
{
    return o->m_val + val;
}

void
testFreeFunctionWithPtr()
{
    TestObj o;

    // Create simple callback to a normal free function.
    auto cb = delegate<int(int)>::makeFreeCBWithPtr<TestObj*, adder>(&o);

    o.m_val = 6;
    int res = cb(3);
    assert(res == 9);

    o.m_val = 3;
    res = cb(9);
    assert(res == 12);
}

void
testMemberFunction()
{
    TestObj o;

    // Create member function callback.
    auto cb = delegate<int(int)>::make<TestObj, &TestObj::add>(o);

    o.m_val = 6;
    int res = cb(3);
    assert(res == 9);

    o.m_val = 3;
    res = cb(9);
    assert(res == 12);

    // Try member construction.
    delegate<int(int)> cb2;
    cb2.set<TestObj, &TestObj::add>(o);

    o.m_val = 6;
    res = cb2(3);
    assert(res == 9);

    o.m_val = 3;
    res = cb2(9);
    assert(res == 12);
}

TEST(delegate, testMemberFunctionConst)
{
    const TestObj o(6);

    // Create member function callback.
    auto cb = delegate<int(int)>::make<TestObj, &TestObj::addc>(o);

    int res = cb(3);
    assert(res == 9);

    res = cb(9);
    assert(res == 15);

    // Try member construction.
    delegate<int(int)> cb2;
    cb2.set<TestObj, &TestObj::addc>(o);

    res = cb2(3);
    assert(res == 9);

    res = cb2(9);
    assert(res == 15);

    // Try const member functions on non const obj.
}

TEST(delegate, testMemberFunctionConst2)
{
    TestObj o(6);

    // Create member function callback.
    auto cb = delegate<int(int)>::make<TestObj, &TestObj::addc>(o);

    int res = cb(3);
    assert(res == 9);

    res = cb(9);
    assert(res == 15);

    // Try member construction.
    delegate<int(int)> cb2;
    cb2.set<TestObj, &TestObj::addc>(o);

    res = cb2(3);
    assert(res == 9);

    res = cb2(9);
    assert(res == 15);

    // Try const member functions on non const obj.
}

void
testLambdaFunction()
{
    struct Functor
    {
        int operator()(int x, int y)
        {
            return x + y;
        }
    };

    Functor fkn;

    // Create simple callback object with operator().
    auto cb = delegate<int(int, int)>::make(fkn);

    // Can I copy?
    auto cb2 = cb;

    auto res = cb2(5, 3);
    assert(res == 8);

    auto t = [](int x, int y) -> int { return x + y; };

    auto cb3 = delegate<int(int, int)>::make(t);

    res = cb3(6, 5);
    assert(res == 11);

    auto lambda = [](int x, int y) -> int { return x + y; };
    auto cb4 = delegate<int(int, int)>::make(lambda);

    res = cb4(6, 5);
    assert(res == 11);
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

    res = cb2(6, 5);
    assert(res == 11);
}

TEST(oldtests, testrun)
{
    test_construction();
    test_nullable();
    testFreeFunction();
    testFreeFunctionWithPtr();
    testMemberFunction();
    testLambdaFunction();
}
