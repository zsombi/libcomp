#ifndef COMP_TRACKER_HPP
#define COMP_TRACKER_HPP

#include <comp/config.hpp>
#include <comp/wrap/intrusive_ptr.hpp>
#include <comp/wrap/memory.hpp>
#include <comp/wrap/vector.hpp>

namespace comp
{

/// The Tracker template class implements the tracking the lifetime of objects, called trackables.
/// The assumption is that each trackable object has a disconnect() method that is called when
/// disconnectTrackables() is called.
/// To track an object, call track(). To remove an object from the tracked objects, call untrack().
/// To disconnect tracked objects, call clearTrackables().
template <typename T>
class COMP_API Tracker
{
public:
    /// Constructor.
    explicit Tracker() = default;

    /// Destructor.
    ~Tracker()
    {
        clearTrackables();
    }

    /// Adds a trackable object to track.
    void track(T trackable)
    {
       m_trackables.push_back(trackable);
    }
    /// Removes a tracked object. Does not disconnect the tracked object.
    void untrack(T trackable)
    {
       erase_first(m_trackables, trackable);
    }
    /// Clears the trackables, disconnecting each tracked objects.
    void clearTrackables()
    {
        while (!m_trackables.empty())
        {
            auto trackable = m_trackables.back();
            m_trackables.pop_back();
            if constexpr (is_pointer_v<T> || is_shared_ptr_v<T> || is_intrusive_ptr_v<T>)
            {
                trackable->disconnect();
            }
            else
            {
                trackable.disconnect();
            }
        }
    }

private:
    vector<T> m_trackables;
};

template <typename T>
struct is_tracker : false_type{};

template <typename T>
struct is_tracker<Tracker<T>> : true_type{};

template <typename T>
constexpr bool is_tracker_v = is_tracker<T>::value;

} // comp

#endif // COMP_TRACKER_HPP
