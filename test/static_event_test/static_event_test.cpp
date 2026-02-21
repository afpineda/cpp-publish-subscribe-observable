/**
 * @file static_event_test.cpp
 *
 * @author Ángel Fernández Pineda. Madrid. Spain. 2026.
 * @brief Publish-subscribe pattern
 * @date 2026-02-21
 *
 * @copyright EUPL 1.2 License
 */

#include "static_event.hpp"
#include <cassert>
#include <iostream>

using namespace std;

//------------------------------------------------------------------------------
// Mocks
//------------------------------------------------------------------------------

static int mock1_witness = 0;
static int mock2_witness = 0;
static bool mock3_witness = false;
static bool mock4_witness = false;

void reset_mock_witnesses()
{
    mock1_witness = 0;
    mock2_witness = 0;
    mock3_witness = false;
    mock4_witness = false;
}

void mock1(int n)
{
    mock1_witness = n;
}

void mock2(int n)
{
    mock2_witness = n;
}

void mock3()
{
    mock3_witness = true;
}

void mock4()
{
    mock4_witness = true;
}

//------------------------------------------------------------------------------
// Tests
//------------------------------------------------------------------------------

void test1()
{
    cout << "- Test mock's witnesses -" << endl;
    reset_mock_witnesses();
    assert(mock1_witness == 0);
    assert(mock2_witness == 0);
    assert(!mock3_witness);
    assert(!mock4_witness);

    reset_mock_witnesses();
    mock1(10);
    assert(mock1_witness == 10);
    assert(mock2_witness == 0);
    assert(!mock3_witness);
    assert(!mock4_witness);

    reset_mock_witnesses();
    mock2(20);
    assert(mock1_witness == 0);
    assert(mock2_witness == 20);
    assert(!mock3_witness);
    assert(!mock4_witness);

    reset_mock_witnesses();
    mock3();
    assert(mock1_witness == 0);
    assert(mock2_witness == 0);
    assert(mock3_witness);
    assert(!mock4_witness);

    reset_mock_witnesses();
    mock4();
    assert(mock1_witness == 0);
    assert(mock2_witness == 0);
    assert(!mock3_witness);
    assert(mock4_witness);
}

void test2()
{
    cout << "- Subscribe dynamically -" << endl;
    static_event<int> evt;
    reset_mock_witnesses();
    evt += mock1;
    evt += mock2;

    evt(5);
    assert(mock1_witness == 5);
    assert(mock2_witness == 5);
}

void test3()
{
    cout << "- Subscribe via initializer_list -" << endl;
    static_event<> evt({mock3, mock4});
    reset_mock_witnesses();

    evt();
    assert(mock3_witness);
    assert(mock4_witness);
}

void test4()
{
    cout << "- Copy and move -" << endl;
    {
        // copy assignment
        static_event source;
        static_event dest;
        source.subscribe(mock3);
        dest = source;
        assert(source == dest);
    }
    {
        // Copy constructor
        static_event source;
        source.subscribe(mock3);
        static_event dest{source};
        assert(source == dest);
    }
    {
        // move assignment
        static_event source;
        static_event dest;
        source.subscribe(mock3);
        assert(source.subscribed() == 1);
        assert(dest.subscribed() == 0);

        dest = ::std::move(source);
        assert(!(source == dest));
        assert(source.subscribed()==0);
    }
    {
        // move constructor
        static_event source;
        source.subscribe(mock3);
        assert(source.subscribed() == 1);

        static_event dest{::std::move(source)};
        assert(!(source == dest));
        assert(source.subscribed()==0);
    }
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
    return 0;
}