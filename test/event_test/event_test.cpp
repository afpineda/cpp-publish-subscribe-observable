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

    void clear()
    {
        executed = false;
        class_executed = false;
    }

    void member_callback()
    {
        executed = true;
    }

    static void class_callback()
    {
        class_executed = true;
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

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------

int main()
{
    test1();
    test2();
    return 0;
}