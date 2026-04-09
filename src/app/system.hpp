#ifndef CG_OPENGL_PROJECT_SYSTEM_HPP
#define CG_OPENGL_PROJECT_SYSTEM_HPP
#include <functional>

class Registry;
using SystemFn = std::function<void(Registry&)>;

#endif  // CG_OPENGL_PROJECT_SYSTEM_HPP
