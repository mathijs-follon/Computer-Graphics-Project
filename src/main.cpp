#include "app/app.hpp"
#include "graphics/window.hpp"

void setupInitSystems(App& app) {
    // Setup logger
    app.addInitSystem(InitStage::Setup, logger::loggerSetupSystem);

    // Setup window
    app.addInitSystem(InitStage::Setup, window::setupSystem);
}

void setupLoopSystems(App& app) {
    app.addLoopSystem(LoopStage::PollEvents, window::pollPlatformEventsSystem);
    app.addLoopSystem(LoopStage::Render, window::renderSystem);
    app.addLoopSystem(LoopStage::EndFrame, window::presentSystem);
}

void setupShutdownSystems(App& app) {
    app.addShutdownSystem(ShutdownStage::Cleanup, window::shutdownSystem);
}

int main() {
    App app;

    setupInitSystems(app);
    setupLoopSystems(app);
    setupShutdownSystems(app);

    app.run();

    return EXIT_SUCCESS;
}
