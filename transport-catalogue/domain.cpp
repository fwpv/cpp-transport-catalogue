#include "domain.h"

#include <algorithm>

namespace tcat {

int Bus::StopCount() const {
    if (is_roundtrip) {
        return static_cast<int>(stops.size());
    } else {
        return static_cast<int>(stops.size() * 2 - 1);
    }
}

int Bus::CountUniqueStops() const {
    std::vector<const Stop*> tmp {stops.begin(), stops.end()};
    std::sort(tmp.begin(), tmp.end());
    auto last = std::unique(tmp.begin(), tmp.end());
    return static_cast<int>(last - tmp.begin());
}
}  // namespace tcat