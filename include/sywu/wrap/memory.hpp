#ifndef SYWU_MEMORY_HPP
#define SYWU_MEMORY_HPP

#include <memory>
#include <sywu/wrap/utility.hpp>
#include <sywu/wrap/type_traits.hpp>

namespace sywu
{

using std::unique_ptr;
using std::default_delete;
using std::shared_ptr;
using std::weak_ptr;

using std::make_unique;
using std::make_shared;

using std::dynamic_pointer_cast;
using std::static_pointer_cast;

using std::enable_shared_from_this;

using std::bad_weak_ptr;

/// \name Traits
/// \{

/// Detect unique_ptr
template <typename T, typename = void>
struct is_unique_ptr : false_type {};

template <typename T>
struct is_unique_ptr<T, enable_if_t<is_same_v<decay_t<T>, unique_ptr<typename decay_t<T>::element_type, typename decay_t<T>::deleter_type>>>> : true_type {};

template <typename T>
constexpr bool is_unique_ptr_v = is_unique_ptr<T>::value;

/// Detect weak_ptr
template <typename T, typename = void>
struct is_weak_ptr : false_type {};

template <typename T>
struct is_weak_ptr<T, enable_if_t<is_same_v<decay_t<T>, weak_ptr<typename decay_t<T>::element_type>>>> : true_type {};

template <typename T>
constexpr bool is_weak_ptr_v = is_weak_ptr<T>::value;

/// Detect shared_ptr
template <typename T, typename = void>
struct is_shared_ptr : false_type {};

template <typename T>
struct is_shared_ptr<T, enable_if_t<is_same_v<decay_t<T>, shared_ptr<typename decay_t<T>::element_type>>>> : true_type {};

template <typename T>
constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;

/// \}

/// std::make_shared initiates lots of control templates, creates loads of control blocks and deleters,
/// which all increase the code size. This template offers smaller code size.
template <class Base, class Derived, class... Arguments>
shared_ptr<Base> make_shared(Arguments&&... args)
{
    return shared_ptr<Base>(static_cast<Base*>(new Derived(forward<Arguments>(args)...)));
}

/// Lock object for shared pointers. When locked, it increases the use count of the shared
/// pointer. When unlocked, it decreases the use count of the shared pointer. The SharedPtrLock
/// increses the use count of the managed shared pointer only a single time.
/// \tparam SharedPtrEnabled A type that is derived from enable_shared_from_this<>
template <class SharedPtrEnabled>
struct SharedPtrLock
{
    /// Constructs a shared pointer lock object using the \a lockable passed as argument.
    /// \param lockable The reference to the object that is derived from enable_shared_from_this<>.
    explicit SharedPtrLock(SharedPtrEnabled& lockable)
        : m_lockable(lockable)
    {
    }
    /// Destructor.
    ~SharedPtrLock()
    {
        SYWU_ASSERT(!m_locked);
    }

    /// Locks the shared pointer by increasing the use count of the managed pointer. Asserts if the pointer is already locked.
    void lock()
    {
        const auto test = try_lock();
        SYWU_ASSERT(test);
    }

    /// Tries to lock the shared pointer.
    /// \returns If the shared pointer is locked, returns \e true. If the shared pointer is already locked, returns \e false.
    bool try_lock()
    {
        if (m_locked)
        {
            return false;
        }
        m_locked = m_lockable.shared_from_this();
        return true;
    }

    /// Unlocks the shared pointer by decreasing the use count of the managed pointer. Asserts if the pointer is already unlocked.
    void unlock()
    {
        SYWU_ASSERT(m_locked);
        m_locked.reset();
    }
    /// Checks the locked status of the shared pointer managed by a SharedPtrLock.
    /// \returns If the
    bool isLocked()
    {
        return m_locked != nullptr;
    }

private:
    SharedPtrEnabled& m_lockable;
    shared_ptr<SharedPtrEnabled> m_locked;
};

} // namespace sywu

#endif // SYWU_MEMORY_HPP
