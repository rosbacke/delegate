#include "delegate/delegate.hpp"

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
void testFkn12()
{ s_obj = 1;}

void testFkn12(int)
{ s_obj = 2; }



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
    MemFkn<decltype(del), false> memberFkn{
        delegate<int(int)>::memFkn<MemberCheck, &MemberCheck::member>()};

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

    MemFkn<decltype(del), true> cmemberFkn{
        delegate<int(int)>::memFkn<MemberCheck, &MemberCheck::cmember>()};
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

    // Make sure convenience notation works.
    auto memberFkn3 = del.memFkn<MemberCheck, &MemberCheck::member>();
    del.set(memberFkn3, mc);
    res = del(1);
    EXPECT_EQ(res, 2);

    auto memberFkn4 = del.memFkn<MemberCheck, &MemberCheck::cmember>();
    del.set(memberFkn4, cmc);
    res = del(1);
    EXPECT_EQ(res, 3);
}

#if __cplusplus >= 201703
// Make sure we can store member function pointer with correct const
// correctness.
TEST(delegate, MemFkn_short_member_intermediate_storage)
{
    MemberCheck mc;
    const MemberCheck cmc;

    delegate<int(int)> del;
    MemFkn<decltype(del), false> memberFkn{
        delegate<int(int)>::memFkn<&MemberCheck::member>()};

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

    MemFkn<decltype(del), true> cmemberFkn{
        delegate<int(int)>::memFkn<&MemberCheck::cmember>()};
    del.set(cmemberFkn, mc);
    res = del(1);
    EXPECT_EQ(res, 3);

    del.clear();

    // Make sure convenience notation works.
    auto memberFkn3 = del.memFkn<&MemberCheck::member>();
    del.set(memberFkn3, mc);
    res = del(1);
    EXPECT_EQ(res, 2);

    auto memberFkn4 = del.memFkn<&MemberCheck::cmember>();
    del.set(memberFkn4, cmc);
    res = del(1);
    EXPECT_EQ(res, 3);
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

    // Set:
    // Free function
    auto CXX_14CONSTEXPR del2 = delegate<void()>{}.set<testFkn>();

    // Member function
    auto CXX_14CONSTEXPR del4 =
        delegate<void()>{}.set<TestMember, &TestMember::member>(tm);
    auto CXX_14CONSTEXPR del6 =
        delegate<void()>{}.set<TestMember, &TestMember::cmember>(ctm);

    // Functor
    auto CXX_14CONSTEXPR del8 = delegate<void()>{}.set(s_f);
    auto CXX_14CONSTEXPR del10 = delegate<void()>{}.set(s_cf);

    // MemFkn
    auto constexpr memFkn =
        delegate<void()>{}.memFkn<TestMember, &TestMember::member>();
    auto constexpr memFkn2 =
        delegate<void()>{}.memFkn<TestMember, &TestMember::cmember>();
    auto CXX_14CONSTEXPR del12 = delegate<void()>{}.set(memFkn, s_f);
    auto CXX_14CONSTEXPR del14 = delegate<void()>{}.set(memFkn2, s_cf);

    // Make:
    // Free function
    auto constexpr del3 = delegate<void()>::make<testFkn>();

    // Member function
    auto constexpr del5 =
        delegate<void()>::make<TestMember, &TestMember::member>(tm);
    auto constexpr del7 =
        delegate<void()>::make<TestMember, &TestMember::cmember>(ctm);

    // Functor
    auto constexpr del9 = delegate<void()>::make(s_f);
    auto constexpr del11 = delegate<void()>::make(s_cf);

    // MemFkn
    auto constexpr del13 = delegate<void()>::make(memFkn, s_f);
    auto constexpr del15 = delegate<void()>::make(memFkn2, s_cf);

    // Member function, short c++17 notation.
#if __cplusplus >= 201703
    auto constexpr del16 = delegate<void()>{}.set<&TestMember::member>(tm);
    auto constexpr del18 = delegate<void()>{}.set<&TestMember::cmember>(ctm);

    auto constexpr del17 = delegate<void()>::make<&TestMember::member>(tm);
    auto constexpr del19 = delegate<void()>::make<&TestMember::cmember>(ctm);

    auto constexpr memFkn3 = delegate<void()>::memFkn<&TestMember::member>();
    auto constexpr memFkn4 = delegate<void()>::memFkn<&TestMember::cmember>();
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
        std::unique_ptr<int> t{new int};
        *t = x;
        return t;
    };
    del.set(t);

    std::unique_ptr<int> up = del(12);
    EXPECT_EQ(*up, 12);
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
    auto cb = delegate<int(int)>::make<TestObj, adder>(o);

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
