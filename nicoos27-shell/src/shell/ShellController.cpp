#include "nicoos/ShellController.hpp"

namespace nicoos {

ShellController& ShellController::instance() {
    static ShellController inst;
    return inst;
}

} // namespace nicoos
