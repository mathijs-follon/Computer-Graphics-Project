#ifndef CG_OPENGL_PROJECT_APP_HPP
#define CG_OPENGL_PROJECT_APP_HPP
#include "pipeline.hpp"
#include "world/registry.hpp"

class App {
public:
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

        while (true) {
            m_loopPipeline.executePipeline(m_registry);
            if (m_registry.hasEvent(EventType::ApplicationExitRequest)) {
                break;
            }
            m_registry.flushEvents();
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