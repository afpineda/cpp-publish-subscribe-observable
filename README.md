# A modern C++ library implementing the publish-subscribe and observable patterns

This repository holds a header-only C++ library
implementing two design patterns:

- **Publish-subscribe**:

  A *publisher* declares an *event* having a number of parameters.
  An arbitrary number of *subscribers* hook a *callback* function to that event.
  Each time the publisher dispatches the event,
  all subscribed callbacks are executed.

- **Observable**:

  A *publisher* declares an **observable variable** (aka "property"),
  which behaves as an ordinary variable.
  However, a *publish-subscribe event* is dispatched every time
  the variable gets written or modified.

  This library supports two kinds of *observables*:
  - Generally readable and writable, as any other variable.
  - Publicly readable and privately writable,
    which is an usual usage pattern.

## Publish-subscribe pattern

Declare events using the `event` class template
with an arbitrary number of parameters.
For instance:

```c++
event<int,const std::string &> on_message;
```

Subscribe and unsubscribe using the `+=` and `-=` operators
or the `subscribe()` and `unsubscribe()` methods
(all of them are thread-safe).
Many callbacks can subscribe to the same event.

For instance:

```c++
void on_message_callback(int id, const std::string &text)
{
    ...
}

void debug_log(int id, const std::string &text)
{
    ...
}
...
on_message += on_message_callback;
on_message += debug_log;
```

Events are dispatched using the function call operator. For instance:

```c++
on_message(27,"test message");
```

All subscribed callbacks (if any) will be executed in subscription order.

### Member function callbacks

**Do not** use `std::bind()` to subscribe member functions.
**It does not work**.
Use the provided `subscribe()` and `unsubscribe()` methods that take the
same arguments as `std::bind()`. For instance:

```c++
struct MyClass
{
    void on_message_callback(int id, const std::string &text)
    {
        ...
    }
} instance1,instance2;
...
event<int,const std::string &> on_message;
on_message.subscribe(&MyClass:on_message_callback, &instance1);
on_message.subscribe(&MyClass:on_message_callback, &instance2);
```

### RAII idiom

Treat subscription as resource allocation and apply the
[RAII idiom](https://en.wikipedia.org/wiki/Resource_acquisition_is_initialization).
Subscribed callbacks **must** unsubscribe before they get deallocated.

## Observable pattern

Each observable variable holds two subscribable events:

- `on_changing`: dispatched every time the observable is about to change.
- `on_change`: dispatched every time the observable is written or modified.

More details below.

### Generally-writable observables

Use the `observable` class template to declare an observable variable.
Observable variables are backed by a private variable of the given type.
For instance:

```c++
class Example{
public:
    observable<int> property{};
} instance;
```

`observable` instances are implicitly casted to the given type.
Some examples:

```c++
// Read example
std::cout << "Initial value: " << instance.property << std::endl;
// Write/modify examples
instance.property = 10000;
++instance.property;
instance.property++;
instance.property += 10;
instance.property -= 10;
instance.property *= 10;
instance.property /= 10;
instance.property %= 5;
instance.property ^= 2;
instance.property |= 1;
instance.property &= 1;
```

### Publicly-readable, privately-writable observables

Use the `observable::readonly` subtype to declare
an observable variable for read only.
Read-only observables are backed by a regular `observable` variable
which should be kept private to allow changes from setter methods.
For instance:

```c++
class Example2
{
private:
    observable<int> private_property{};

public:
    observable<int>::readonly property{private_property};

    void setter(int value)
    {
        // Example of private writing
        if (value > 0)
            private_property = value;
    }
} instance2;
```

`readonly` instances do not have assignment,
increment/decrement or compound operators
and do not have access to the backing variable.

```c++
std::cout << "Initial value: " << instance2.property << std::endl;
// instance.property = 3; <-- Compiler error
instance2.setter(3); // the on_change event is dispatched here
```

### Event subscription

`on_change` and `on_changing` are public members
of `observable` and `observable::readonly`.
See the `event` description above for details.
To subscribe (example):

```c++
...
observable<int> property1{};
observable<int>::readonly property2{some_private_property};
...
property1.on_change += callback;
property2.on_change += callback;
```

The *callback signature* is:

```c++
void callback(void *event, T value)
```

Where:

- `T` is the first template parameter and the type of the backing variable.

- `event` is a pointer to the `on_change` or `on_changing` event.
  If you need to, use this parameter to know
  which instance dispatched the event. For instance:

  ```c++
    ...
    if (event==&certain_object.property.on_change) {
      ...
    } else if (event==&certain_object.property.on_changing) {
      ...
    }
    ...
  ```

- `value` in the `on_changing` event is the value about to change.
- `value` in the `on_change` event is the latest value.

### Access to the backing variable

As shown before, you can write to the backing variable,
but there is no direct access to their members in complex types.
Let's see an example:

```c++
struct Item {
    std::string tag{};
    int value{}
};

observable<Item> observable_item{};
...
observable_item.value = 10; // ERROR: DOES NOT COMPILE
```

To access the backing variable, call `with()` to retrieve an
`observable::context` instance that provides access to the backing
variable and **guarantees** that events are properly dispatched.
In particular, `on_change` is dispatched
when `observable::context` goes out of scope.
To continue with the example:

```c++
...
{
  auto ctx = observable_item.with(); // on_changing is dispatched here
  ctx->value = 10;
} // on_change is dispatched here
...
```

Please, stick to the RAII idiom. The lifetime of the `observable::context`
instance must not supersede the lifetime of the corresponding `observable`.

### Additional notes

#### ⚠️ Infinite loop warning ⚠️

A callback **must** not change the observed variable directly or indirectly.
Otherwise, you have an infinite loop. For example:

```c++
observable<int> property{};

void callback(void *event, const int &new_value)
{
    property = new_value + 1; // Bad idea!
}

property.on_change += callback;
property = 1; // Infinite loop
```
