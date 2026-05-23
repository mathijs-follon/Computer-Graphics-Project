# Demo-video script — Computer Graphics Project

**Taal:** Nederlands  
**Geschatte lengte:** 8–12 minuten  
**Uitvoerbaar:** `./build/CG_OpenGL_Project` (of `CG_OpenGL_Project.exe` op Windows)  
**Tip:** Zet de console/log zichtbaar als je toetsaanslagen wilt bevestigen (bijv. “Bloom enabled”, “Chroma overlay: chroma-keyed”).

**Team:** Mathijs & Sillard — de demo wordt door **één persoon** opgenomen (**Mathijs**): hij bedient de app, spreekt de voice-over en stelt **beide** teamleden voor. Sillard hoeft niet fysiek aanwezig te zijn; gebruik “wij” waar het over het project gaat.

---

## Rollen (voor opname)

| Rol | Wie | Taak |
|-----|-----|------|
| **Presentator & operator** | Mathijs | Script volgen, applicatie bedienen, opname (scherm + mic) |
| **Team (niet in beeld)** | Sillard | Wordt in intro/afsluiting genoemd; bijdrage via gezamenlijk project |

---

## Snelle referentie — bediening

| Toets / muis | Functie |
|--------------|---------|
| **W A S D** | Vrij rondlopen (free-roam camera) |
| **Spatie / Shift** | Omhoog / omlaag |
| **Ctrl** | Sneller bewegen |
| **Muis** | Kijk rond (cursor vastgezet) |
| **Linkermuisknop** | Cursor vastzetten na Esc |
| **Esc** | Cursor vrijgeven |
| **C** | Wissel camera: vrij ↔ glijbaan (first-person ride) |
| **W / S** (ride-cam) | Vooruit / achteruit over de glijbaan |
| **G** | Auto-ride aan/uit |
| **R** | Rit op glijbaan resetten naar start |
| **Pijl omhoog / omlaag** | Snelheid auto-ride |
| **V** | Debug-curve van glijbaan tonen/verbergen |
| **B** | Bloom aan/uit |
| **P** | Post-processing: uit → Gaussian blur → edge detection (Laplacian) |
| **L** | Chroma-key overlay: uit → raw → YCbCr key |
| **Linkermuisklik** (cursor vast) | Kleur van lichtbol wisselen (raycast naar bol) |

**Demo-tip post-processing:** **B** (bloom) en **P** (convolutie) gebruiken elk een eigen scene-FBO — zet het andere effect **uit** voordat je het volgende demonstreert.

---

## Act 1 — Intro (± 45 sec)

**Beeld:** Titelslide of korte montage van README-screenshots (`docs/images/`).

**Tekst:**

> Hallo, ik ben **Mathijs**. Samen met **Sillard** hebben we dit project gemaakt voor het vak Computer Graphics — in deze demo laat ik ons werk zien.  
> We hebben een interactieve 3D-scène gebouwd in **C++23** en **OpenGL 3.3**, met **GLFW**, **GLAD**, **GLM** en **spdlog**. De engine draait als pipelines met aparte **init-, update-, render- en shutdown-systemen** (ECS-achtige registry).  
> In deze video laat ik de scene, de glijbaan, belichting, post-processing en interactie zien.

**Op scherm (optioneel):** projectnaam, beide namen (Mathijs & Sillard), groepsnummer, datum.

---

## Act 2 — De scene & engine (± 1,5 min)

**Beeld:** Start de applicatie. Begin met een **bird’s-eye** shot: langzaam over het eiland vliegen (WASD + muis).

**Tekst:**

> De hoofdscene is gebaseerd op het Sketchfab-model *Sea Keep “Lonely Watcher”* (`.gltf`): een rotsachtig eiland met toren, pier en zee.  
> Modellen laden we met **Assimp** (ook glTF); textures met **stb_image**. Geen aparte tinygltf-loader — alles loopt via Assimp.  
> Elk mesh wordt een `RenderMeshInstance` in de registry, met eigen VAO/VBO en materiaal uit het model.  
> De render-loop tekent in volgorde: ondoorzichtige meshes → transparant water → wireframe/debug → emissive lichtbollen → chroma-overlay → bloom of convolutie → crosshair.

**Laat zien:**

- Rotsen, toren, pier, begroeiing (textures met alpha: `discard` in `default.frag` bij lage alpha).
- **Water:** meshes waarvan de naam met `Sea` begint krijgen `water.vert` / `water.frag` — semi-transparant (alpha ≈ 0,55), golven via `sin`/`cos` op `u_time` in de vertex shader.
- **Door het water:** modeltexturen (rots/zand) onder het oppervlak; camera laag bij pier of ondiep water.

**Koppeling issues:** #2–#6 (architectuur, window, GLM, loaders), #19 (thematic dressing).

---

## Act 3 — Belichting (± 1 min)

**Beeld:** Beweeg naar een donkere hoek van het eiland, dan naar verlichte plekken.

**Tekst:**

> We gebruiken **per-pixel belichting** in `default.frag`: tot 16 point lights (ambient/diffuse/specular + lineaire/kwadratische attenuatie).  
> Diffuse komt van `u_albedo`; ambient/diffuse/specular/shininess uit het materiaal dat Assimp uit het glTF haalt.

**Laat zien:**

- Highlights en schaduwcontrast op rotsen/toren (geen shadow maps — alleen directe verlichting).
- **Vijf interactieve lichtbollen** (semi-transparante spheres, emissive shader): **linkermuisklik** terwijl de cursor vaststaat — kleur cyclust door wit, rood, oranje, geel, groen, cyaan, magenta.
- Leg kort uit: **ray–sphere intersect** langs de camerarichting; de bol schrijft naar een `u_lights`-slot (slots 1–5, slot 0 is vast).

**Koppeling issues:** #12 (per-pixel lighting), #17 (raycasting/picking).

---

## Act 4 — Glijbaan & curves (± 2 min)

**Beeld:** Vlieg naar de gele half-pipe. Bij start staat de **debug-curve al aan** (gele lijnen); **V** schakelt die uit/aan.

**Tekst:**

> Het spoor loopt langs **kubische Bézier-segmenten**: Catmull-Rom-knopen worden omgezet naar Bézier-control points (`CubicBezierCurve` in `geometry.hpp`).  
> Per segment bouwen we een **booglengte-LUT**; de rit gebruikt afgelegde booglengte, zodat snelheid in wereldruimte gelijk blijft.  
> De gele **half-pipe** is proceduraal gemeshd langs het pad; de **dinosaur-rider** (`assets/models/dinosaur/scene.gltf`) volgt positie en orientatie op het spoor.

**Laat zien:**

- Gele debug-lijnen + half-pipe (**V** om debug te verbergen als je alleen de baan wilt).
- **C** → ride-camera (first-person achter de rider).
- **G** = auto-ride aan/uit (standaard **aan**); **R** = reset naar start; pijltjes = snelheid (20–420).
- **W/S** alleen handmatig rijden als auto-ride **uit** staat én ride-camera actief is.

**Tekst (afsluiting act):**

> Voor de rider gebruiken we raakvector + track-normal (`computeFrameAt`), niet het optionele Frenet-frame in de curve-structuur — dat hebben we voor issue #11 niet ingezet.

**Koppeling issues:** #7–#10 (Bézier, debug, arc-length, track geometry), #18 (ride camera).

---

## Act 5 — Framebuffers & post-processing (± 2,5 min)

**Beeld:** Statisch mooi shot van toren + lichtbollen. Demonstreer **B**, **P** en **L** elk apart (niet bloom + convolutie tegelijk).

### 5a — Bloom

**Zorg dat P uit staat** (geen blur/edges in log). **Druk B** (bloom aan; standaard uit).

**Tekst:**

> Met bloom aan rendert de scene naar een **RGBA16F** framebuffer.  
> Heldere pixels (luma-threshold) gaan naar een **halve-resolutie** ping-pong blur; daarna compositen we scene + glow terug naar het scherm. De emissive lichtbollen vallen mee in die pass.

**Koppeling:** #13 (FBO + screen quad), #15 (bloom).

### 5b — Convolution (blur & edges)

**Zet B uit.** **Druk P** één of twee keer.

**Tekst:**

> Met **P** cyclen we een aparte post-processing pipeline: verborgen → **Gaussian blur** (`bloom_blur.frag`, meerdere passes) → **Laplacian edge detection** (`laplacian_edge.frag`) → uit.  
> Ook hier eerst scene naar FBO, filter naar texture, fullscreen quad naar het scherm.

**Koppeling:** #14 (convolution).

### 5c — Chroma-key (YCbCr)

**Bloom/convolutie uit.** **Druk L**: verborgen → **raw overlay** → **chroma-keyed** → verborgen.

**Tekst:**

> **L** legt een fullscreen quad over de scene (`chroma_key.jpg`).  
> **Raw:** volledige texture, groen nog zichtbaar.  
> **Keyed:** RGB → **YCbCr**, groen dicht bij de key in het Cb/Cr-vlak wordt `discard`, zachte rand met `smoothstep` op alpha — de scene schijnt door het frame.

**Druk L** tot de overlay weer verborgen is.

**Koppeling:** #16 (chroma YCbCr).

---

## Act 6 — Alles samen (± 1 min)

**Beeld:** Korte “highlight reel” — geen voice-over nodig, of korte samenvatting.

**Tekst:**

> Even alles in één flow: vrij rond het eiland, ride-cam met auto-ride, bloom aan (P uit), lichtbol recoloreren, chroma-keyed frame, close-up water/glijbaan.

**Suggestie montage:**

1. Wide shot eiland  
2. Ride-cam POV (auto-ride **G**)  
3. Bloom + lichtbol klikken  
4. Chroma-keyed overlay  
5. Onder water / pier close-up  

---

## Act 7 — Afsluiting (± 30 sec)

**Beeld:** Terug naar titelslide of README.

**Tekst:**

> Samengevat: een modulaire OpenGL-engine (Assimp + stb), Bézier-glijbaan met booglengte-parameterisatie, point-light shading met ray-picking, FBO-bloom en convolutie, en YCbCr chroma-key.  
> De broncode en session logs staan op GitHub. Bedankt voor het kijken — namens mij en Sillard.

**Op scherm:** repo-URL, Mathijs & Sillard, eventueel CC-bronvermelding Sea Keep-model (zie README).

---

## Checklist vóór opname

- [ ] Opname-setup: één persoon (Mathijs) — schermopname + mic; beide namen op titel/afsluit-slide.
- [ ] Release/Debug build werkt; `assets/` naast executable (CMake kopieert bij build).
- [ ] Schermresolutie vast (1080p aanbevolen); VSync aan als je tearing wilt vermijden.
- [ ] Mic-test; game-audio meestal niet nodig.
- [ ] Console zichtbaar voor log-bevestigingen (optioneel).
- [ ] Geen persoonlijke paden in terminal zichtbaar.
- [ ] Chroma-texture (`assets/textures/chroma_key.jpg`) aanwezig.
- [ ] Voor act 5: bloom (**B**) en convolutie (**P**) niet tegelijk aan tijdens opname.

---

## Mapping: milestones → demo-act

| Milestone | Wat tonen in video |
|-----------|-------------------|
| 1 — Engine & assets | Act 2 (laden, scene-opbouw) |
| 2 — Bézier & track | Act 4 (curve, rit, debug **V**) |
| 3 — Shaders & FBOs | Act 5 (bloom **B**, convolutie **P**) |
| 4 — Chroma & interactie | Act 3 (lichten klikken), Act 5c (**L**) |
| 5 — Ace it / polish | Act 2 (water, zee), Act 6 (montage) |

---

## Optionele B-roll (geen voice-over)

- README-screenshots: frog shot, dino perspective, chroma still.
- Korte clip: frustum / wireframe als je debug-visualisatie toevoegt later.
- Split-screen: raw chroma vs keyed (**L** cyclen).

---

## Notities voor de editor

- Ondertiteling: gebruik de **Snelle referentie**-tabel als lower-thirds bij eerste gebruik van een toets.
- Muziek: rustig, geen vocals (makkelijker voice-over).
- Export: 1080p30 of 60; H.264 voor inlevering.
