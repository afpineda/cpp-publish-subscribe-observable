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

    void member_callback(void *sender, int value)
    {
        actual = value;
    }

    bool check()
    {
        return actual == expected;
    }

} mock1, mock2;

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

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------

int main()
{
    test1();
    test2();
    return 0;
}