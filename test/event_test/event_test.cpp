/**
 * @file event_test.cpp
 *
 * @author Ángel Fernández Pineda. Madrid. Spain. 2026.
 * @brief Publish-subscribe pattern
 * @date 2026-02-07
 *
 * @copyright EUPL 1.2 License
 */

#include "event.hpp"
#include <cassert>
#include <iostream>

using namespace std;

//------------------------------------------------------------------------------
// Mocks
//------------------------------------------------------------------------------

struct Mock
{
    bool executed = false;
    inline static bool class_executed = false;
    inline static int class_executed_counter = 0;

    void clear()
    {
        executed = false;
    }

    static void class_clear()
    {
        class_executed = false;
        class_executed_counter = 0;
    }

    void member_callback()
    {
        executed = true;
    }

    static void class_callback()
    {
        class_executed = true;
        class_executed_counter++;
    }
} mock1, mock2;

struct IntDoubleMock
{
    int a{};
    double b{};
    void member_callback(int a, const double &b)
    {
        this->a = a;
        this->b = b;
    }
};

//------------------------------------------------------------------------------
// Tests
//------------------------------------------------------------------------------

void test1()
{
    cout << "- Subscribe/unsubscribe non-member function -" << endl;
    event evt;
    auto sh = evt.subscribe(Mock::class_callback);
    assert(sh.is_subscribed());
    evt.unsubscribe(sh);
    assert(!sh.is_subscribed());
}

void test2()
{
    cout << "- Subscribe/unsubscribe member function -" << endl;
    event evt;

    auto sh1 = evt.subscribe(&Mock::member_callback, &mock1);
    assert(sh1.is_subscribed());

    auto sh2 = evt.subscribe(&Mock::member_callback, &mock2);
    assert(sh1.is_subscribed());
    assert(sh2.is_subscribed());

    evt.unsubscribe(sh1);
    assert(!sh1.is_subscribed());
    assert(sh2.is_subscribed());

    evt.unsubscribe(sh2);
    assert(!sh1.is_subscribed());
    assert(!sh2.is_subscribed());
}

void test3()
{
    cout << "- Run callbacks -" << endl;
    event evt;
    mock1.clear();
    mock2.clear();
    Mock::class_clear();
    assert(!Mock::class_executed);
    assert(!mock1.executed);
    assert(!mock2.executed);

    evt.subscribe(&Mock::member_callback, &mock1);
    evt.subscribe(Mock::class_callback);
    evt.subscribe(&Mock::member_callback, &mock2);
    assert(evt.subscribed() == 3);

    evt();
    assert(Mock::class_executed);
    assert(mock1.executed);
    assert(mock2.executed);

    evt.clear();
    assert(evt.subscribed() == 0);
}

void test4()
{
    cout << "- Subscribe/unsubscribe twice or more times -" << endl;
    event evt;
    auto sh1 = evt.subscribe(Mock::class_callback);
    auto sh2 = evt.subscribe(Mock::class_callback);
    auto sh3 = evt.subscribe(Mock::class_callback);
    assert(evt.subscribed() == 3);
    assert(sh1.is_subscribed());
    assert(sh2.is_subscribed());
    assert(sh3.is_subscribed());

    Mock::class_clear();
    assert(!Mock::class_executed);
    assert(Mock::class_executed_counter == 0);
    evt();
    assert(Mock::class_executed_counter == 3);

    evt.unsubscribe(sh2);
    assert(evt.subscribed() == 2);
    Mock::class_clear();
    evt();
    assert(Mock::class_executed_counter == 2);

    evt.unsubscribe(sh3);
    assert(evt.subscribed() == 1);
    Mock::class_clear();
    evt();
    assert(Mock::class_executed_counter == 1);

    evt.unsubscribe(sh1);
    evt.unsubscribe(sh1);
    evt.unsubscribe(sh1);
    assert(!sh1.is_subscribed());
    assert(evt.subscribed() == 0);
    Mock::class_clear();
    evt();
    assert(Mock::class_executed_counter == 0);
}

void test5()
{
    cout << "- Unsubscribe not subscribed -" << endl;
    event evt1;
    event evt2;
    assert(evt1.subscribed() == 0);
    assert(evt2.subscribed() == 0);

    auto sh = evt1.subscribe(Mock::class_callback);
    evt2.unsubscribe(sh);
    assert(evt1.subscribed() == 1);
    assert(evt2.subscribed() == 0);
    assert(sh.is_subscribed());

    event<>::subscription_handler orphan;
    assert(!orphan.is_subscribed());
    evt1.unsubscribe(orphan);
    assert(evt1.subscribed() == 1);
    assert(!orphan.is_subscribed());
}

void test6()
{
    cout << "- Copy and move -" << endl;
    {
        // copy assignment
        event source;
        event dest;
        source.subscribe(Mock::class_callback);
        dest = source;
        assert(source.subscribed() == 1);
        assert(dest.subscribed() == 1);
    }
    {
        // Copy constructor
        event source;
        source.subscribe(Mock::class_callback);
        event dest{source};
        assert(source.subscribed() == 1);
        assert(dest.subscribed() == 1);
    }
    {
        // move assignment
        event source;
        event dest;
        source.subscribe(Mock::class_callback);
        assert(source.subscribed() == 1);
        assert(dest.subscribed() == 0);

        dest = ::std::move(source);
        assert(source.subscribed() == 0);
        assert(dest.subscribed() == 1);
    }
    {
        // move constructor
        event source;
        source.subscribe(Mock::class_callback);
        assert(source.subscribed() == 1);

        event dest{::std::move(source)};
        assert(source.subscribed() == 0);
        assert(dest.subscribed() == 1);
    }
}

void test7()
{
    // NOTE: mostly a compile-time test
    // to check template parameter expansion
    cout << "- Member function binding with two template parameters -" << endl;
    event<int, double> evt;
    IntDoubleMock x;
    evt.subscribe(&IntDoubleMock::member_callback, &x);
    evt(10, 10.0);
    assert(x.a == 10);
    assert(x.b == 10.0);
}

void test8()
{
    cout << "- Lambda subscription/unsubscription without captures -" << endl;
    event evt;
    auto lambda1 = []()
    {
        cout << "ignore this" << endl;
    };
    auto lambda2 = []()
    {
        cout << "ignore this again" << endl;
    };

    auto sh1 = evt.subscribe(lambda1);
    assert(sh1.is_subscribed());
    assert(evt.subscribed() == 1);
    auto sh2 = evt.subscribe(lambda2);
    assert(sh2.is_subscribed());
    assert(evt.subscribed() == 2);

    evt.unsubscribe(sh2);
    assert(evt.subscribed() == 1);
    evt.unsubscribe(sh1);
    assert(evt.subscribed() == 0);
}

void test9()
{
    cout << "- Lambda subscription/unsubscription with captures -" << endl;
    event<int> evt;
    int x1 = 0;
    int x2 = 0;
    auto lambda1 = [&x1](const int &value)
    {
        x1 = value;
    };
    auto lambda2 = [&x2](const int &value)
    {
        x2 = value;
    };

    auto sh1 = evt.subscribe(lambda1);
    assert(sh1.is_subscribed());
    assert(evt.subscribed() == 1);
    auto sh2 = evt.subscribe(lambda2);
    assert(sh2.is_subscribed());
    assert(evt.subscribed() == 2);

    evt(1);
    assert(x1 == 1);
    assert(x2 == 1);

    evt.unsubscribe(sh2);
    assert(evt.subscribed() == 1);
    evt(2);
    assert(x1 == 2);
    assert(x2 == 1);

    evt.unsubscribe(sh1);
    assert(evt.subscribed() == 0);
    evt(3);
    assert(x1 == 2);
    assert(x2 == 1);
}

void test10()
{
    cout << "- Subscribing/unsubscribing with ::std::bind() -" << endl;
    event evt;

    auto sh1 = evt.subscribe(::std::bind(&Mock::member_callback, &mock1));
    assert(sh1.is_subscribed());

    auto sh2 = evt.subscribe(::std::bind(&Mock::member_callback, &mock2));
    assert(sh1.is_subscribed());
    assert(sh2.is_subscribed());

    evt.unsubscribe(sh1);
    assert(!sh1.is_subscribed());
    assert(sh2.is_subscribed());

    evt.unsubscribe(sh2);
    assert(!sh1.is_subscribed());
    assert(!sh2.is_subscribed());
}

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------

int main()
{
    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
    test7();
    test8();
    test9();
    test10();
    return 0;
}