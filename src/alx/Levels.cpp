#include "alx/Levels.h"
#include "assets/Levels.h"

namespace alx::Levels {

const Level* get_level(int id) {
    return Assets::Levels::get_level(id);
}

bool has_level(int id) {
    return Assets::Levels::has_level(id);
}

} // namespace alx::Levels
