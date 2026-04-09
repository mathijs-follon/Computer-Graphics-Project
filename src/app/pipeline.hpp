#ifndef CG_OPENGL_PROJECT_PIPELINE_HPP
#define CG_OPENGL_PROJECT_PIPELINE_HPP
#include <map>
#include <type_traits>
#include <vector>

#include "system.hpp"

enum class LoopStage {
    PollEvents,
    Update,
    Render,
    EndFrame,
};

enum class InitStage {
    Setup,
};

enum class ShutdownStage {
    Cleanup,
};

struct EnumClassCompareLess {
    template <typename T>
    bool operator()(T left, T right) const noexcept {
        static_assert(std::is_enum_v<T>);
        using Underlying = std::underlying_type_t<T>;
        // We gebruiken de orde als gedefinieerd in enum class
        return static_cast<Underlying>(left) < static_cast<Underlying>(right);
    }
};

template <typename T>
class Pipeline {
public:
    Pipeline() = default;

    void addSystem(T stage, SystemFn fn) { m_stages[stage].push_back(std::move(fn)); }

    void executePipeline(Registry& registry) {
        for (auto& [stage, systems] : m_stages) {
            for (auto& fn : systems) {
                fn(registry);
            }
        }
    }

private:
    std::map<T, std::vector<SystemFn>, EnumClassCompareLess> m_stages;
};

#endif  // CG_OPENGL_PROJECT_PIPELINE_HPP