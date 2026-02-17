/**
 * @file event.hpp
 *
 * @author Ángel Fernández Pineda. Madrid. Spain. 2026.
 * @brief Publish-subscribe pattern
 * @date 2026-02-07
 *
 * @copyright EUPL 1.2 License
 *
 */

#pragma once

//------------------------------------------------------------------------------

#include <functional>
#include <deque>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <cstring>
#include <cstdint>

//------------------------------------------------------------------------------

/**
 * @brief Publish-subscribe event
 *
 * @note Thread-safe
 *
 * @tparam Args Callback argument types
 */
template <class... Args>
class event
{
public:
    /// @brief This type
    using type = event<Args...>;
    /// @brief Callback type
    using callback_type = typename ::std::function<void(Args...)>;

    /**
     * @brief Subscription handler for managing callback lifetimes
     *
     * @note Invalid after callback is unsubscribed
     */
    class subscription_handler
    {
        friend class event<Args...>;
        /// @brief Pointer to owning event instance
        void *owner{nullptr};
        /// @brief Subscription id
        ::std::size_t id{0};

        /**
         * @brief Private constructor
         * @param owner Owning event instance
         * @param id Subscription id
         */
        constexpr subscription_handler(void *owner, ::std::size_t id) noexcept
            : owner{owner}, id{id} {}

    public:
        /**
         * @brief Check if subscribed
         *
         * @return true if subscribed
         * @return false otherwise
         */
        constexpr bool is_subscribed() const noexcept
        {
            return (owner != nullptr);
        }

        /// @brief Default constructor
        constexpr subscription_handler() noexcept = default;
        /// @brief Move constructor (default)
        constexpr subscription_handler(
            subscription_handler &&) noexcept = default;
        /// @brief Copy constructor (deleted)
        constexpr subscription_handler(
            const subscription_handler &) noexcept = delete;
        /// @brief Move-assignment (default)
        constexpr subscription_handler &operator=(
            subscription_handler &&) noexcept = default;
        /// @brief Copy-assignment (deleted)
        constexpr subscription_handler &operator=(
            const subscription_handler &) noexcept = delete;
    };

    /**
     * @brief Subscribe a callback function
     *
     * @param callback Callback function to be called on event dispatch
     * @return subscription_handler Handler required to unsubscribe
     */
    subscription_handler subscribe(const callback_type &callback) noexcept
    {
        if (!callback)
            return subscription_handler();

        ::std::unique_lock<::std::shared_mutex> guard(subscribe_mutex);
        _subscriptions.push_back(
            {
                .callback = callback,
                .id = next_id,
            });

        return subscription_handler(this, next_id++);
    }

    /**
     * @brief Subscribe a member function
     *
     * @tparam C Holder class
     * @tparam MemFn Member function pointer type
     * @param member_function Member function
     * @param obj Holder instance
     * @return subscription_handler Handler required to unsubscribe
     */
    template <class C, class MemFn>
    subscription_handler subscribe(MemFn member_function, C *obj) noexcept
    {
        // Create callback wrapper from member function and object
        callback_type callback = [member_function, obj](Args... args)
        {
            (obj->*member_function)(args...);
        };
        return subscribe(callback);
    }

    /**
     * @brief Unsubscribe
     *
     * @note No effect if @p h is invalid or already unsubscribed
     *
     * @param h Subscription handler returned by subscribe()
     */
    void unsubscribe(subscription_handler &h) noexcept
    {
        if (h.owner != this)
            return;

        ::std::unique_lock<::std::shared_mutex> guard(subscribe_mutex);
        for (auto entry = _subscriptions.begin();
             entry != _subscriptions.end();
             ++entry)
            if (entry->id == h.id)
            {
                _subscriptions.erase(entry);
                break;
            }
        h.owner = nullptr;
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
        for (const auto &entry : _subscriptions)
            entry.callback(args...);
    }

    /**
     * @brief Dispatch event to all subscribed callbacks (const)
     *
     * @param args Event data
     */
    void operator()(const Args &...args) const
    {
        ::std::shared_lock<::std::shared_mutex> guard(subscribe_mutex);
        for (const auto &entry : _subscriptions)
            entry.callback(args...);
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
    constexpr event() noexcept = default;

    /**
     * @brief Copy constructor
     *
     * @param source Instance to be copied
     */
    event(const type &source)
    {
        ::std::shared_lock<::std::shared_mutex> guard2(source.subscribe_mutex);
        _subscriptions = source._subscriptions;
    }

    /**
     * @brief Move constructor
     *
     * @param source Rvalue
     */
    event(type &&source)
    {
        ::std::unique_lock<::std::shared_mutex> guard(source.subscribe_mutex);
        _subscriptions.swap(source._subscriptions);
    }

private:
    /// @brief Subscription entry
    struct subscription_entry
    {
        /// @brief Callback
        callback_type callback;
        /// @brief Subscription id
        ::std::size_t id;
    };

    /// @brief List of subscription entries
    ::std::deque<subscription_entry> _subscriptions{};
    /// @brief Next subscription id
    ::std::size_t next_id{0};
    /// @brief Mutex for thread-safe operations
    mutable ::std::shared_mutex subscribe_mutex{};
};

//------------------------------------------------------------------------------
