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

/**
 * @brief Observable variable
 *
 * @tparam T Backing variable type
 */
template <typename T>
struct observable
{
    /// @brief Resulting type of this template instantiation
    using type = observable<T>;

    /// @brief Subscribable event type
    using event_type = event<void *, const T &>;

    //.... Subscribable events ....

    /// @brief Subscribable event to notify about to change values
    event_type on_changing{};

    /// @brief Subscribable event to notify value changes
    event_type on_change{};

    //.... Constructors ....

    /// @brief Default initialization constructor
    constexpr observable() : _var{} {}

    /// @brief Initialization constructor
    /// @param initial_value Initial value
    constexpr observable(const T &initial_value) : _var{initial_value} {}

    /// @brief Copy constructor
    constexpr observable(const type &) noexcept = default;
    /// @brief Move constructor
    constexpr observable(type &&) noexcept = default;
    /// @brief Move-assignment
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

    //.... Read ....

    /// @brief Get the value of the backing variable
    constexpr operator T() const noexcept { return _var; }

    //.... Write ....

    /// @brief Assign a new value
    /// @param source Value to be assigned
    /// @return Reference to this instance
    constexpr type &operator=(const T &source)
    {
        on_changing(&on_changing, _var);
        _var = source;
        on_change(&on_change, source);
        return *this;
    }

    /// @brief Prefix increment
    /// @return Reference to this instance
    constexpr type &operator++() noexcept
    {
        on_changing(&on_changing, _var);
        ++_var;
        on_change(&on_change, _var);
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
        on_changing(&on_changing, _var);
        --_var;
        on_change(&on_change, _var);
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
        on_changing(&on_changing, _var);
        _var += rhs;
        on_change(&on_change, _var);
        return *this;
    }

    /// @brief Compound decrement
    /// @param rhs Value to substract
    /// @return Reference to this instance
    constexpr type &operator-=(const T &rhs)
    {
        on_changing(&on_changing, _var);
        _var -= rhs;
        on_change(&on_change, _var);
        return *this;
    }

    /// @brief Compound product
    /// @param rhs Value to multiply
    /// @return Reference to this instance
    constexpr type &operator*=(const T &rhs)
    {
        on_changing(&on_changing, _var);
        _var *= rhs;
        on_change(&on_change, _var);
        return *this;
    }

    /// @brief Compound division
    /// @param rhs Divisor
    /// @return Reference to this instance
    constexpr type &operator/=(const T &rhs)
    {
        on_changing(&on_changing, _var);
        _var /= rhs;
        on_change(&on_change, _var);
        return *this;
    }

    /// @brief Compound modulus
    /// @param rhs Divisor
    /// @return Reference to this instance
    constexpr type &operator%=(const T &rhs)
    {
        on_changing(&on_changing, _var);
        _var %= rhs;
        on_change(&on_change, _var);
        return *this;
    }

    /// @brief Compound binary xor
    /// @param rhs Operand
    /// @return Reference to this instance
    constexpr type &operator^=(const T &rhs)
    {
        on_changing(&on_changing, _var);
        _var ^= rhs;
        on_change(&on_change, _var);
        return *this;
    }

    /// @brief Compound binary conjuction
    /// @param rhs Operand
    /// @return Reference to this instance
    constexpr type &operator&=(const T &rhs)
    {
        on_changing(&on_changing, _var);
        _var &= rhs;
        on_change(&on_change, _var);
        return *this;
    }

    /// @brief Compound binary disjunction
    /// @param rhs Operand
    /// @return Reference to this instance
    constexpr type &operator|=(const T &rhs)
    {
        on_changing(&on_changing, _var);
        _var |= rhs;
        on_change(&on_change, _var);
        return *this;
    }

    //.... Backing variable access without a context ....
    //     Easier but prone to human mistake
    //     Uncomment if you wish.

    // /// @brief Forcedly dispatch the on_change event
    // void dispatch()
    // {
    //     on_change(&on_change, _var);
    // }

    // /**
    //  * @brief Forcedly dispatch the on_changing and on_change events
    //  *
    //  * @param old_value Previous value.
    //  */
    // void dispatch(const T &old_value)
    // {
    //     on_changing(&on_changing, old_value);
    //     on_change(&on_change, _var);
    // }

    // T *operator->()
    // {
    //     return &_var;
    // }

    //.... Backing variableaccess via context ....

    /**
     * @brief Context to access the backing variable members
     *
     */
    struct context
    {
        /// @brief Dispatch on_change
        virtual ~context()
        {
            owner.on_change(&owner.on_change, owner._var);
        }

        /// @brief Move constructor
        constexpr context(context &&) noexcept = default;

        /// @brief Deleted copy constructor
        constexpr context(const context &) noexcept = delete;

        /// @brief Move-assignment
        constexpr context &operator=(context &&) noexcept = default;

        /// @brief Deleted copy-assignment
        constexpr context &operator=(const context &) noexcept = delete;

        /// @brief Access to the backing variable
        /// @return Pointer to the backing variable
        T *operator->()
        {
            return &owner._var;
        }

    private:
        friend class observable<T>;
        /// @brief Context owner type
        using owner_type = observable<T>;
        /// @brief Context owner
        observable<T> &owner;

        /// @brief Private constructor
        /// @param owner Owner of this context
        constexpr context(owner_type &owner) : owner{owner}
        {
            owner.on_changing(&owner.on_changing, owner._var);
        }
    };

    context with() { return context(*this); }

    //.... Public read-only access ....

    /**
     * @brief Read-only observable variable for public interfaces
     *
     * @note The observable variable can change only by means of
     *       a backing observable which should be hold in private.
     */
    struct readonly
    {
        /// @brief Subscribable event to notify about to change values
        event_type &on_changing;

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
            : on_changing{writer.on_changing},
              on_change{writer.on_change},
              _var{writer._var} {}

        /// @brief Copy constructor (default)
        constexpr readonly(const readonly &) noexcept = default;
        /// @brief Move constructor (default)
        constexpr readonly(readonly &&) noexcept = default;
        /// @brief Copy-assignment (default)
        constexpr readonly &operator=(const readonly &) noexcept = default;
        /// @brief Move-assignment (default)
        constexpr readonly &operator=(readonly &&) noexcept = default;

        /// @brief Get the current value
        constexpr operator T() const noexcept { return _var; }

    private:
        /// @brief Reference to the backing variable
        T &_var;
    };

private:
    friend class readonly;
    /// @brief Backing variable
    T _var;
};
