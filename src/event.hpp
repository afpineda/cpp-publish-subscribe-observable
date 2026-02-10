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
#include <vector>
#include <mutex>
#include <string>
#include <cstring>
#include <cstdint>

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

/**
 * @brief Publish-subscribe event with subscription handles
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
     * @note Provides O(1) unsubscribe via generational index
     * @note Invalid after callback is unsubscribed
     */
    class subscription_handler
    {
        friend class event<Args...>;
        /// @brief Index in subscription array
        ::std::size_t index = 0;
        /// @brief Generation number to prevent ABA problem
        ::std::uint32_t generation = 0;
        /// @brief Pointer to owning event instance
        void *owner = nullptr;
        /// @brief Subscription status
        bool subscribed = false;

        /**
         * @brief Check if handle is valid (generation > 0 and owner != nullptr)
         * @return true if handle may be valid
         * @return false if handle is definitely invalid
         */
        bool is_valid() const noexcept
        {
            return generation > 0 && owner != nullptr;
        }

    public:
        constexpr bool is_subscribed() const noexcept { return subscribed; }
    };

    /**
     * @brief Subscribe a callback function
     *
     * @note Thread-safe
     *
     * @param callback Callback function to be called on event dispatch
     * @return subscription_handler Handler required to unsubscribe
     */
    subscription_handler subscribe(const callback_type &callback) noexcept
    {
        ::std::lock_guard<::std::mutex> guard(subscribe_mutex);

        if (!callback)
            return {};

        // Compute key for callback identity
        const void *tgt = callback.template target<callback_type>();
        ::std::string key;
        if (tgt)
        {
            key.resize(sizeof(tgt));
            ::std::memcpy(&key[0], &tgt, sizeof(tgt));
        }

        // Allocate slot
        subscription_handler result;
        result.owner = static_cast<void *>(this);
        result.subscribed = true;

        if (!_free_list.empty())
        {
            result.index = _free_list.back();
            _free_list.pop_back();
            result.generation = _subscriptions[result.index].generation + 1;
        }
        else
        {
            result.index = _subscriptions.size();
            result.generation = 1;
        }

        // Store subscription
        if (result.index >= _subscriptions.size())
            _subscriptions.resize(result.index + 1);

        _subscriptions[result.index] = {callback, key, result.generation, true};

        return result;
    }

    /**
     * @brief Subscribe a member function
     *
     * @note Thread-safe
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
        callback_type callback = [member_function, obj](Args... args) {
            (obj->*member_function)(args...);
        };
        return subscribe(callback);
    }

    /**
     * @brief Unsubscribe
     *
     * @note Thread-safe
     * @note No effect if @p h is invalid or already unsubscribed
     *
     * @param h Subscription handler returned by subscribe()
     */
    void unsubscribe(subscription_handler &h) noexcept
    {
        if (!h.is_valid())
            return;

        // Verify handle belongs to this instance
        if (h.owner != static_cast<const void *>(this))
            return; // Handle from different event instance, no-op

        ::std::lock_guard<::std::mutex> guard(subscribe_mutex);

        // Verify handle validity
        if (h.index >= _subscriptions.size() ||
            _subscriptions[h.index].generation != h.generation)
        {
            return; // Handle is stale, no-op
        }

        // Mark slot as free
        _subscriptions[h.index].active = false;
        _subscriptions[h.index].generation++;
        _free_list.push_back(h.index);

        // Mark handle as not subscribed
        h.subscribed = false;
    }

    /**
     * @brief Clear all subscriptions
     *
     * @warning To be used exclusively in test units. Thread-safe.
     */
    void clear() noexcept
    {
        ::std::lock_guard<::std::mutex> guard(subscribe_mutex);
        _subscriptions.clear();
        _free_list.clear();
    }

    /**
     * @brief Dispatch event to all subscribed callbacks
     *
     * @param args Event data
     */
    void operator()(const Args &...args)
    {
        for (const auto &entry : _subscriptions)
        {
            if (entry.active)
                entry.callback(args...);
        }
    }

    /**
     * @brief Dispatch event to all subscribed callbacks (const)
     *
     * @param args Event data
     */
    void operator()(const Args &...args) const
    {
        for (const auto &entry : _subscriptions)
        {
            if (entry.active)
                entry.callback(args...);
        }
    }

    /**
     * @brief Get the number of subscribed callbacks
     *
     * @return ::std::size_t Count of subscribed callbacks
     */
    ::std::size_t subscribed()
    {
        ::std::size_t count = 0;
        for (const auto &entry : _subscriptions)
        {
            if (entry.active)
                count++;
        }
        return count;
    }

    /**
     * @brief Move-assignment
     *
     * @param source Rvalue
     * @return type& Reference to this instance
     */
    type &operator=(type &&source) noexcept
    {
        ::std::lock_guard<::std::mutex> guard(subscribe_mutex);
        _subscriptions.swap(source._subscriptions);
        _free_list.swap(source._free_list);
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
        ::std::lock_guard<::std::mutex> guard(subscribe_mutex);
        _subscriptions = source._subscriptions;
        _free_list = source._free_list;
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
        : _subscriptions{source._subscriptions},
          _free_list{source._free_list},
          subscribe_mutex{} {}

    /**
     * @brief Move constructor
     *
     * @param source Rvalue
     */
    event(type &&source) : subscribe_mutex{}
    {
        _subscriptions.swap(source._subscriptions);
        _free_list.swap(source._free_list);
    }

private:
    /// @brief Subscription entry with callback, key, and generation
    struct subscription_entry
    {
        callback_type callback;
        ::std::string key;
        ::std::uint32_t generation = 0;
        bool active = false;
    };

    /// @brief List of subscription entries
    ::std::vector<subscription_entry> _subscriptions{};
    /// @brief List of free slot indices for reuse
    ::std::vector<::std::size_t> _free_list{};
    /// @brief Mutex for thread-safe operations
    mutable ::std::mutex subscribe_mutex{};
};

//------------------------------------------------------------------------------
