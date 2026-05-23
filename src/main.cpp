#include "app/app.hpp"
#include "app/bloom.hpp"
#include "app/chroma.hpp"
#include "app/convolution.hpp"
#include "app/crosshair.hpp"
#include "app/interactive_lights.hpp"
#include "app/objects/island.hpp"
#include "app/objects/glijbaan.hpp"
#include "app/objects/lights.hpp"
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
    app.addInitSystem(InitStage::Setup, glijbaan::setupSystem);
    app.addInitSystem(InitStage::Setup, chroma::setupSystem);
    app.addInitSystem(InitStage::Setup, lights::setupSystem);
    app.addInitSystem(InitStage::Setup, interactive_lights::setupSystem);
    app.addInitSystem(InitStage::Setup, bloom::setupSystem);
    app.addInitSystem(InitStage::Setup, convolution::setupSystem);
    app.addInitSystem(InitStage::Setup, crosshair::setupSystem);
}

void setupLoopSystems(App& app) {
    // Events
    app.addLoopSystem(LoopStage::PollEvents, registry::eventResetSystem);
    app.addLoopSystem(LoopStage::PollEvents, window::pollPlatformEventsSystem);

    // Updating
    app.addLoopSystem(LoopStage::Update, camera::switchSystem);
    app.addLoopSystem(LoopStage::Update, camera::inputSystem);
    app.addLoopSystem(LoopStage::Update, glijbaan::inputSystem);
    app.addLoopSystem(LoopStage::Update, glijbaan::rideSystem);
    app.addLoopSystem(LoopStage::Update, chroma::inputSystem);
    app.addLoopSystem(LoopStage::Update, island::updateWaterTimeSystem);
    app.addLoopSystem(LoopStage::Update, bloom::inputSystem);
    app.addLoopSystem(LoopStage::Update, convolution::inputSystem);
    // Picking runs after camera::inputSystem so it sees the up-to-date
    // cursorCaptured flag; only fires when the cursor is captured.
    app.addLoopSystem(LoopStage::Update, interactive_lights::inputSystem);
    app.addLoopSystem(LoopStage::Update, camera::syncCamerasFromEntitiesSystem);
    app.addLoopSystem(LoopStage::Update, camera::updateMatricesSystem);

    // Rendering
    app.addLoopSystem(LoopStage::Render, window::clearWindowSystem);
    app.addLoopSystem(LoopStage::Render, bloom::beginScenePassSystem);
    app.addLoopSystem(LoopStage::Render, convolution::beginScenePassSystem);
    app.addLoopSystem(LoopStage::Render, rendering::prepareRenderStateSystem);
    app.addLoopSystem(LoopStage::Render, rendering::gatherCullSortDrawablesSystem);
    app.addLoopSystem(LoopStage::Render, rendering::drawOpaqueMeshesSystem);
    app.addLoopSystem(LoopStage::Render, rendering::drawTransparentMeshesSystem);
    app.addLoopSystem(LoopStage::Render, rendering::drawWireFrameMeshesSystem);
    // Sphere lights draw before chroma + bloom so they end up in the HDR scene
    // FBO and bloom their emissive color.
    app.addLoopSystem(LoopStage::Render, interactive_lights::renderSystem);
    app.addLoopSystem(LoopStage::Render, chroma::renderSystem);
    app.addLoopSystem(LoopStage::Render, bloom::postProcessSystem);
    app.addLoopSystem(LoopStage::Render, convolution::postProcessSystem);
    app.addLoopSystem(LoopStage::Render, crosshair::renderSystem);
    app.addLoopSystem(LoopStage::Render, rendering::endRenderStateSystem);

    // End
    app.addLoopSystem(LoopStage::EndFrame, window::presentSystem);

    // Debug
    // app.addLoopSystem(LoopStage::Debug, camera::debugPrintCameraSystem);
    // app.addLoopSystem(LoopStage::EndFrame, camera::coordDebugSystem);  // glijbaan control-point
    // capture
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
