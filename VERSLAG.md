# Verslag - Computer Graphics Project
- **Cursus:** Computer Graphics & Visual Computing 
- **Team:** Tarudahat, mathijs-follon 
- **Applicatie:** `CG_OpenGL_Project` (C++23, OpenGL 4.6 Core) 
- **Demo:** [youtube video](https://www.youtube.com/watch?v=W0UpU7VaEPI)

Dit verslag beschrijft per GitHub-issue wat er is geïmplementeerd en **hoe** dat technisch werkt. De tekst is in het Nederlands; vaktermen (shader, framebuffer, uniform, …) blijven in het Engels zoals gebruikelijk in graphics-documentatie.

---

## Overzicht van de eindscene

De demo toont een **glijbaan-scène** rond het Sketchfab-model *Sea Keep “Lonely Watcher”*: eiland met toren, pier, zee en thematische objecten (onder andere een dinosaurusrider op de glijbaan). De engine draait als een **pipeline van systemen** (init --> loop --> shutdown) met een centrale `Registry` voor state en scene-objecten.

**Belangrijkste render-volgorde per frame** (`src/main.cpp`):

1. Window clearen (of FBO clearen als bloom/convolution actief is)
2. Scene naar HDR-FBO (bloom) of scene-FBO (convolution) of direct naar scherm
3. Opaque meshes --> transparant water --> debug wireframe (curve)
4. Interactieve lichtbollen (emissive)
5. Chroma-key overlay
6. Bloom- of convolution-post-processing
7. Crosshair op het default framebuffer

**Bediening (selectie):** WASD + muis (free-roam), **C** (camera wisselen), **G/R/V** (glijbaan), **B** (bloom), **P** (convolution), **L** (chroma), linkermuisklik op lichtbollen (picking).

---

## Milestone 1 - Engine Skeleton & Asset Pipeline

### Issue #1 - Group Registration (process)

**Status:** Afgerond.

Registratie van de projectgroep en afspraken over verdeling en planning. De samenwerking is vastgelegd in GitHub-milestones, session logs (`logging/taru.md`, `logging/mathijs.md`) en issues #2–#20.

---

### Issue #2 - Project Architecture & Build System

**Status:** Afgerond.

**Hoe:**

- **CMake 3.30+** bouwt één executable `CG_OpenGL_Project` met C++23 (`CMakeLists.txt`).
- Dependencies: **GLFW**, **GLAD** (OpenGL loader), **GLM**, **spdlog**, **Assimp** (modellen), **stb_image** (textures). FetchContent of lokale `third_party/`.
- Na de build worden **`assets/`** naar de outputmap gekopieerd (shaders, modellen, textures).
- **Architectuur:**
  - `App` (`src/app/app.hpp`) voert drie pipelines uit: `InitStage`, `LoopStage`, `ShutdownStage`.
  - Elk “systeem” is een functie `void(Registry&)` die in `main.cpp` wordt geregistreerd.
  - `Registry` (`src/world/registry.hpp`) bewaart named objects en events in `std::any`.
- CI: clang-format en multi-platform CMake workflows (`.github/workflows/`).

Dit scheidt **wat** er per frame gebeurt (stages) van **data** (registry), zodat features (bloom, glijbaan, …) los toegevoegd kunnen worden.

---

### Issue #3 - Window & Input Context

**Status:** Afgerond.

**Hoe:** `src/graphics/window.hpp`

- **GLFW** initialiseert een 1600×900 venster, OpenGL **4.6 Core**, vsync (`glfwSwapInterval(1)`).
- **GLAD** laadt functiepointers na `glfwMakeContextCurrent`.
- `pollPlatformEventsSystem` leest events, sluit bij `glfwWindowShouldClose`, en vult `App::Time` (`deltaTime` voor animatie en beweging).
- `clearWindowSystem` / `presentSystem` clearen en swappen buffers.

Input voor camera en gameplay gebeurt in aparte systemen (camera, glijbaan, …) die dezelfde `GLFWwindow*` uit de registry gebruiken.

---

### Issue #4 - GLM Integration & Base Camera

**Status:** Afgerond.

**Hoe:** `src/graphics/camera.hpp`, `src/graphics/geometry.hpp`

- **GLM** voor vectoren, matrices, `lookAt`, `perspective`.
- `Camera` bevat positie, yaw/pitch, `viewMatrix`, `projMatrix`, `viewProjMatrix` en een **view-frustum** voor culling.
- **Free-roam:** muis past yaw/pitch aan; WASD beweegt horizontaal t.o.v. kijkrichting; spatie/shift omhoog/omlaag; Ctrl = sneller.
- **Cursor:** standaard vastgezet (`GLFW_CURSOR_DISABLED`); Esc geeft cursor vrij; linkermuisklik vangt opnieuw.
- `updateMatricesSystem` herberekent view/projection per frame op basis van framebuffer-aspect.

De frustum (`Frustum::fromViewProjection`) wordt gebruikt in `rendering::gatherCullSortDrawablesSystem` om meshes buiten beeld te skippen.

---

### Issue #5 - Texture Loader (stb_image)

**Status:** Afgerond.

**Hoe:** `src/asset/texture.cpp`

- **stb_image** laadt PNG/JPG; gebruik`stbi_set_flip_vertically_on_load(true)` voor correcte verticale OpenGL-UV mapping.
- Intern format afhankelijk van kanaalaantal (R8, RG8, RGB8, RGBA8).
- Mipmaps: `glGenerateMipmap`, `GL_LINEAR_MIPMAP_LINEAR`, repeat wrapping.
- Fouten loggen via spdlog; loader retourneert `std::optional<Texture>`.

Textures worden gekoppeld aan material slots bij model load en aan overrides (glijbaan-texture, chroma-plaatje).

---

### Issue #6 - Model Loader (Assimp)

**Status:** Afgerond.

**Hoe:** `src/asset/model.cpp`, `src/asset/render_object_spawner.cpp`

- Modellen laden via **Assimp** met flags: triangulate, gen normals, tangent space, cache locality.
- Per mesh: posities, normals, UV’s, indices; **world transform** uit de scene graph wordt op vertices **gebakken** (inclusief correcte normal matrix).
- Materialen: diffuse/emissive kleur, opacity, shininess, texture-paden (diffuse, normal, specular, …) opgelost relatief t.o.v. het modelbestand.
- `spawnModelAsRenderMeshes` zet meshes om naar `RenderMeshInstance` met VAO/VBO/EBO en shader-uniform locaties.

**Gebruikte assets:** o.a. `assets/models/sea_keep/scene.gltf`, `assets/models/dinosaur/scene.gltf`, Utah teapot (vroege tests).

---

## Milestone 2 - Bezier Curves, Splines & Track Generation

### Issue #7 - Bezier Curve Math

**Status:** Afgerond.

**Hoe:** `src/graphics/geometry.hpp` - struct `CubicBezierCurve`

- Kubische Bézier met **Bernstein-basismatrix** `basis` en control points als `glm::mat4x3`.
- Evaluatie: `pointAt(t)`, `tangentAt(t)` via machten van \(t\).
- **Forward differencing** (`sampleFD`) samplet de curve en bouwt tegelijk een **arc-length LUT**: map `(afstand --> parameter t)`.
- Optioneel: `frenetFrameAt(t)` (tangent, normal, binormal) - zie issue #11.

**Glijbaan:** `src/app/objects/glijbaan.hpp` zet **Catmull-Rom-knots** om naar cubic Bézier-segmenten (`catmullRomToBezier`) voor \(C^1\) spline-gedrag over 8 segmenten.

---

### Issue #8 - Debug Curve Visualizer

**Status:** Afgerond.

**Hoe:** `glijbaan::setupSystem` + shaders `raw_vert.vert` / `raw_vert.frag`

- Centerline-samples (`buildCenterLineSamples`, 64 samples per segment) worden als **`GL_LINES`** gerenderd (debug layer `RenderLayer::DebugOverlay`).
- **Shader:** alleen `u_mvp` / `u_model`; fragment shader = vast rood (`vec4(1,0,0,1)`).
- Toets **V** togglet zichtbaarheid (`showCurveDebug`).

Zo kan de spline gecontroleerd worden vóór/naast de half-pipe geometrie.

---

### Issue #9 - Arc-Length Parameterization (Constant Speed)

**Status:** Afgerond.

**Hoe:** `CubicBezierCurve::arcLengthLUT` + `tvalueForDistance` + `glijbaan::sampleAtDistance`

- Tijdens sampling wordt kumulatieve booglengte bijgehouden; LUT mapt **boogafstand \(d\)** --> **curveparameter \(t\)** (lineaire interpolatie tussen LUT-entries).
- `rideSystem` verhoogt `rideDistance` met `rideSpeed * deltaTime` (constant in **meter per seconde**, niet per \(t\)).
- `wrapArcLength` maakt de baan cyclisch.

Handmatig rijden (W/S in slide-camera) gebruikt dezelfde parametrisatie.

---

### Issue #10 - Dynamic Track Geometry

**Status:** Afgerond.

**Hoe:** `glijbaan::buildHalfPipeMesh` in `glijbaan.hpp`

- Langs de centerline wordt per station een **lokaal frame** gezet: tangent, `slideUp` (wereld +Y geprojecteerd), `slideRight` (`computeFrameAt`).
- Dwarsdoorsnede: halve cilinder (half-pipe) met straal `kHalfPipeRadius`, wanddikte `kHalfPipeWallThickness`, `kHalfPipeRingSegments` ringen.
- Twee lagen vertices (binnen + buiten) + zijwanden; UV’s op basis van booglengte langs de baan en boog langs de dwarsdoorsnede (tiling `kGlijbaanTextureTileWorldUnits`).
- Mesh wordt gespawned met `default.vert` / `default.frag` en texture `assets/textures/glijbaan.png`.

De **dinosaurusrider** volgt `rideDistance` via `seatPositionOnPath` en `orientationFromTrackFrame` (up = track normal, forward = tangent).

---

### Issue #11 - Object Orientation (Frenet Frame)

**Status:** Niet gepland (overgeslagen).

**Toelichting:** In `CubicBezierCurve` bestaat `frenetFrameAt` nog als API, maar de glijbaan en rider gebruiken bewust een **vereenvoudigd frame** (`computeFrameAt`: tangent + wereld-up), stabieler bij bijna-lodere tangents dan klassieke Frenet-Serret.

Orientatie rider: `orientationFromTrackFrame` + optionele yaw/pitch-correctie op het model (`SlideRiderEntity`).

---

## Milestone 3 - Advanced Shaders & Framebuffers

### Issue #12 - Per-Pixel Lighting

**Status:** Afgerond.

**Hoe:** `assets/shaders/default.vert`, `assets/shaders/default.frag`, `src/app/objects/lights.hpp`

**Vertex shader (`default.vert`):**

- Wereldpositie `v_frag_pos`, wereldnormaal `v_worldNormal` (normal matrix), `v_uv`.

**Fragment shader (`default.frag`):**

- Struct `Material` (enkel shininess gebruikt, material sampling onnodig aangezien modellen deze niet hebben) + array `PointLight u_lights[16]` (max `LIGHT_COUNT_MAX`).
- Per licht (indien `on == 1`): ambient, diffuse (Lambert), specular (Phong, `shininess`), met **distance attenuation** \(1 / (1 + k_l d + k_q d^2)\).
- Albedo uit `sampler2D u_albedo`; alpha &lt; 0.5 --> `discard` (bladeren/fences).
- Basis stilistische ambient `vec3(0.18)` (onafhankelijk van attenuation) plus opgetelde lichten, vermenigvuldigd met albedo.

**CPU:** `lights::setupSystem` zet slot 0 (zon-achtig puntlicht ver weg). `interactive_lights` vult slots 1–5 dynamisch.

---

### Issue #13 - FBO Setup & Screen Quad

**Status:** Afgerond.

**Hoe:** `src/graphics/fbo.hpp`, `src/graphics/screen_quad.hpp`, `assets/shaders/screen_quad.vert`

- `SceneFbo`: kleurtexture **RGBA16F**, depth renderbuffer **DEPTH_COMPONENT24**, completeness check.
- `ScreenQuad`: VAO met NDC-posities \([-1,1]\) en UV’s; `drawScreenQuad` voor fullscreen passes.
- **`screen_quad.vert`:** deelt vertex-shader voor bloom, convolution en simpele texture-blit; `v_uv` voor sampling.

Post-process passes schrijven eerst naar FBO’s, daarna een quad naar framebuffer 0.

---

### Issue #14 - Convolution Post-Processing

**Status:** Afgerond.

**Hoe:** `src/app/convolution.hpp`

- Toets **P** cyclust: uit --> **Gaussian blur** --> **Laplacian edge detection**.
- Scene rendert naar `sceneFbo` (`beginScenePassSystem`).
- **Gaussian:** hergebruikt `bloom_blur.frag` - separable 5-tap kernel met vaste gewichten, 3 ping-pong passes (horizontaal/verticaal).
- **Laplacian:** `laplacian_edge.frag` - 5×5 kernel (klassische sharpen/edge kernel met centrale +16).
- Resultaat naar scherm via `screen_quad.frag` (texture blit).

**Shader `laplacian_edge.frag`:** nested loops over 5×5 offsets met `u_texelSize`.

---

### Issue #15 - Bloom / Neon Effect Pipeline

**Status:** Afgerond.

**Hoe:** `src/app/bloom.hpp`

- Toets **B** schakelt bloom in/uit.
- **Pipeline:**
  1. Scene --> HDR `sceneFbo` (RGBA16F).
  2. **Bright pass** (`bloom_bright.frag`): luminantie threshold + soft knee (`smoothstep`).
  3. **Blur** (`bloom_blur.frag`): separable Gaussian, 5 iteraties ping-pong op halve resolutie (`kBloomBufferScale = 0.5`).
  4. **Composite** (`bloom_composite.frag`): `scene + u_intensity * bloom`.

Emissive objecten (lichtbollen met hoge kleurwaarden) dragen bij aan het bloom-masker; interactieve bollen worden vóór chroma getekend zodat ze in de scene-FBO zitten.

---

## Milestone 4 - Interaction, Chroma-key & Camera Polish

### Issue #16 - Chroma-keying (YCbCr)

**Status:** Afgerond.

**Hoe:** `src/app/chroma.hpp`, `assets/shaders/chroma.vert`, `assets/shaders/chroma.frag`

- Toets **L:** hidden --> raw overlay --> chroma-keyed.
- Fullscreen quad met `assets/textures/chroma_key.jpg`.
- **`chroma.frag`:**
  - `rgbToYCbCr` (BT.601-achtige coëfficiënten).
  - Key-kleur (default groen) omgezet naar YCbCr; afstand in **CbCr-vlak** (`distance(pixel.yz, key.yz)`).
  - Onder threshold: `discard`; tussen threshold en threshold+softness: `smoothstep` op alpha.
- Raw mode: `u_enableKey == false`, volledige overlay zonder discard.

Depth uit, alpha blending aan - groene achtergrond verdwijnt, scene schijnt door.

---

### Issue #17 - Interaction via Raycasting / Picking

**Status:** Afgerond.

**Hoe:** `src/app/interactive_lights.hpp`

- Vijf **transparante emissive spheres** op vaste posities langs de glijbaan; elk gekoppeld aan point-light slot 1–5.
- **Picking:** bij linkermuisklik (alleen als cursor captured) ray van `camera.position` langs `normalize(camera.front)`.
- **Ray-sphere intersectie:** kwadratische vergelijking; kleinste positieve \(t\) binnen `farZ` wint.
- Treffer --> `colorIndex` cyclisch door palette (wit, rood, oranje, geel, groen, cyaan, magenta); `writeLightSlot` past alle geladen `default.frag`-programma’s aan.

**Render:** `emissive.frag` - uniforme kleur × `kEmissiveBoost`, alpha 0.55, gesorteerd back-to-front.

---

### Issue #18 - First-Person Ride Camera

**Status:** Afgerond.

**Hoe:** `camera.hpp`, `players.hpp` (`SlideRiderEntity`), `glijbaan::rideSystem`

- Twee camera’s in `CameraState`: `FreeRoam` en `SlideFollow`; **C** wisselt.
- Slide-camera: positie = rider + `trackNormal * eyeHeight + forward * eyeForward`; richting = `rider.forward` (tangent); FOV uit rider (`fovYDeg` ≈ 120° voor wide ride-gevoel).
- `syncCamerasFromEntitiesSystem` kopieert entiteiten naar camera matrices vóór `updateMatricesSystem`.

Glijbaan-input: **G** auto-ride, **R** reset, **W/S** handmatig in ride-modus, pijltjes snelheid.

---

## Milestone 5 - The Ace It Phase

### Issue #19 - Thematic Dressing & Polish

**Status:** Afgerond.

**Hoe:** `src/app/objects/island.hpp`, water shaders, crosshair, scene composition

**Eiland (`island.hpp`):**

- Sea Keep-model met schaal/rotatie/positie; meshes via `spawnModelAsRenderMeshes`.
- Meshes met naam prefix `"Sea"` krijgen **water shader** en `RenderLayer::Transparent`.

**Water (`water.vert` / `water.frag`):**

- Vertex: sinusgolf op Y met `u_time` voor subtiele deining.
- Fragment: eenvoudige directionele lighting + **alpha 0.55**; depth write uit tijdens transparante pass.

**Crosshair (`crosshair.hpp`, `crosshair.vert` / `crosshair.frag`):**

- Klein NDC-quad; fragment tekent “+” met `discard` buiten armen (gap in het midden voor FPS-stijl).

**Thema:** glijbaan + dino-rider + zee-eiland + neon-achtige lichtbollen + post-processing maken een afgeronde demo-scène (zie ook `docs/images/` en `SCRIPT.md` voor presentatievolgorde).

---

## Shader-overzicht

| Bestand | Rol |
|---------|-----|
| `default.vert` / `default.frag` | Lit textured meshes, supports max 16 point lights |
| `water.vert` / `water.frag` | Geanimeerd transparant water |
| `raw_vert.vert` / `raw_vert.frag` | Rode debug-lijnen (curve) |
| `emissive.frag` | Onverlichte emissive kleur (lichtbollen) |
| `screen_quad.vert` | Fullscreen pass UV’s |
| `screen_quad.frag` | Eenvoudige texture copy |
| `bloom_bright.frag` | Extract heldere pixels |
| `bloom_blur.frag` | Separable Gaussian blur |
| `bloom_composite.frag` | Scene + bloom |
| `laplacian_edge.frag` | 5×5 edge filter |
| `chroma.vert` / `chroma.frag` | YCbCr chroma key overlay |
| `crosshair.vert` / `crosshair.frag` | HUD-vizier |

Alle shaders: `#version 460 core`.

---

## Samenvatting per milestone

| Milestone | Issues | Kernresultaat |
|-----------|--------|----------------|
| 1 | #1–#6 | CMake-engine, window, GLM-camera, asset loaders |
| 2 | #7–#10 (#11 skip) | Spline-glijbaan, booglengte, half-pipe mesh, debug curve |
| 3 | #12–#15 | Phong lighting, FBO + quad, bloom, convolution |
| 4 | #16–#18 | Chroma key, picking, ride-camera |
| 5 | #19 | Island, water, crosshair, scene polish |

---

## Build & run

```bash
cmake -S . -B build
cmake --build build
./build/CG_OpenGL_Project
```

Assets staan automatisch na build in `build/assets/`. Zie `README.md` voor CI en credits (Sketchfab-modellen).

---

## Reflectie

- **Sterk:** modulaire pipeline, duidelijke scheiding van post-process passes, hergebruik van `bloom_blur.frag` voor convolution, constante snelheid op de glijbaan via arc-length LUT.
- **Beperkingen:** Convolution en bloom hebben elk een eigen scene-FBO (niet tegelijk gecombineerd in één knop); picking is beperkt tot lichtbollen (geen algemene mesh picking).

**Totale ontwikkeltijd (logs):** ca. ~40 uur per teamlid verspreid over april–mei 2026 (zie `logging/`).
