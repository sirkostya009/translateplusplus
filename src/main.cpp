#include "App.hpp"
#include "AssetsProvider.hpp"

auto main() -> int {
//    ul::Platform::instance().set_file_system(new AssetsProvider);

    App().run();
}
