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
        if (callback && (index_of(callback, nullptr) >= _callback.size()))
        {
            _callback.push_back(callback);
            _callback_holder.push_back(nullptr);
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
    template <class C>
    void subscribe(void (C::*member_function)(Args...), C *obj) noexcept
    {
        if (obj && member_function)
        {
            ::std::lock_guard<::std::mutex> guard(subscribe_mutex);
            auto callback = (callback_type(::std::bind(member_function, obj)));
            if (index_of(callback, obj) >= _callback.size())
            {
                _callback.push_back(callback);
                _callback_holder.push_back(obj);
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
            auto index = index_of(callback, nullptr);
            if (index < _callback.size())
            {
                _callback.erase(_callback.begin() + index);
                _callback_holder.erase(_callback_holder.begin() + index);
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
    template <class C>
    void unsubscribe(void (C::*member_function)(Args...), C *obj) noexcept
    {
        if (obj && member_function)
        {
            ::std::lock_guard<::std::mutex> guard(subscribe_mutex);
            auto callback = (callback_type(::std::bind(member_function, obj)));
            auto index = index_of(callback, obj);
            if (index < _callback.size())
            {
                _callback.erase(_callback.begin() + index);
                _callback_holder.erase(_callback_holder.begin() + index);
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
        _callback_holder.clear();
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
        return (index_of(callback, nullptr) < _callback.size());
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
    template <class C>
    bool is_subscribed(void (C::*member_function)(Args...), C *obj)
    {
        callback_type callback =
            (callback_type(::std::bind(member_function, obj)));
        return (index_of(callback, obj) < _callback.size());
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
        _callback_holder.swap(source._callback_holder);
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
        _callback_holder = _callback_holder;
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
          _callback_holder{source._callback_holder},
          subscribe_mutex{} {}

    /**
     * @brief Move constructor
     *
     * @param source Rvalue
     */
    event(type &&source) : subscribe_mutex{}
    {
        _callback.swap(source._callback);
        _callback_holder.swap(source._callback_holder);
    }

private:
    /// @brief List of subscribed callbacks
    ::std::vector<callback_type> _callback{};
    /// @brief List of callback holders (in the case of a member function)
    ::std::vector<void *> _callback_holder{};
    /// @brief Mutex for callback subscription
    mutable ::std::mutex subscribe_mutex{};

    /**
     * @brief Get the index of a callback
     *
     * @param cb Callback
     * @param obj Callback holder or nullptr
     * @return ::std::size_t Index. If not found, index==_callback.size()
     */
    ::std::size_t index_of(const callback_type &cb, void *obj)
        const noexcept
    {
        ::std::size_t index;
        for (index = 0; index < _callback.size(); index++)
            if ((_callback_holder[index] == obj) &&
                (_callback[index].template target<callback_type>() ==
                 cb.template target<callback_type>()))
                break;
        return index;
    }
};

//------------------------------------------------------------------------------
