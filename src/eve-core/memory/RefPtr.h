#ifndef __UTILS__REF_PTR_H__INCL__
#define __UTILS__REF_PTR_H__INCL__

#include <memory>

/**
 * @brief RefObject — legacy base class.
 * RefPtr now uses std::shared_ptr internally; RefObject is kept as empty base
 * for backward compatibility during migration. New classes should not inherit from it.
 */
class RefObject
{
public:
    RefObject() = default;
    virtual ~RefObject() = default;
    // No-op stubs for legacy code that calls IncRef/DecRef directly
    void IncRef() const {}
    void DecRef() const {}
    uint16 GetCount() { return 0; }
    bool IsDeleted() { return false; }
};

/**
 * @brief RefPtr — now wraps std::shared_ptr.
 * API preserved: get(), operator->, operator*, StaticCast, etc.
 */
template<typename X>
class RefPtr
{
public:
    explicit RefPtr(X* p = nullptr)
    : mPtr(p) {}

    RefPtr(const RefPtr& oth) = default;
    RefPtr(RefPtr&& oth) = default;

    template<typename Y>
    RefPtr(const RefPtr<Y>& oth)
    : mPtr(oth.get()) {}

    template<typename Y>
    RefPtr(const std::shared_ptr<Y>& sp)
    : mPtr(sp) {}

    virtual ~RefPtr() = default;

    RefPtr& operator=(const RefPtr&& oth) = default;
    RefPtr& operator=(const RefPtr& oth) = default;

    template<typename Y>
    RefPtr& operator=(const RefPtr<Y>& oth) {
        mPtr = oth.get();
        return *this;
    }

    X* get() const { return mPtr.get(); }

    explicit operator bool() const noexcept {
        return mPtr != nullptr;
    }

    X& operator*() const { return *mPtr; }
    X* operator->() const { return mPtr.get(); }

    template<typename Y>
    bool operator==(const RefPtr<Y>& oth) const { return (mPtr.get() == oth.get()); }

    template<typename Y>
    static RefPtr StaticCast(const RefPtr<Y>& oth) {
        return RefPtr(std::static_pointer_cast<X>(oth.mPtr));
    }

    std::shared_ptr<X>& operator->() { return mPtr; }

protected:
    std::shared_ptr<X> mPtr;
};

#endif
