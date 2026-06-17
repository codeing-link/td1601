#include <stdint.h>
#include <drv/tick.h>

extern "C" uint32_t millis(void);

namespace std {
namespace chrono {
inline namespace _V2 {
struct system_clock {
    typedef long long rep;
    typedef struct { } period;
    typedef struct { rep count; } duration;
    struct time_point {
        duration d;
    };
    static time_point now() noexcept {
        time_point tp;
        tp.d.count = (rep)millis();
        return tp;
    }
};
}
}
}

void operator delete(void* ptr, unsigned int) noexcept {
    (void)ptr;
}
