#ifndef SYWU_CONNECTION_HPP
#define SYWU_CONNECTION_HPP

#include <config.hpp>
#include <sywu/extras.hpp>
#include <sywu/guards.hpp>
#include <sywu/type_traits.hpp>

namespace sywu
{

class SignalConcept;

struct Tracker;
using TrackerPtr = std::unique_ptr<Tracker>;

class Slot;
using SlotPtr = std::shared_ptr<Slot>;
using SlotWeakPtr = std::weak_ptr<Slot>;

/// The Slot holds the invocable connected to a signal. The slot hosts a function, a function object, a method
/// or an other signal.
class SYWU_API Slot : public Lockable, public std::enable_shared_from_this<Slot>
{
    SYWU_DISABLE_COPY_OR_MOVE(Slot);

public:
    /// Destructor.
    virtual ~Slot() = default;

    /// Returns the enabled state of a slot.
    /// \return If the slot is enabled, returns \e true, otherwise returns \e false.
    bool isEnabled() const
    {
        return m_isEnabled.load();
    }
    /// Sets the enabled state of a slot.
    /// \param enable The enabled state to set for the slot.
    void setEnabled(bool enable)
    {
        m_isEnabled.store(enable);
    }

    /// Checks the validity of a slot.
    /// \return If the slot is valid, returns \e true, otherwise returns \e false.
    bool isValid() const
    {
        auto predicate = [](auto& tracker)
        {
            return !tracker->isValid(tracker.get());
        };
        const auto it = utils::find_if(m_trackers, predicate);
        if (it != m_trackers.end())
        {
            return false;
        }
        return isValidOverride();
    }

    /// Disconnects a slot.
    void disconnect()
    {
        lock_guard lock(*this);
        m_trackers.clear();
        disconnectOverride();
    }

    /// Binds a trackable object to the slot. The trackable object is either a shared pointer, a weak pointer,
    /// or a Trackable derived object.
    /// \tparam TrackableType The type of the trackable, a shared_ptr, weak_ptr, or a pointer to the Trackable
    ///         derived object.
    template <class TrackableType>
    void bind(TrackableType trackable);

protected:
    /// The container with the binded trackers.
    using TrackersContainer = std::vector<TrackerPtr>;

    /// Constructor.
    explicit Slot() = default;

    /// To implement slot specific validation, override this method.
    virtual bool isValidOverride() const = 0;
    /// To implement slot specific disconnect, override this method.
    virtual void disconnectOverride() = 0;

    /// The binded trackers.
    TrackersContainer m_trackers;
    /// The enabled state of the slot.
    std::atomic_bool m_isEnabled = true;
};

///The SlotImpl declares the activation of a slot.
template <typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API SlotImpl : public Slot
{
public:
    /// Activates the slot with the arguments passed, and returns the slot's return value.
    ReturnType activate(Arguments&&...);

protected:
    /// To implement slot specific activation, override this method.
    virtual ReturnType activateOverride(Arguments&&...) = 0;
};

/// The Connection holds a slot connected to a signal. It is a token to a sender signal and a receiver
/// slot connected to that signal.
class SYWU_TEMPLATE_API Connection
{
    template <typename, typename, typename...>
    friend class SignalConceptImpl;

public:
    /// Constructor.
    Connection() = default;
    \
    /// Constructs the connection with the \a sender signal.
    Connection(SignalConcept& sender, SlotPtr slot)
        : m_sender(&sender)
        , m_slot(slot)
    {
    }

    /// Destructor.
    ~Connection() = default;

    /// Disconnect the connection from the signal.
    void disconnect()
    {
        auto slot = m_slot.lock();
        SYWU_ASSERT(slot);
        slot->disconnect();
        m_slot.reset();
        m_sender = nullptr;
    }

    /// Returns the sender signal of the connection.
    /// \return The sender signal. If the connection is invalid, returns \e nullptr.
    SignalConcept* getSender() const
    {
        return m_sender;
    }

    /// Returns the sender signal of the connection.
    /// \tparam SignalType The type fo the signal.
    /// \return The sender signal. If the connection is invalid, or the signal type differs, returns \e nullptr.
    template <class SignalType>
    SignalType* getSender() const
    {
        return static_cast<SignalType*>(m_sender);
    }

    /// Returns the valid state of the connection.
    /// \return If the connection is valid, returns \e true, otherwise returns \e false. A connection is invalid when its
    /// source signal or its trackers are destroyed.
    bool isValid() const
    {
        const auto slot = m_slot.lock();
        if (!slot || !m_sender)
        {
            return false;
        }
        return slot->isValid();
    }

    /// Binds trackables to the slot.
    /// \param trackables... The trackables to bind.
    template <class... Trackables>
    Connection& bind(Trackables... trackables);

protected:

    SignalConcept* m_sender = nullptr;
    SlotWeakPtr m_slot;
};

/// To track the lifetime of a connection based on an arbitrary object that is not a smart pointer, use this class.
/// You can use this class as a base class
class SYWU_API Trackable
{
public:
    /// Destructor.
    virtual ~Trackable()
    {
        disconnectSlot();
    }

    /// Override this method to prevent the object from being deleted.
    virtual bool retain()
    {
        return true;
    }

    /// Override this method to release the retained object.
    virtual void release()
    {
    }

    /// Checks whether the trackable has a slot attached.
    bool isAttached() const
    {
        return m_slot != nullptr;
    };

    /// Attaches a \a slot to the trackable.
    void attach(SlotPtr slot)
    {
        m_slot = slot;
    }

    /// Detaches the slot from the trackable.
    void detach()
    {
        m_slot.reset();
    }

protected:
    /// Constructor.
    explicit Trackable() = default;

    /// Disconnects the attached slot. Call thsi method if you want to disconnect from the attached slot
    /// earlier than at the trackable destruction time.
    void disconnectSlot()
    {
        if (m_slot)
        {
            m_slot->disconnect();
            m_slot.reset();
        }
    }

private:
    SlotPtr m_slot;
};

} // namespace sywu

#endif // SYWU_CONNECTION_HPP
