#ifndef CG_OPENGL_PROJECT_APP_HPP
#define CG_OPENGL_PROJECT_APP_HPP
#include "pipeline.hpp"
#include "log/log.hpp"
#include "world/registry.hpp"

class App {
public:

    struct Time {
        double lastFrameTime{0.0};
        double currentTime{0.0};
        float deltaTime{0.0};
    };

    App() = default;

    Registry& registry() { return m_registry; }
    const Registry& registry() const { return m_registry; }

    void addLoopSystem(const LoopStage stage, SystemFn fn) {
        m_loopPipeline.addSystem(stage, std::move(fn));
    }

    void addInitSystem(const InitStage stage, SystemFn fn) {
        m_initPipeline.addSystem(stage, std::move(fn));
    }

    void addShutdownSystem(const ShutdownStage stage, SystemFn fn) {
        m_shutdownPipeline.addSystem(stage, std::move(fn));
    }

    void run() {
        m_initPipeline.executePipeline(m_registry);

        while (!m_registry.getFrameEvent(EventType::ApplicationExitRequest)) {
            m_loopPipeline.executePipeline(m_registry);
        }

        m_shutdownPipeline.executePipeline(m_registry);
    }

private:
    Registry m_registry;
    Pipeline<LoopStage> m_loopPipeline;
    Pipeline<InitStage> m_initPipeline;
    Pipeline<ShutdownStage> m_shutdownPipeline;
};

#endif  // CG_OPENGL_PROJECT_APP_HPP