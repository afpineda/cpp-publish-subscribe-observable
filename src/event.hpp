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

//------------------------------------------------------------------------------

/**
 * @brief Publish-subscribe event
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
     * @brief Subscribe
     *
     * @note Thread-safe
     * @note Subscribing twice hass no effect
     *
     * @param callback Callback function to be called on event dispatch
     */
    void subscribe(const callback_type &callback) noexcept
    {
        ::std::lock_guard<::std::mutex> guard(subscribe_mutex);
        if (callback)
        {
            const void *tgt = callback.template target<callback_type>();
            ::std::string key;
            if (tgt)
            {
                key.resize(sizeof(tgt));
                ::std::memcpy(&key[0], &tgt, sizeof(tgt));
            }
            if (index_of_member(key) >= _callback.size())
            {
                _callback.push_back(callback);
                _callback_key.push_back(key);
            }
        }
    }

    /**
     * @brief Subscribe
     *
     * @note Thread-safe
     * @note Subscribing twice hass no effect
     *
     * @param callback Callback function to be called on event dispatch
     * @return type& Reference to this instance
     */
    type &operator+=(const callback_type &callback) noexcept
    {
        subscribe(callback);
        return *this;
    }

    /**
     * @brief Subscribe member function
     *
     * @note Thread-safe
     * @note Subscribing twice hass no effect
     *
     * @tparam C Holder class
     * @param member_function Member function
     * @param obj Holder
     */
    template <class C, class MemFn>
    void subscribe(MemFn member_function, C *obj) noexcept
    {
        if (obj && member_function)
        {
            ::std::lock_guard<::std::mutex> guard(subscribe_mutex);
            ::std::uintptr_t addr = reinterpret_cast<::std::uintptr_t>(obj);
            ::std::string key(sizeof(addr) + sizeof(member_function), '\0');
            ::std::memcpy(&key[0], &addr, sizeof(addr));
            ::std::memcpy(&key[0 + sizeof(addr)], &member_function, sizeof(member_function));
            auto callback = callback_type([obj, member_function](Args... args) {
                ::std::invoke(member_function, obj, args...);
            });
            if (index_of_member(key) >= _callback.size())
            {
                _callback.push_back(callback);
                _callback_key.push_back(key);
            }
        }
    }

    /**
     * @brief Unsubscribe
     *
     * @note Thread-safe
     * @note No effect if not subscribed
     *
     * @param callback Callback function previously subscribed
     */
    void unsubscribe(const callback_type &callback) noexcept
    {
        if (callback)
        {
            ::std::lock_guard<::std::mutex> guard(subscribe_mutex);
            const void *tgt = callback.template target<callback_type>();
            ::std::string key;
            if (tgt)
            {
                key.resize(sizeof(tgt));
                ::std::memcpy(&key[0], &tgt, sizeof(tgt));
            }
            auto index = index_of_member(key);
            if (index < _callback.size())
            {
                _callback.erase(_callback.begin() + index);
                _callback_key.erase(_callback_key.begin() + index);
            }
        }
    }

    /**
     * @brief Unsubscribe
     *
     * @note Thread-safe
     * @note No effect if not subscribed
     *
     * @param callback Callback function previously subscribed
     * @return type& Reference to this instance
     */
    type &operator-=(const callback_type &callback) noexcept
    {
        unsubscribe(callback);
        return *this;
    }

    /**
     * @brief Unsubscribe member function
     *
     * @note Thread-safe
     * @note No effect if not subscribed
     *
     * @tparam C Holder class
     * @param member_function Member function
     * @param obj Holder
     */
    template <class C, class MemFn>
    void unsubscribe(MemFn member_function, C *obj) noexcept
    {
        if (obj && member_function)
        {
            ::std::lock_guard<::std::mutex> guard(subscribe_mutex);
            ::std::uintptr_t addr = reinterpret_cast<::std::uintptr_t>(obj);
            ::std::string key(sizeof(addr) + sizeof(member_function), '\0');
            ::std::memcpy(&key[0], &addr, sizeof(addr));
            ::std::memcpy(&key[0 + sizeof(addr)], &member_function, sizeof(member_function));
            auto index = index_of_member(key);
            if (index < _callback.size())
            {
                _callback.erase(_callback.begin() + index);
                _callback_key.erase(_callback_key.begin() + index);
            }
        }
    }

    /**
     * @brief Clear all subscriptions
     *
     * @warning To be used exclusively in test units. Thread-safe.
     */
    void clear() noexcept
    {
        ::std::lock_guard<::std::mutex> guard(subscribe_mutex);
        _callback.clear();
        _callback_key.clear();
    }

    /**
     * @brief Check subscription
     *
     * @param callback Callback
     * @return true If subscribed
     * @return false If not subscribed
     */
    bool is_subscribed(const callback_type &callback)
    {
        const void *tgt = callback.template target<callback_type>();
        ::std::string key;
        if (tgt)
        {
            key.resize(sizeof(tgt));
            ::std::memcpy(&key[0], &tgt, sizeof(tgt));
        }
        return (index_of_member(key) < _callback.size());
    }

    /**
     * @brief Check subscription of member function
     *
     * @tparam C Holder class
     * @param member_function Member function
     * @param obj Holder instance
     * @return true If subscribed
     * @return false If not subscribed
     */
    template <class C, class MemFn>
    bool is_subscribed(MemFn member_function, C *obj)
    {
        ::std::uintptr_t addr = reinterpret_cast<::std::uintptr_t>(obj);
        ::std::string key(sizeof(addr) + sizeof(member_function), '\0');
        ::std::memcpy(&key[0], &addr, sizeof(addr));
        ::std::memcpy(&key[0 + sizeof(addr)], &member_function, sizeof(member_function));
        return (index_of_member(key) < _callback.size());
    }

    /**
     * @brief Dispatch event
     *
     * @param args Event data
     */
    void operator()(const Args &...args)
    {
        for (auto cb : _callback)
            cb(args...);
    }

    /**
     * @brief Dispatch event
     *
     * @param args Event data
     */
    void operator()(const Args &...args) const
    {
        for (auto cb : _callback)
            cb(args...);
    }

    /**
     * @brief Get the number of subscribed callbacks
     *
     * @return ::std::size_t Count of subscribed callbacks
     */
    ::std::size_t subscribed()
    {
        return _callback.size();
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
        _callback.swap(source._callback);
        _callback_key.swap(source._callback_key);
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
        _callback = source._callback;
        _callback_key = source._callback_key;
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
        : _callback{source._callback},
          _callback_key{source._callback_key},
          subscribe_mutex{} {}

    /**
     * @brief Move constructor
     *
     * @param source Rvalue
     */
    event(type &&source) : subscribe_mutex{}
    {
        _callback.swap(source._callback);
        _callback_key.swap(source._callback_key);
    }

private:
    /// @brief List of subscribed callbacks
    ::std::vector<callback_type> _callback{};
    /// @brief Key bytes to identify subscription (object address + member/function bytes)
    ::std::vector<::std::string> _callback_key{};
    /// @brief Mutex for callback subscription
    mutable ::std::mutex subscribe_mutex{};

    /**
     * @brief Get the index of a callback
     *
     * @param cb Callback
     * @param obj Callback holder or nullptr
     * @return ::std::size_t Index. If not found, index==_callback.size()
     */
    ::std::size_t index_of_member(const ::std::string &key) const noexcept
    {
        ::std::size_t index;
        for (index = 0; index < _callback.size(); index++)
            if (_callback_key[index] == key)
                break;
        return index;
    }
};

//------------------------------------------------------------------------------
