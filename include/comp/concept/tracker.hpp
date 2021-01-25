#ifndef COMP_TRACKER_HPP
#define COMP_TRACKER_HPP

#include <comp/config.hpp>
#include <comp/concept/zero_safe_container.hpp>
#include <comp/wrap/type_traits.hpp>

namespace comp { namespace trackers
{

class COMP_API Trackable
{
public:
    virtual ~Trackable() = default;

    virtual void disconnect() = 0;

protected:
    explicit Trackable() = default;

    COMP_DISABLE_COPY_OR_MOVE(Trackable);
};

template <typename TrackableType>
class COMP_API Tracker
{
public:
    explicit Tracker()
    {
        static_assert(is_pointer_v<TrackableType> || is_shared_ptr_v<TrackableType> || is_weak_ptr_v<TrackableType>, "Tracker works only with pointer types");
    }

    ~Tracker()
    {
        disconnectTrackables();
    }

    void track(TrackableType trackable)
    {
        m_trackables.push_back(trackable);
    }

    void untrack(TrackableType trackable)
    {
        erase_first(m_trackables, trackable);
    }

    void disconnectTrackables()
    {
        m_trackables.clear();
    }

protected:

    COMP_DISABLE_COPY_OR_MOVE(Tracker);
    struct InvalidateTrackable
    {
        void operator()(TrackableType& trackable)
        {
            trackable->disconnect();
            if constexpr (is_pointer_v<TrackableType>)
            {
                trackable = nullptr;
            }
            else
            {
                trackable.reset();
            }
        }
    };
    ZeroSafeContainer<TrackableType, detail::NullCheck<TrackableType>, InvalidateTrackable> m_trackables;
};

}} // comp::trackers

#endif // COMP_TRACKER_HPP
