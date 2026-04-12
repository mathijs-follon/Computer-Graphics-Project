#include "app/app.hpp"
#include "graphics/camera.hpp"
#include "graphics/window.hpp"

void setupInitSystems(App& app) {
    // Setup logger
    app.addInitSystem(InitStage::Setup, logger::loggerSetupSystem);

    // Setup window
    app.addInitSystem(InitStage::Setup, window::setupSystem);
    app.addInitSystem(InitStage::Setup, camera::setupSystem);

}

void setupLoopSystems(App& app) {

    // Events
    app.addLoopSystem(LoopStage::PollEvents, registry::eventResetSystem);
    app.addLoopSystem(LoopStage::PollEvents, window::pollPlatformEventsSystem);


    // Updating
    app.addLoopSystem(LoopStage::Update, camera::switchSystem);
    app.addLoopSystem(LoopStage::Update, camera::inputSystem);
    app.addLoopSystem(LoopStage::Update, camera::syncCamerasFromEntitiesSystem);
    app.addLoopSystem(LoopStage::Update, camera::updateMatricesSystem);


    // Rendering
    app.addLoopSystem(LoopStage::Render, window::clearWindowSystem);


    // End
    app.addLoopSystem(LoopStage::EndFrame, window::presentSystem);

    // Debug
    app.addLoopSystem(LoopStage::Debug, camera::debugPrintCameraSystem);
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
