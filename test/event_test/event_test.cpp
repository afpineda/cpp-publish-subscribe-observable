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

//------------------------------------------------------------------------------
// Tests
//------------------------------------------------------------------------------

void test1()
{
    cout << "- Subscribe/unsubscribe non-member function -" << endl;
    event evt;
    assert(!evt.is_subscribed(Mock::class_callback));
    evt += Mock::class_callback;
    assert(evt.is_subscribed(Mock::class_callback));
    evt -= Mock::class_callback;
    assert(!evt.is_subscribed(Mock::class_callback));
}

void test2()
{
    cout << "- Subscribe/unsubscribe member function -" << endl;
    event evt;
    assert(!evt.is_subscribed(&Mock::member_callback, &mock1));
    assert(!evt.is_subscribed(&Mock::member_callback, &mock2));

    evt.subscribe(&Mock::member_callback, &mock1);
    assert(evt.is_subscribed(&Mock::member_callback, &mock1));
    assert(!evt.is_subscribed(&Mock::member_callback, &mock2));

    evt.subscribe(&Mock::member_callback, &mock2);
    assert(evt.is_subscribed(&Mock::member_callback, &mock1));
    assert(evt.is_subscribed(&Mock::member_callback, &mock2));

    evt.unsubscribe(&Mock::member_callback, &mock1);
    assert(!evt.is_subscribed(&Mock::member_callback, &mock1));
    assert(evt.is_subscribed(&Mock::member_callback, &mock2));

    evt.unsubscribe(&Mock::member_callback, &mock2);
    assert(!evt.is_subscribed(&Mock::member_callback, &mock1));
    assert(!evt.is_subscribed(&Mock::member_callback, &mock2));
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
    evt += Mock::class_callback;
    evt += Mock::class_callback;
    evt += Mock::class_callback;
    assert(evt.subscribed() == 1);

    Mock::class_clear();
    assert(!Mock::class_executed);
    assert(Mock::class_executed_counter == 0);

    evt();
    assert(Mock::class_executed_counter == 1);

    evt -= Mock::class_callback;
    assert(evt.subscribed() == 0);
}

void test5()
{
    cout << "- Unsubscribe not subscribed -" << endl;
    event evt;
    assert(evt.subscribed() == 0);
    assert(!evt.is_subscribed(Mock::class_callback));

    evt -= Mock::class_callback;
    assert(evt.subscribed() == 0);
    assert(!evt.is_subscribed(Mock::class_callback));
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
    return 0;
}