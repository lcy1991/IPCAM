#ifndef LIGHTREFBASE_H
#define LIGHTREFBASE_H

#include <stdint.h>
#ifdef ANDROID_NDK
#include <sys/atomics.h>
#define _atomic_inc_(x)  __atomic_inc(x)
#define _atomic_dec_(x)  __atomic_dec(x)
#else
#define _atomic_inc_(x) ( __sync_fetch_and_add (x, 1))   //gcc default atomic
#define _atomic_dec_(x) ( __sync_fetch_and_sub (x, 1))
#endif

template <class T>
class LightRefBase
{
public:
    inline LightRefBase() : mCount(0) { }
    inline void incStrong(const void* id) const {
            _atomic_inc_(&mCount);
    }
    inline void decStrong(const void* id) const {
        if (_atomic_dec_(&mCount) == 1) {
            delete static_cast<const T*>(this);
        }
    }
    //! DEBUGGING ONLY: Get current strong ref count.
    inline int32_t getStrongCount() const {
        return mCount;
    }

    typedef LightRefBase<T> basetype;

protected:
    inline ~LightRefBase() { }
	
private:
    mutable volatile int32_t mCount;
};
#endif
