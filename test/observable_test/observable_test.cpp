/**
 * @file observable_test.cpp
 *
 * @author Ángel Fernández Pineda. Madrid. Spain. 2026.
 * @brief Publish-subscribe pattern
 * @date 2026-02-07
 *
 * @copyright EUPL 1.2 License
 */

#include "observable.hpp"
#include <cassert>
#include <iostream>

using namespace std;

//------------------------------------------------------------------------------
// Mocks
//------------------------------------------------------------------------------

struct Mock
{
    int expected = 0;
    int actual = 0;

    void member_callback(void *sender, const int &value)
    {
        actual = value;
    }

    bool check()
    {
        return actual == expected;
    }

} mock1, mock2;

struct Item
{
    int value = 0;
};

struct ItemMock
{
    int expected = 0;
    int actual = 0;

    void member_callback(void *sender, const Item &item)
    {
        actual = item.value;
    }

    bool check()
    {
        return actual == expected;
    }

} itemMock1;

//------------------------------------------------------------------------------
// Tests
//------------------------------------------------------------------------------

void test1()
{
    cout << "- Default constructors and assignments" << endl;
    {
        // move constructor
        observable<int> source{100};
        observable<int> dest{::std::move(source)};
        assert(dest == 100);
    }
    {
        // copy constructor
        observable<int> source{100};
        observable<int> dest{source};
        assert(dest == 100);
    }
    {
        // move-assignment
        observable<int> source{100};
        observable<int> dest{0};
        dest = ::std::move(source);
        assert(dest == 100);
    }
    {
        // copy-assignment
        observable<int> source{100};
        observable<int> dest{0};
        dest = source;
        assert(dest == 100);
    }
}

void test2()
{
    cout << "- Default constructors and assignments (readonly)" << endl;
    observable<int> private_source{100};
    observable<int> private_dest{0};
    {
        // move constructor
        observable<int>::readonly source{private_source};
        observable<int>::readonly dest{::std::move(source)};
        assert(dest == 100);
    }
    {
        // copy constructor
        observable<int> source{100};
        observable<int> dest{source};
        assert(dest == 100);
    }
    {
        // move-assignment
        observable<int> source{100};
        observable<int> dest{0};
        dest = ::std::move(source);
        assert(dest == 100);
    }
    {
        // copy-assignment
        observable<int> source{100};
        observable<int> dest{0};
        dest = source;
        assert(dest == 100);
    }
}

void test3()
{
    cout << "- T assignment and typecast -" << endl;
    observable<int> var{0};
    assert(var == 0);
    var.on_changing.subscribe(&Mock::member_callback, &mock1);
    var.on_change.subscribe(&Mock::member_callback, &mock2);

    var = 10;
    assert(var == 10);
    mock1.expected = 0;
    mock2.expected = 10;
    assert(mock1.check());
    assert(mock2.check());
}

void test4()
{
    // Note: mostly a compile-time test
    cout << "- T increment/decrement -" << endl;
    observable<int> var{0};
    var++;
    assert(var == 1);
    var--;
    assert(var == 0);
    ++var;
    assert(var == 1);
    --var;
    assert(var == 0);
}

void test5()
{
    // Note: mostly a compile-time test
    cout << "- T compound operators -" << endl;
    observable<int> var{0};
    var += 1;
    assert(var == 1);
    var -= 1;
    assert(var == 0);
    var = 10;
    var *= 10;
    assert(var == 100);
    var /= 10;
    assert(var == 10);
    var = 0b01;
    var |= 0b10;
    assert(var == 0b11);
    var &= 0b10;
    assert(var == 0b10);
    var = 10;
    var %= 3;
    assert(var == 1);
    var = 0b01;
    var ^= 0b10;
    assert(var == 0b11);
}

void test6()
{
    cout << "- observable::context -" << endl;
    Item initial_value{.value = 10};
    observable<Item> var(initial_value);
    var.on_change.subscribe(&ItemMock::member_callback, &itemMock1);
    var.on_changing.subscribe(&ItemMock::member_callback, &itemMock1);

    {
        auto ctx = var.with();
        itemMock1.expected = 10;
        assert(itemMock1.check());
        ctx->value = 20;
    }
    itemMock1.expected = 20;
    assert(itemMock1.check());

    {
        auto ctx = var.with();
        (*ctx).value = 30;
    }
    itemMock1.expected = 30;
    assert(itemMock1.check());
}

void test7()
{
    cout << "- observable::readonly -" << endl;
    observable<int> var;
    observable<int>::readonly var_ro{var};

    var = 10;
    assert(var_ro == 10);

    var_ro.on_change.subscribe(&Mock::member_callback, &mock1);
    var = 20;
    mock1.expected = 20;
    assert(mock1.check());
    assert(var_ro == 20);
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
    return 0;
}