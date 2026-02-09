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

Subscribe and unsubscribe using the `+=` and `-=` operators.
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

`event` is thread-safe.

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

Each observable variable holds a subscribable event called `on_change`.
This event is dispatched every time the variable is written or modified.
More details below.

### Generally-writable observables

Use the `observable` class template to declare an observable variable.
Observable variables are backed by a regular variable (a reference)
which holds the actual value.
For instance:

```c++
class Example{
private:
    int _property{};
public:
    observable<int> property{_property};
} instance;
```

`observable` instances are implicitly casted to the given type
(first template parameter). Some examples:

```c++
// Read example
std::cout << "Initial value: " << instance.property << std::endl;
// Write/modify examples
instance.property = 3;
++instance.property;
instance.property++;
instance.property += 10;
```

### Publicly-readable, privately-writable observables

Use the `observable::readonly` subtype to declare
an observable variable for read only.
Read-only observables are backed by a regular `observable` variable
(a reference) which should be kept private to allow changes
from setter methods. For instance:

```c++
class Example2
{
private:
    int _property{};
    observable<int> private_property{_property};

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
increment/decrement or compound operators.

```c++
std::cout << "Initial value: " << instance2.property << std::endl;
// instance.property = 3; <-- Compiler error
instance2.setter(3); // the on_change event is dispatched here
```

### Subscribing to the `on_change` event

`on_change` is a member of `observable` and `observable::readonly`.
To subscribe (example):

```c++
observable<int> property{_property};
...
property.on_change += callback;
```

or

```c++
observable<int>::readonly property{private_property};
...
property.on_change += callback;
```

The *callback signature* depends on the second `observable` template parameter,
which defaults to `false`. In such a case the callback signature is:

```c++
void callback(void *event, T new_value)
```

Where:

- `event` is a pointer to the
  `observable.on_change` or `observable::readonly.on_change` event.
  If you need to, use this parameter to know
  which instance dispatched the event. For instance:

  ```c++
  void callback(void *event, const T &new_value)
  {
     if (sender==&certain_object.property.on_change) {
        ...
     }
  }
  ```

- `T` is the first template parameter and the type of the backing variable.
- `new_value` is the latest value written to the observable variable.

If the second `observable` template parameter is `true`,
the callback signature is:

```c++
void callback(void *sender, const T &old_value, const T &new_value)
```

Where `old_value` is the previous value
just before the variable gets overwritten.
There is a **small overhead** in this signature,
as the previous value has to be copied then discarded.

### Additional notes

#### Copy-assignable requirement

`T` in `observable<T>` must be copy-assignable.

#### Manual dispatching of the `on_change` event

You can dispatch the `on_change` event manually as shown
in the following example:

```c++
WeirdClass backing_var;
observable<WeirdClass> property{backing_var};
...
backing_var.setter("some value");
property.dispatch();
```

You are forced to do this because `property.setter()` does not exist
(`observable<WeirdClass>` does not have a `setter()` method).

#### ⚠️ Infinite loop warning⚠️

A callback **must** not change the observed variable directly or indirectly.
Otherwise, you have an infinite loop. For example:

```c++
int _property{};
observable<int> property{_property};

void callback(void *event, const T &new_value)
{
    property = new_value + 1; // Bad idea!
    property.dispatch(); // Bad idea!
}
...
property = 1; // Infinite loop
```
