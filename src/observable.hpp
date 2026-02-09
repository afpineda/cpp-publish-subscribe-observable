/**
 * @file observable.hpp
 *
 * @author Ángel Fernández Pineda. Madrid. Spain. 2026.
 * @brief Observable pattern
 * @date 2026-02-08
 *
 * @copyright EUPL 1.2 License
 *
 */
#pragma once

#include "event.hpp"
#include <type_traits>

/**
 * @brief Observable variable
 *
 * @tparam T Variable type
 * @tparam notify_previous If true, both the previous and new values
 *                         are passed to the subscribed callbacks.
 *                         If false, only the new value is passed.
 */
template <typename T, bool notify_previous = false>
struct observable
{
    static_assert(
        ::std::is_copy_assignable<T>::value,
        "T must be copy-assignable in observable<T>");

    /// @brief Resulting type of this template instantiation
    using type = observable<T, notify_previous>;

    /// @brief Subscribable event type
    using event_type = ::std::conditional<
        notify_previous,
        event<void *, const T &, const T &>,
        event<void *, const T &>>::type;

    /// @brief Subscribable event to notify value changes
    event_type on_change{};

    /**
     * @brief Create an observable variable
     *
     * @warning The lifetime of the backing variable must match
     *          the lifetime of this instance.
     *
     * @param backing_var Backing variable holding the actual value
     */
    constexpr observable(T &backing_var) : _var{backing_var} {}

    /// @brief Copy constructor (default)
    constexpr observable(const type &) noexcept = default;
    /// @brief Move constructor (default)
    constexpr observable(type &&) noexcept = default;
    /// @brief Move-assignment (default)
    constexpr type &operator=(type &&) noexcept = default;

    /**
     * @brief Copy-assignment
     *
     * @param source Instance to be copied
     * @return constexpr type& Reference to this instance
     */
    constexpr type &operator=(const type &source) noexcept
    {
        _var = source._var;
        on_change = source.on_change;
        return *this;
    }

    /// @brief Get the current value
    constexpr operator T() const noexcept { return _var; }

    /// @brief Assign a new value
    /// @param source Value to be assigned
    /// @return Reference to this instance
    constexpr type &operator=(const T &source)
    {
        if constexpr (notify_previous)
        {
            T old_value = _var;
            _var = source;
            on_change(&on_change, old_value, source);
        }
        else
        {
            _var = source;
            on_change(&on_change, source);
        }
        return *this;
    }

    /// @brief Prefix increment
    /// @return Reference to this instance
    constexpr type &operator++() noexcept
    {
        if constexpr (notify_previous)
        {
            T old_value = _var;
            ++_var;
            on_change(&on_change, old_value, _var);
        }
        else
        {
            ++_var;
            on_change(&on_change, _var);
        }
        return *this;
    }

    /// @brief Postfix increment
    /// @param Unused
    /// @return Copy of this instance holding the previous value
    constexpr type operator++(int)
    {
        T old_instance = *this;
        operator++();
        return old_instance;
    }

    /// @brief Prefix decrement
    /// @return Reference to this instance
    constexpr type &operator--()
    {
        if constexpr (notify_previous)
        {
            T old_value = _var;
            --_var;
            on_change(&on_change, old_value, _var);
        }
        else
        {
            --_var;
            on_change(&on_change, _var);
        }
        return *this;
    }

    /// @brief Postfix decrement
    /// @param Unused
    /// @return Copy of this instance holding the previous value
    constexpr type operator--(int)
    {
        T old_instance = *this;
        operator--();
        return old_instance;
    }

    /// @brief Compound increment
    /// @param rhs Value to add
    /// @return Reference to this instance
    constexpr type &operator+=(const T &rhs)
    {
        if constexpr (notify_previous)
        {
            T old_value = _var;
            _var += rhs;
            on_change(&on_change, old_value, _var);
        }
        else
        {
            _var += rhs;
            on_change(&on_change, _var);
        }
        return *this;
    }

    /// @brief Compound decrement
    /// @param rhs Value to substract
    /// @return Reference to this instance
    constexpr type &operator-=(const T &rhs)
    {
        if constexpr (notify_previous)
        {
            T old_value = _var;
            _var -= rhs;
            on_change(&on_change, old_value, _var);
        }
        else
        {
            _var -= rhs;
            on_change(&on_change, _var);
        }
        return *this;
    }

    /// @brief Compound product
    /// @param rhs Value to multiply
    /// @return Reference to this instance
    constexpr type &operator*=(const T &rhs)
    {
        if constexpr (notify_previous)
        {
            T old_value = _var;
            _var *= rhs;
            on_change(&on_change, old_value, _var);
        }
        else
        {
            _var *= rhs;
            on_change(&on_change, _var);
        }
        return *this;
    }

    /// @brief Compound division
    /// @param rhs Divisor
    /// @return Reference to this instance
    constexpr type &operator/=(const T &rhs)
    {
        if constexpr (notify_previous)
        {
            T old_value = _var;
            _var /= rhs;
            on_change(&on_change, old_value, _var);
        }
        else
        {
            _var /= rhs;
            on_change(&on_change, _var);
        }
        return *this;
    }

    /// @brief Compound modulus
    /// @param rhs Divisor
    /// @return Reference to this instance
    constexpr type &operator%=(const T &rhs)
    {
        if constexpr (notify_previous)
        {
            T old_value = _var;
            _var %= rhs;
            on_change(&on_change, old_value, _var);
        }
        else
        {
            _var %= rhs;
            on_change(&on_change, _var);
        }
        return *this;
    }

    /// @brief Compound binary xor
    /// @param rhs Operand
    /// @return Reference to this instance
    constexpr type &operator^=(const T &rhs)
    {
        if constexpr (notify_previous)
        {
            T old_value = _var;
            _var ^= rhs;
            on_change(&on_change, old_value, _var);
        }
        else
        {
            _var ^= rhs;
            on_change(&on_change, _var);
        }
        return *this;
    }

    /// @brief Compound binary conjuction
    /// @param rhs Operand
    /// @return Reference to this instance
    constexpr type &operator&=(const T &rhs)
    {
        if constexpr (notify_previous)
        {
            T old_value = _var;
            _var &= rhs;
            on_change(&on_change, old_value, _var);
        }
        else
        {
            _var &= rhs;
            on_change(&on_change, _var);
        }
        return *this;
    }

    /// @brief Compound binary disjunction
    /// @param rhs Operand
    /// @return Reference to this instance
    constexpr type &operator|=(const T &rhs)
    {
        if constexpr (notify_previous)
        {
            T old_value = _var;
            _var |= rhs;
            on_change(&on_change, old_value, _var);
        }
        else
        {
            _var |= rhs;
            on_change(&on_change, _var);
        }
        return *this;
    }

    /**
     * @brief Forcedly dispatch the on_change event
     *
     * @note Requires the template parameter `notify_previous` set to `false`
     */
    void dispatch()
    {
        static_assert(
            !notify_previous,
            "observable::dispatch() requires a previous value as parameter");
        if constexpr (!notify_previous)
            on_change(&on_change, _var);
    }

    /**
     * @brief Forcedly dispatch the on_change event
     *
     * @param old_value Previous value.
     *                  Ignored if `notify_previous` is `false`.
     */
    void dispatch(const T &old_value)
    {
        if constexpr (notify_previous)
            on_change(&on_change, old_value, _var);
        else
            on_change(&on_change, _var);
    }

    /**
     * @brief Read-only observable variable for public interfaces
     *
     * @note The observable variable can change only by means of
     *       a backing observable which should be hold in private.
     */
    struct readonly
    {
        /// @brief Subscribable event to notify value changes
        event_type &on_change;

        /**
         * @brief Create a read-only observable variable
         *
         * @warning The lifetime of the backing observable must match
         *          the lifetime of this instance.
         *
         * @param writer Observable holding the actual value
         */
        constexpr readonly(type &writer)
            : on_change{writer.on_change}, _writer{writer} {}

        /// @brief Copy constructor (default)
        constexpr readonly(const readonly &) noexcept = default;
        /// @brief Move constructor (default)
        constexpr readonly(readonly &&) noexcept = default;
        /// @brief Copy-assignment (default)
        constexpr readonly &operator=(const readonly &) noexcept = default;
        /// @brief Move-assignment (default)
        constexpr readonly &operator=(readonly &&) noexcept = default;

        /// @brief Get the current value
        constexpr operator T() const noexcept { return _writer._var; }

    private:
        /// @brief Reference to backing observable
        type &_writer;
    };

private:
    friend class readonly;
    /// @brief Reference to backing variable
    T &_var;
};
