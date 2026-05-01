#include "app/app.hpp"
#include "app/objects/island.hpp"
#include "graphics/rendering.hpp"
#include "graphics/camera.hpp"
#include "graphics/window.hpp"
#include "log/log.hpp"

void setupInitSystems(App& app) {
    // Setup logger
    app.addInitSystem(InitStage::Setup, logger::loggerSetupSystem);

    // Setup window
    app.addInitSystem(InitStage::Setup, window::setupSystem);
    app.addInitSystem(InitStage::Setup, camera::setupSystem);
    app.addInitSystem(InitStage::Setup, rendering::setupSystem);
    app.addInitSystem(InitStage::Setup, island::setupSystem);

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
    app.addLoopSystem(LoopStage::Render, rendering::prepareRenderStateSystem);
    app.addLoopSystem(LoopStage::Render, rendering::gatherCullSortDrawablesSystem);
    app.addLoopSystem(LoopStage::Render, rendering::drawOpaqueMeshesSystem);
    app.addLoopSystem(LoopStage::Render, rendering::drawTransparentMeshesSystem);
    app.addLoopSystem(LoopStage::Render, rendering::endRenderStateSystem);


    // End
    app.addLoopSystem(LoopStage::EndFrame, window::presentSystem);

    // Debug
    // app.addLoopSystem(LoopStage::Debug, camera::debugPrintCameraSystem);
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
