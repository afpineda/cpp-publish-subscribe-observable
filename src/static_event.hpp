/**
 * @file static_event.hpp
 *
 * @author Ángel Fernández Pineda. Madrid. Spain. 2026.
 * @brief Publish-subscribe pattern
 * @date 2026-02-21
 *
 * @copyright EUPL 1.2 License
 *
 */

#pragma once

//------------------------------------------------------------------------------

#include <initializer_list>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <type_traits>

//------------------------------------------------------------------------------

/**
 * @brief Publish-subscribe event (forever subscribed)
 *
 * @note Thread-safe
 *
 * @tparam Args Callback argument types
 */
template <class... Args>
class static_event
{
public:
    /// @brief This type
    using type = static_event<Args...>;
    /// @brief Callback type for subscribers
    using callback_type = typename ::std::add_pointer<void(Args...)>::type;
    /// @brief Callback for event dispatching (optional)
    using dispatch_callback_type =
        typename ::std::add_pointer<void(::std::size_t)>::type;

    /**
     * @brief Function to be called on event dispatch after
     *        each subscribed callback
     *
     * @details If provided, this function runs after each subscribed callback
     *          has finished and before the next one begins.
     *
     * @note Use this callback to reset watchdog timers or yield tasks.
     */
    dispatch_callback_type on_dispatch{nullptr};

    /**
     * @brief Subscribe forever
     *
     * @param callback Callback function to be called on event dispatch
     */
    void subscribe(callback_type callback) noexcept
    {
        if (!callback)
            return;

        ::std::unique_lock<::std::shared_mutex> guard(subscribe_mutex);
        _subscriptions.push_back(callback);
    }

    /**
     * @brief Subscribe forever
     *
     * @note Syntactic sugar
     *
     * @param callback Callback function to be called on event dispatch
     * @return type& This instance
     */
    type &operator+=(const callback_type &callback) noexcept
    {
        subscribe(callback);
        return *this;
    }

    /**
     * @brief Clear all subscriptions
     *
     * @warning To be used exclusively in test units.
     */
    void clear() noexcept
    {
        ::std::unique_lock<::std::shared_mutex> guard(subscribe_mutex);
        _subscriptions.clear();
    }

    /**
     * @brief Dispatch event to all subscribed callbacks
     *
     * @param args Event data
     */
    void operator()(const Args &...args)
    {
        ::std::shared_lock<::std::shared_mutex> guard(subscribe_mutex);
        ::std::size_t counter = 0;
        for (const auto &entry : _subscriptions)
        {
            entry(args...);
            if (on_dispatch)
                on_dispatch(++counter);
        }
    }

    /**
     * @brief Dispatch event to all subscribed callbacks (const)
     *
     * @param args Event data
     */
    void operator()(const Args &...args) const
    {
        ::std::shared_lock<::std::shared_mutex> guard(subscribe_mutex);
        ::std::size_t counter = 0;
        for (const auto &entry : _subscriptions)
        {
            entry(args...);
            if (on_dispatch)
                on_dispatch(++counter);
        }
    }

    /**
     * @brief Get the number of subscribed callbacks
     *
     * @return ::std::size_t Count of subscribed callbacks
     */
    ::std::size_t subscribed()
    {
        return _subscriptions.size();
    }

    /**
     * @brief Move-assignment
     *
     * @param source Rvalue
     * @return type& Reference to this instance
     */
    type &operator=(type &&source) noexcept
    {
        ::std::unique_lock<::std::shared_mutex> guard1(subscribe_mutex);
        ::std::unique_lock<::std::shared_mutex> guard2(source.subscribe_mutex);
        _subscriptions.swap(source._subscriptions);
        return *this;
    }

    /**
     * @brief Copy-assignment
     *
     * @param source Instance to be copied
     * @return type& Reference to this instance
     */
    type &operator=(const type &source) noexcept
    {
        ::std::unique_lock<::std::shared_mutex> guard1(subscribe_mutex);
        ::std::shared_lock<::std::shared_mutex> guard2(source.subscribe_mutex);
        _subscriptions = source._subscriptions;
        return *this;
    }

    /**
     * @brief Default constructor
     *
     */
    constexpr static_event() noexcept = default;

    /**
     * @brief List-initialized constructor
     *
     */
    constexpr static_event(::std::initializer_list<callback_type> init_list) noexcept
        : _subscriptions(init_list) {};

    /**
     * @brief Copy constructor
     *
     * @param source Instance to be copied
     */
    static_event(const type &source)
    {
        ::std::shared_lock<::std::shared_mutex> guard2(source.subscribe_mutex);
        _subscriptions = source._subscriptions;
    }

    /**
     * @brief Move constructor
     *
     * @param source Rvalue
     */
    static_event(type &&source)
    {
        ::std::unique_lock<::std::shared_mutex> guard(source.subscribe_mutex);
        _subscriptions.swap(source._subscriptions);
    }

    /**
     * @brief Equality operator
     *
     * @note Mostly for testing purposes.
     *
     * @param other Instance to compare to
     * @return true If equal
     * @return false If not equal
     */
    constexpr bool operator==(const type &other) const noexcept
    {
        return (_subscriptions == other._subscriptions);
    }

private:
    /// @brief List of subscription entries
    ::std::vector<callback_type> _subscriptions{};
    /// @brief Mutex for thread-safe operations
    mutable ::std::shared_mutex subscribe_mutex{};
};

//------------------------------------------------------------------------------
