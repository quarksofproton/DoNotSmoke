---
title: "Do Not Smoke — Worklog & Architectural Decisions"
domain: 01-Projects
category: Do-Not-Smoke
status: canonical
---

# Worklog — Do Not Smoke

### 2026-11-01 — Start Screen Art, Fade Into the Level, Wing Hitboxes

* **Milestone:**
  * The title screen now shows Berf's start art (`T_UI_StartValley`) instead of the frozen level. The icon, title, tagline and buttons sit in a left column over a soft dark band. The pause menu stays centred over the frozen frame.
  * After Play the title screen fades to black (0.4 s). The level then fades in from black under the "3" of the countdown (0.6 s HUD overlay).
  * Controls panel scaled back by 10%. The language square is wider to the right, so "EN" sits centred.
  * The glider has `LeftWingCollision` / `RightWingCollision` spheres (radius 45u at ±130u), so the wings collide up to about three quarters of the span. They apply the fuselage's crash rules on overlap: chimney body → Smokestack, Type C plume → ToxicPlume, other pawn-blocking geometry → Ground.
* **Decisions & Rationale:**
  * *Fade-In on the HUD, Not the Camera:* `StartCameraFade` did not visibly advance while the level stays paused for the countdown. A UMG overlay ticks on real time.
  * *Overlap Spheres for the Wings:* The kinematic `SetActorLocation` sweep only moves the root, so child shapes never get blocking hits. Overlap events give the same coverage without changing the root collision.

### 2026-11-01 — Berf's Button Art, One Solar Seal for All Mouths

* **Milestone:**
  * Every menu button now draws Berf's `T_UI_ButtonPanel` 9-sliced: title screen, pause menu, quit dialog, language square and the newspaper buttons.
  * Every sealed mouth (Normal, Clustered, Heavy) now uses the solar-panel `SM_Seal_Single`, scaled by `ShaftRadius / SealMeshRadius` (290u): Clustered 0.91, Heavy 1.09. The code no longer loads `SM_Seal_Cluster` / `SM_Seal_Heavy`.
  * The battery sun icon moved further right and slightly down (U 0.67, +6 px).
  * The Pull Up warning at 1200u was checked in play and is fine (Quark).
* **Decisions & Rationale:**
  * *Button Art Imported at 530x100:* Slate Box slices are measured in texture pixels, so the 2120x400 original made the corners fill the whole button. The source stays in `SourceArt/UI/`, with the 1/4 copy beside it.
  * *One Cap, Scaled (Quark):* No separate panel caps are needed for Clustered and Heavy.

### 2026-11-01 — Solar Panel Seal Mesh, Quit Confirmation, HUD Nudges

* **Milestone:**
  * Barlas's new `SM_Seal_Single` (seal plate with a solar panel on top) replaced the Rev3 asset at the same path, so code and level references are unchanged. It has 7 material slots, including new `solar_*` materials. The GLB source is in `SourceArt/Meshes/`.
  * QUIT (title screen) and EXIT (pause menu) now open a "Quit the game?" YES / NO dialog. NO, or ESC, closes it.
  * The battery sun icon moved to the end of the arc (U 0.57 → 0.62). The altitude readout moved 4 px down.
* **Decisions & Rationale:**
  * *Merged GLB Before Import:* Interchange split the 22 glTF nodes into 22 meshes in a nested folder. Merging every primitive into one glTF mesh before a headless import kept a single `SM_Seal_Single` with per-part material slots.

### 2026-11-01 — Pause Menu, Game Icon in Menu, Timed Controls Panel

* **Milestone:**
  * ESC during flight opens `UDoNotSmokePauseMenuWidget`: the title-screen layout with "PAUSED", RESUME, EXIT and the EN/TR/NL square. ESC again, Enter or Space resumes. The level freezes with motion blur off.
  * The title screen logo is now the game icon (`T_UI_GameIcon`, imported headlessly from `SourceArt/icon_transparent.png`).
  * Controls panel is 20% larger and stays 10 s in total. A ring in its bottom-right corner (seal cooldown ring art, `M_UI_ProgressMask`) drains over that time, then the panel fades out in 0.3 s. The timer stops while paused.
* **Decisions & Rationale:**
  * *Pause Menu as a Subclass:* `UDoNotSmokePauseMenuWidget` only overrides `IsPauseMenu()`, so both menus share one layout and one place to swap in Berf's button art later.
  * *ESC in PIE:* The editor uses ESC to stop Play-In-Editor, so the pause menu is only reachable in a packaged build (or after rebinding the editor's stop key).
  * *EXIT Quits the Game:* Same as the title screen's QUIT. Going back to the title screen is not wired yet.

### 2026-11-01 — Battery Sun Icon, Altitude Readout, Game Icon

* **Milestone:**
  * HUD: battery arc enlarged 280 → 340 px with Berf's sun-bolt icon (`T_UI_SunBolt`) over its top cap. A live altitude number now sits in the small window at the top of the altimeter ruler.
  * Game icon from `SourceArt/icon_transparent.png`:
    * `Build/Windows/Application.ico` (16–256 px; UBT picks it up by default; un-ignored in `.gitignore`)
    * `Content/Splash/Icon.bmp` / `EdIcon.bmp` (Linux window icon, raw files that must not be imported)
    * `DoNotSmoke.png` (project thumbnail)
* **Decisions & Rationale:**
  * *Icon Files over Project Settings:* UE reads these fixed paths with no config change, so the Windows build picks the icon up straight after a pull.

### 2026-11-01 — VIP Jet Rework (Mid-Band Pass, Repel Field, Sun-Blocking Smoke), COP Cheat, Real SUN %, Type C Cone

* **Milestone:**
  * The VIP jet now flies at 4500u, through the middle of the flight band:
    * A soft repel field (6500u → 3500u keep-out) pushes the glider away.
    * The downwash is gone. Instead the jet drops `NS_SkySmog` puffs over the play area, and each puff shades the sun below it for 5–10 s.
    * The banner is a two-sided 5.5 km streamer with large yellow text. Checked in Simulate: the jet and the slogan read clearly from the side.
  * HUD SUN % is now the open share of the play area's sun cells (canopies and jet smoke), not `1 - carbon / C_min`. That formula hit 0% once carbon passed C_min while the glider could still charge.
  * Type C plume widened:
    * `NS_HeavyToxicSmog`: velocity cone 9° → 18°, puffs 500–600 → 750–900.
    * Hazard: four stacked capsules widening 260u → 1100u (`LethalPlumeTopRadius`).
  * Sparkle base sprite size reduced in the asset (1200–2000 → 100–200; ×0.2 in code = 20–40u); spawn rate raised 8 → 30.
  * Typing C, O, P in flight ends the level (`CheatFinishLevel`). The newspaper got Restart and Next Level buttons; Next Level is a placeholder.
  * The Turkish tagline "Dumanı kes, güneşi geri getir." is kept (Quark).
* **Decisions & Rationale:**
  * *Smoke Instead of Downwash (Quark):* Shading the sun makes the jet's damage to the environment felt in the core loop (charge), instead of a random altitude penalty.
  * *Shading by Query, Not Colliders:* The puffs are a list checked by `IsShadedBySmoke`. Visibility-blocking spheres at 4500u would also have intercepted the aim trace and the crosshair.
  * *Cheat on the GameMode:* The GameMode takes input itself (`EnableInput`), so the glider stays free of debug bindings.

### 2026-11-01 — L1 Play Area from Border Cubes, 18 Stacks, Outer City Ring, Soft Wall, UI Polish

* **Milestone:**
  * Quark's Border1-4 cubes (±25000, tag `LevelBorder`) now define the play area. The GameMode hides them at runtime and gives the glider an XY clamp 2500u outside them (`FlightWallMargin`): the glider slides along the soft wall instead of crashing.
  * Layout (Haiku subagents via the editor MCP):
    * L1 now has 18 stacks / 23 mouths inside ±22000: 13 Normal, Clustered 2/2/2/3 and the one Type C test stack (renamed `Smokestack_Heavy`). Minimum spacing is ~9000u, each stack has its pedestal.
    * 309 `City_Outer_*` buildings fill the ring out to ±52000 under the smog, and the asphalt floor is scaled to ±56000.
  * UI and tuning:
    * `NS_SealSparkle` `SizeScale` dropped 0.6 → 0.08 and now uses absolute scale.
    * Crosshair and cooldown ring are 50% smaller.
    * `AimAssistMaxRange` 12000 → 6000.
    * The VIP jet warning blinks 3 times.
    * Smog bed `OpacityMin/Max` 0.45/0.80 → 0.40/0.72.
    * Language moved to an EN/TR/NL square in the menu's bottom-right corner.
    * Turkish tagline is now "Dumanı kes, güneşi geri getir." (placeholder until Quark picks one).
  * Motion blur is suppressed while the level is frozen before takeoff.
* **Decisions & Rationale:**
  * *Clamp Instead of Collision Walls:* Any blocking hit on non-hazard geometry crashes the glider (`EGliderCrashCause::Ground`), so a physical wall would kill. Clamping the kinematic position needs no new collision rules.
  * *Borders Found by Tag:* Actor labels are editor-only, so the runtime looks the cubes up by tag; designers can still move them in the editor.
  * *Frozen-Frame Blur:* A paused frame keeps its last motion vectors, so the glider smeared behind the menu; turning motion blur off until "RENEW IT!" removes it.

### 2026-11-01 — Title Screen, Countdown, Controls Panel, EN/TR/NL Text, Rim Sparkle, Audio Pack

* **Milestone:**
  * Added `UDoNotSmokeMainMenuWidget`: logo, title, tagline and flat PLAY / LANGUAGE / QUIT buttons over the frozen L1. A restart (`?SkipMenu`) skips it.
  * Added the "3, 2, 1, RENEW IT!" countdown; the level unpauses on "RENEW IT!". Added a top-left controls panel that fades out 20 s after takeoff.
  * Added `DoNotSmokeLocale`: every on-screen string in English / Turkish / Dutch, cycled from the menu and saved to `GameUserSettings.ini`. English is the default; the old Turkish newspaper headlines are now English by default.
  * Raised `PullUpWarningAltitude` 600 → 1200u.
  * Replaced the white `NS_CleanSteam` on sealed rims with `NS_SealSparkle`, authored via MCP as a copy of `NS_SunMotes`: short-lived, no gravity, slight upward drift, default additive sprite.
  * Photon rain: `SpawnRate` 2.0 → 1.6, `SizeScale` 0.3 → 0.15.
  * Imported Barlas's SFX pack (headless `UnrealEditor-Cmd -run=pythonscript`; mp3 converted with ffmpeg) and wired wind, alarm, fire, boost, latch, jet roar, heavy rumble and L1 music.
  * PIE-checked menu, language cycling, countdown, HUD, controls panel, crash screen and restart.
* **Decisions & Rationale:**
  * *Menu Over the Level, Not a Separate Map (Quark):* No extra map to load or keep in sync; the frozen valley is the background. The countdown gives the player a beat to read the scene before the glider starts sinking.
  * *In-Code String Table over UE Localization:* Three languages and ~40 strings do not justify the gather/compile localization pipeline; one C++ table keeps them reviewable in a diff.
  * *Deferred Pre-Flight Pause:* Pausing in `BeginPlay` left the spring arm at its pivot (paused ticks have zero delta, so lag never moved it), which put the camera inside the glider. The level now freezes 0.1 s later and camera lag starts off until the first live tick.
  * *Sparkle as a Solar Panel Stand-In (Quark):* No falling light onto the rim, just a glint on it, so it reads as the future panel catching the sun.
  * *Music Trimmed to 3 min:* The full 8 min wave would be an ~50 MB asset in a non-LFS repo; 3 min loops well enough for a ~2 min level.
  * *Niagara via MCP Again:* The earlier crash was a one-off (Quark). Duplicating an asset and editing plain stack values went through cleanly; no new user parameters were added.

### 2026-11-01 — Windows Build Pipeline Working; Code Cleanup Playtest Passed

* **Milestone:**
  * Windows exe packaged on the Windows side of the dual-boot (`c77e14e`). Workflow: push from Linux, pull on Windows, package there.
  * MSVC fix: local `Slot` variables in `DoNotSmokeGameMode.cpp` renamed to `CanvasSlot` (they shadowed `UWidget::Slot`).
  * Windows editor re-saved `DoNotSmoke.uproject` (2-space indent, dropped `AdditionalDependencies` and the `ModelingToolsEditorMode` Editor-only allow list); harmless so far.
  * 2026-11-01 Code Cleanup playtest passed (Quark).
* **Decisions & Rationale:**
  * *L1 Before L2/L3 (Quark):* Polish L1 to a good state first; the later sectors reuse its systems, so they become cheap once L1 is solid.
  * *Star Bands Deferred:* Optimal runs end at ~5500 kg, so 4000 kg is too low. Proposed bands 5000–6000 / 6000–7000 / 7000+ kg, but not locked because L1 will grow (more stacks, wider area) first.

### 2026-11-01 — Sky Smoke Raised to ~9700u (Committed with the Sky Choke Pass, `5b6de74`)

* **Milestone:**
  * Raised `CanopySmokeOffsetZ` −500 → +1200 (sky smoke ~9700u).
  * Reduced `CanopySmokeSizeScaleRange` max 4 → 2.
  * Increased `CanopySmokeSpawnRateRange` max 10 → 16.
  * Committed the whole sky pass as `5b6de74`: faster choke, clear play space, canopy disc fix, photon rain, raised sky smoke.
* **Decisions & Rationale:**
  * *Clouds Above the Flight Band (Quark):* Growing ×4 sprites hung several km below the layer into the flight band. The gameplay blocker stays at 8500u, so only the look moves.

### 2026-11-01 — Sun Beams Replaced by Level-Wide "Photon Rain"; Canopy Blocker Fixed

* **Milestone:**
  * Removed the per-stack sun beam (Quark disliked it).
  * Added a level-wide grid of `NS_SunMotes` cells (sparse warm light motes falling from the ceiling), each switched off while a smoke canopy covers it.
  * Turned the canopy blocker into a lifted-sphere disc.
  * Sunlight traces now end at Z 8500.
  * PIE-checked.
  * Follow-up (Quark: "looks like snow"): the motes became thin bright-yellow vertical streaks via the new additive `M_LightStreak`, with sprite rotation set to 0 and `SizeScale` 0.3. Only material and plain values changed; no risky Niagara enum or parameter edits.
* **Decisions & Rationale:**
  * *Grid of Cells, Not Per-Particle Collision:* One trace per 5 km cell every 0.5 s replaces CPU collision on thousands of motes. The rain covers the whole level, so the player can spot a distant opening and fly for it.
  * *Canopy Bug Found:* With the 9 km radius, the old sphere sagged into the flight band and upward traces starting inside it missed. The glider near open stacks wrongly got sunlight. The lifted dome plus traces that stop exactly at the ceiling fix both the glider charge and the rain.
  * *Safe Niagara Authoring:* Adding a user parameter through MCP crashed the editor (`NiagaraVariant` assert). `NS_SunMotes` is therefore a copy of `NS_SkySmog` that reuses its existing user parameters and changes only plain values.

### 2026-11-01 — Clear Play Space, Hard-Edged Smoke Ceiling, Sun Beams over Real Sun Pockets

* **Milestone:**
  * Removed the high-altitude haze (the height fog second layer is no longer driven).
  * Sun dimming is softened from 30% to 60%.
  * `NS_SkySmog` is darker and more opaque (colour 0.07, alpha 0.88).
  * Added `SunBeamMesh` + `M_SunBeam` to `ASmokestack`: a warm light shaft over a sealed stack that is shown only while an upward trace confirms the pocket is really open.
  * Added the debug exec `DebugSealNearestStack`.
  * Built, PIE-checked (beam over the sealed stack, clear air, faint city).
* **Decisions & Rationale:**
  * *Play Space Must Stay Readable (Quark):* After a while the beige haze hid the surroundings and killed the fun. The choke now lives only in the ceiling, as solid closed areas versus clear open areas, never as fog in the flight band. The smog bed fog stays as it was (Z 0, density 0.35), because lowering it to −1000 made the city fully visible.
  * *Beam Visibility Uses the Gameplay Trace:* The beam appears exactly where the glider's sunlight trace would succeed, so it is a reliable "charge here" marker. A sealed stack whose sky is smothered by a neighbour's 9 km canopy shows no beam.

### 2026-11-01 — Sky Choke Made Visible: Full Choke at 1.0 × C_min, Canopy 1.5 → 9 km, Growing Sky Smoke

* **Milestone:**
  * `SkyChokeCarbonMultiplier` 2.5 → 1.0.
  * `CanopyMinRadius` / `CanopyMaxRadius` 800 / 3200 → 1500 / 9000.
  * Sky smoke spawn rate 3 → 10/s.
  * New `User.SizeScale` on `NS_SkySmog` (added by a Sonnet agent, multiplies the initial sprite size), driven 1 → 4 with the canopy growth.
  * PIE: the blue sky is mostly gone by about 11 s with nothing sealed.
* **Decisions & Rationale:**
  * *Why It Never Closed (Quark):* Full choke needed 10,000 kg (2.5 × C_min), about 77 s at 13 open mouths and never reached in a real run. The 3.2 km clouds also covered almost none of the new 65 km layout, and fixed-size particles thinned out as the cloud widened.
  * *Gameplay Follows the Visual:* The sunlight-trace blocker uses the same radius. Overlapping 9 km canopies therefore really shrink the sun pockets to the sealed stacks, as the GDD intends. This makes L1 harder, so it goes into the C_min / balance playtest.

### 2026-11-01 — Quieter Jet Warning, Faster Cruise with Same Sink, Type C as Tall as Type A, Inverted W/S

* **Milestone:**
  * The jet warning appears once for 3 s, steady, when a jet is dispatched. It sits right of the top bar at the same height (448×112).
  * `BaseVelocity` 1800 → 2400, `MaxDiveVelocity` 3800 → 4600, `GlideRatio` 15 → 20.
  * `HeavyHeightScale` 1.0 → 1.714, so the Type C mouth is at ~2400u.
  * S now pitches up and W pitches down.

  Built and PIE-checked.
* **Decisions & Rationale:**
  * *Warning Fatigue (Quark):* A blinking panel shown for the whole approach every ~10 s was on screen almost constantly. Showing it once per dispatch keeps the information without the noise.
  * *Speed Up, Same Sink:* Quark found unpowered cruising too slow. The sink is `speed / GlideRatio`, so raising the speed and the ratio together (2400 / 20 = 1800 / 15 = 120 u/s) keeps the descent identical. This has the same effect as Quark's "more gravity" idea, without changing the dive feel. The dive cap was raised so boost (2400 + 2200) is no longer clipped.
  * *Inverted Pitch (Quark):* Flight-stick convention.

### 2026-11-01 — Level-Complete Pause & Confetti, One Random Jet Route Every 10 s, Slogan Off the HUD

* **Milestone:**
  * Level completion pauses the world and bursts UI confetti with the newspaper.
  * Jets are dispatched by the GameMode, one random idle route every 10 s, the first at 6 s.
  * The HUD jet warning shows only "! VIP JET INCOMING !".

  PIE-tested: the frozen world behind the summary, confetti, R restart unpausing, and the slogan-free warning.
* **Decisions & Rationale:**
  * *Pause, Not Per-Actor Freeze:* `SetGamePaused` stops the glider, jets, Niagara and timers in one call. UMG keeps ticking on real time, so the spin and confetti still animate, and `TriggerInstantRestart` unpauses before reloading.
  * *Confetti as Plain UMG Rectangles:* ~140 tinted `UImage`s simulated in `NativeTick` (gravity, drag, flutter, spin, fade). This needs no art from Berf, no Niagara-in-UI plugin, and runs while paused.
  * *Central Jet Dispatcher (Quark):* With several routes in the level, each `APrivateJet`'s own 18 s timer launched them all together. The GameMode now owns timing, which also lets Quark add routes freely. Jets keep `bAutoSchedulePasses` for maps without a dispatcher.
  * *Slogan Only on the Banner (Quark):* The satire reads on the towed banner in the world. The HUD stays a pure warning.

### 2026-11-01 — L1 Layout Widened (11 Stacks / 13 Mouths) & Type A Raised to ~2400u

* **Milestone:**
  * Added per-archetype height scales (`NormalHeightScale` 2.0) and a per-placement `HeightVariation` to `ASmokestack`. The body mesh stretches in Z and the rims, collision and VFX follow.
  * Rebuilt the `L_Level01_Valley` layout: the 8 existing stacks were moved and 3 new Type A stacks added, each on a matching pedestal, spread from the spawn out to ~65 km:
    * a 4-stack S-curve valley line in front of the spawn;
    * a 5 km industrial ring of 3 singles around a twin;
    * a southeast twin plus a single;
    * a lone far Heavy.
  * Type A heights vary ×0.85–1.2.
  * `TargetMinCarbonKg` 500 → 4000.
  * Map saved and PIE-checked: STACKS 13/13.
* **Decisions & Rationale:**
  * *Stretch Instead of New Art:* Type A had to clearly tower over Type B (2400u vs 600u, previously 1200u vs 600u). Scaling the Rev3 assembly in Z avoids waiting on Barlas, and the type scales stay tunable.
  * *C_min Re-estimated:* With 13 mouths and a 5 s seal cooldown, a perfect run needs ≥ 65 s with ~6.5 mouths open on average (10 kg/s each), about 4200 kg. 500 kg was unreachable, and the sky (choke at 2.5 × C_min) would have closed in ~10 s. 4000 kg is an estimate to be confirmed in play.

### 2026-11-01 — Flight Ceiling Doubled (3900u → 7800u) with the Smoke Sky Raised Alongside

* **Milestone:** Doubled `GliderMaxCeilingAltitude` from 3900u to 7800u (Quark), moving these with it:
  * `JetAltitude` 4200 → 8400u; the wake box still reaches down to 1400u (offset/extent Z 3500).
  * `CanopyWorldAltitude` 4250 → 8500u, so `NS_SkySmog` sits at ~8000u.
  * `SunlightTraceLength` is now a 10000u UPROPERTY (was a hardcoded 6000).
  * `LethalPlumeHalfHeight` 1200 → 3400 (the Type C wall spans the flight band).
  * High haze offset 3500 → 7000u.
  * The HUD ruler top is 3900 → 7800u.

  A Sonnet agent lengthened `NS_ToxicSmog` / `NS_HeavyToxicSmog` to reach ~7000u above the mouth (speed 700–880, life 10–12 s, spawn 18 and 30/s, peak ~200 and ~330 particles). Built and PIE-checked.
* **Decisions & Rationale:**
  * *Smoke Sky Rises with the Ceiling (Quark's choice):* Leaving the canopy at 4250u would have let the glider climb above the smoke into permanent sunlight and remove the choke pressure. Raising it keeps the sun-pocket mechanic intact at every height. The cost is longer plumes, accepted as the price of keeping the mechanic.

### 2026-11-01 — Smoke Feedback Pass: Plumes Visible from Above, `NS_SkySmog` Canopy, Faster Sink

* **Milestone:**
  * The lower plumes no longer vanish from high angles.
  * The flat canopy discs are replaced by a volumetric `NS_SkySmog` cloud that spreads from each plume top. It was authored by a Sonnet agent, uses the user parameters `SpreadRadius` / `SpawnRate`, and is driven by `ASmokestack::UpdateCanopyCloud`.
  * `GlideRatio` 20 → 15, about 120 u/s of sink.
  * Built, editor restarted, PIE-checked.
* **Decisions & Rationale:**
  * *Translucency Sort Priority:* The 200 km translucent `SmogBedSurface` sorted after the smoke particles and painted over the lower plumes from above. Setting its `TranslucencySortPriority` to −100 makes it always draw first.
  * *Particles over a Mesh for the Sky Smoke:* Quark found the growing flattened sphere read as a thin ring. A Niagara cylinder slab with the same radius as the trace blocker matches the plumes' look, and it dissipates naturally when a stack is sealed (deactivate instead of hide).
  * *Editor Restarts and Port 8000:* Restarting the editor right after quitting can fail to bind the MCP port. Waiting about 20 s before relaunching fixes it.

### 2026-11-01 — Constant Glide Sink, Real Smoke Plumes (Type C Pitch Black) & Visible Sky Choke

* **Milestone:**
  * The glider now sinks continuously.
  * Every unsealed chimney emits a real smoke plume; Type C emits a separate pitch-black dense plume.
  * The sky visibly chokes as carbon rises: canopy smoke clouds over each open stack grow, the sun dims and a high haze thickens.

  Built, editor restarted, and PIE-tested (plumes, canopy clouds, sky darkening, ground crash after ~18 s with no input).
* **Decisions & Rationale:**
  * *Sink at Every Attitude (Quark):* The old sink only applied near level pitch and was scaled by 0.25 (~18 u/s), so the glider could cruise almost indefinitely. Now the glider sinks at `speed / GlideRatio` whenever it is not boosting, with `GlideRatio` 25 → 20 (~90 u/s), and the zoom-climb ceiling decays at the same rate. Sunlight and Solar Boost therefore become the core survival loop.
  * *Niagara Rebuilt from Fountain Templates (Sonnet agent + Opus):*
    * **Old state:** `NS_ToxicSmog` / `NS_CleanSteam` were untouched Fountain templates (white sparks falling under gravity), so there was no visible smoke.
    * **Plumes:** they are now looping, rising and growing plumes (`ScaleSpriteSize` added).
    * **Type C:** `NS_HeavyToxicSmog` is new and used by Type C through `ASmokestack::GetActiveSmogSystem`.
  * *Custom `M_SmokeSprite`:* The default Niagara sprite material is additive, so black smoke was invisible. The new translucent unlit sprite material uses particle colour and a soft radial noise mask.
  * *Canopy Made Visible Instead of a New Sky System:* The existing per-stack canopy sphere already drives the sun-pocket gameplay. A child `CanopyCloudMesh` with the same radius makes the mechanic readable 1:1, and sealing removes the cloud above that stack. The first try had hard-edged "saucers"; a fresnel edge fade (`M_SmokeCanopy`) softened them.
  * *Global Sky Darkening in the GameMode:* `ApplySkyChoke` dims the sun to 30% and raises a high-altitude haze (height fog second layer) with the existing choke alpha, then clears the sky when the level completes.

### 2026-11-01 — Chimney Sealing Becomes Solar Panel Installation (Design Pivot)

* **Milestone:** Core interaction changed from capping chimneys to installing solar panels. Implementation deferred; docs and code not yet updated.
* **Decisions & Rationale:**
  * *Seal Caps → Solar Panels:* A hit on a rim now installs a solar panel on the chimney instead of sealing it shut. Rationale: Closing a chimney sent the wrong message. The intended message is the transition from fossil fuels to renewable energy, and panels make that visible.
  * *Panel Mechanic Ties to Existing Systems:* Installed panels fit the Solar Boost and sunlight loop already in the design, so the reward is thematically consistent with the sky clearing up.
  * *Pending Work:* Rename or rework the seal-related code (`IBPI_SealableTarget`, `CarbonSealProjectile`, `SM_Seal_*`, `CleanSteamVFX`), update the SFX list (`SFX_Seal_Latch`), and rewrite the seal wording across the GDD, architecture, one-pager and TODO.

### 2026-11-01 — COP31 Newspaper Summary & Two-Slot HUD Top Bar (Berf's Second UI Batch)

* **Milestone:**
  * Imported Berf's 8 new images as `T_UI_*` (source PNGs in `SourceArt/UI/`).
  * Rebuilt `UDoNotSmokeSummaryWidget` as a spinning *The Daily Smog* front page, with the Do Not Smoke logo, the headline, a star-dependent valley photo, an AUDIT stats box and a `[R]` restart footer.
  * Replaced the HUD top bar with the two-slot `T_UI_HudBar` and a smoke cloud CO2 icon.
  * Added the non-shipping console command `DebugForceLevelComplete <1-3>`.
  * PIE-tested 1, 2 and 3 star pages and the `R` restart.
* **Decisions & Rationale:**
  * *Placement (Quark):*
    * `hud_bar` → flight HUD top bar: stacks / sun / stars on the left, CO2 on the right.
    * `smoke_cloud` → CO2 icon on the HUD and in the audit box.
    * `prohibition` + `stack_puff` → masthead logo, matching the GDD logo concept.
    * `valley_1star` / `valley_2star` / `valley_scene` → the 1 / 2 / 3 star photo.
  * *Pixel-Space Layout on the Art:* Every newspaper element is positioned in the 800x1000 source pixel coordinates times one page scale. The text boxes therefore line up with the drawn frames, and the whole page can be rescaled or spun as one panel.
  * *Debug Exec over Manual Playthroughs:* Reaching the summary needs sealing every stack. A console command makes the summary screen testable in seconds.

### 2026-11-01 — Smog Bed Rebuilt (Height Fog + `M_SmogBed` Surface) & Aim Lock Range 12000u

* **Milestone:** Rebuilt the `L_Level01_Valley` smog bed:
  * Removed the 16 `City_NiagaraSmog_*` actors.
  * Retuned `SmogBedHeightFog`: moved to Z 0, density 0.35, falloff 2.0, max opacity 1.0, start distance 0, toxic brown inscattering, sky contribution 0.35.
  * Added a non-colliding `SmogBedSurface` (`SM_SmogLayer` ×4 at Z 380) with a new unlit material `Content/Game/VFX/Materials/M_SmogBed`.
  * Lowered `AimAssistMaxRange` from 20000u to 12000u (Quark).

  Quark saved the map and playtested the look. The module builds.
* **Decisions & Rationale:**
  * *Niagara "Fountain" Smog Removed:* `NS_ValleySmog` was built from the Fountain template. Its particles shot up and fell under −980 gravity, so the smog was patchy and flickering, and its large translucent sprites cost frame time.
  * *Height Fog Was Capped at 25%:* With `FogMaxOpacity = 0.25` the city could never be hidden. The city sits at −1800u to −1050u, far below the 200u kill floor, so the fog has to cover a deep volume. Density 0.35 keeps the buildings as faint silhouettes, as Quark asked.
  * *Custom Unlit `M_SmogBed` over the glTF Material:* The imported `M_SmogCeiling` (a glTF translucent instance) tiled visibly from altitude, looking like a plaid shirt. Being lit and reflective, it also mirrored the glider and chimneys, which looked like double images. `M_SmogBed` samples `LowResBlurredNoise` in world space at 22000u and 7000u, panning in opposite directions. Opacity varies from 0.45 to 0.8 with a 700u depth fade where chimneys cross it, and there is no lighting or reflection.
  * *VRAM Accepted for Now:* The Editor still uses ~4 GB on this level. Niagara was a frame-time cost, not the main VRAM user. The earlier out-of-memory crash has not recurred, so it is parked as a Low task.

### 2026-11-01 — C++ Codebase Review & Cleanup (Phase 1 Bug Fixes + Phase 2 Tidy)

* **Milestone:** Three Sonnet agents reviewed all gameplay C++ (read-only), then fixed the agreed Phase 1 bugs and Phase 2 cleanup in parallel per file group. Opus reviewed the diffs and the module builds with zero errors. Not yet playtested in PIE (the Editor crashed with a Vulkan VRAM out-of-memory on `L_Level01_Valley`; see the fog task in `TODO.md`).
* **Decisions & Rationale:**
  * *Bug Fixes:*
    * **Win/crash ordering:** a projectile landing after a crash no longer completes the level.
    * **Win screen:** the summary now hides the HUD, takes UI input and restarts on `R` / `Enter` / `Space` (previously a dead end).
    * **Crash cleanup:** a crash now stops the sunlight timer, the PULL UP state, the boost VFX and the held inputs.
    * **Magnetic lock flag:** derived from the same condition that uses the rim.
    * **Projectile Blueprint overrides:** speed and gravity overrides now reach the movement component.
    * **Jet:** snaps to the spline start before its wake box turns on, uses an altitude guard with no gap (`JetAltitude`), and plays slogan 0 first.
    * **Canopy:** placed at world Z `CanopyWorldAltitude` instead of a clamped relative offset.
  * *Performance:* The glider caches the sealable-actor list in `BeginPlay` instead of scanning the world every tick, and the HUD only updates the top-bar text when it changes.
  * *Cleanup:*
    * **Debug HUD:** removed the unreachable on-screen telemetry and the debug-draw reticle.
    * **Hidden constants:** the hidden `×15` steer multiplier is now an honest `MagneticSteerInterpSpeed = 90`, with identical behaviour.
    * **Duplicated literals:** star thresholds, jet wake box and aim range are now UPROPERTYs or reuse existing ones.
    * **Stale comments:** fixed in all files.
    * **WBP class lookups:** the failing `FClassFinder`s for the non-reparented WBPs were dropped, which removes the startup error log.
  * *Kept on Purpose:*
    * **Mesh migration guards:** the `BeginPlay`/`OnConstruction` guards that swap old non-Rev3/V2 meshes stay, so level instances saved before the art revision still upgrade. Only their path strings were deduplicated.
    * **Concrete-type couplings:** GameMode/HUD → glider/stack/jet and glider → projectile are recorded as accepted jam exceptions in `technical-architecture.md` instead of being refactored.
    * **Rim and plume shapes:** collision shapes are now hidden in game (they were debug-visible).

### 2026-10-31 — Berf's UI Art Integrated: Textured Flight HUD, `M_UI_ProgressMask` & Crash Screen with Cause Icons

* **Milestone:** Moved Berf's 19 PNGs to `SourceArt/UI/` and imported them as `Content/Game/UI/Textures/T_UI_*` (UserInterface2D, no mips, UI group) via the Unreal MCP. Built the UI material `M_UI_ProgressMask`, rebuilt `UDoNotSmokeFlightHUDWidget` on the textures, and added `UDoNotSmokeCrashWidget` plus `EGliderCrashCause`. Compiled, hot-reloaded and playtested in PIE: HUD layout, jet warning, PULL UP panel + vignette, ground-impact crash screen and `R` restart all work.
* **Decisions & Rationale:**
  * *Crash Screen Replaces Silent Instant Restart (Quark):* A crash now shows the cracked-glass vignette, a cause icon (`T_UI_GliderDive` ground, `T_UI_SmokestackBurst` chimney body, `T_UI_SmokeWarning` Type C plume) and a keycap `RESTART [R]` button. Restart stays one input away, so the arcade retry loop is kept while the player learns what killed them.
  * *Crash Cause from the Existing Collision Paths, No New Casts:* Blocking hit on a lethal `IBPI_FlightHazard` = smokestack, lethal overlap = toxic plume (the only lethal overlap volume), kill floor or other geometry = ground. Rejected adding a cause method to `IBPI_FlightHazard`, which would touch every hazard class for the same result.
  * *Altimeter Arrow Mapped Piecewise to the Ruler's Colour Bands:* The ruler art has dark / red / orange / green bands, so the arrow is mapped 0/400/600/1400/3900u onto those band edges instead of linearly. The bands now mean Smog Bed, PULL UP zone, chimney band and open sky.
  * *One Parameterised UI Material for Both Fills:* `M_UI_ProgressMask` does a radial clockwise mask (cooldown ring) or a bottom-to-top mask with a V range (battery arc). One shader serves both, which keeps compile cost and asset count down.
  * *Verification:* Quark playtested the parts the automated PIE run could not reach (battery arc and cooldown ring fills, smokestack and plume crash icons, mouse click on the restart button); all work.

### 2026-10-31 — Code Values Ratified as Final Tuning; Design Docs Aligned

* **Milestone:** Quark declared the numeric values currently in `Source/DoNotSmoke/` final. Aligned the GDD, one-pager and `technical-architecture.md` to them.
* **Decisions & Rationale:**
  * *Code Is the Tuning Source of Truth:* Values were tuned in-engine during playtesting, so the docs now follow the code rather than the other way round. Superseded doc values: Pitch Rate `45 → 105 deg/s`, Roll/Yaw Rate `60 → 125 deg/s`, camera yaw clamp `±110° → ±85°`, dive/boost FOV `110° → 96°`, aim cone `45° → 135°`, aim-assist range `8000u → 20000u`, rim sphere `220–250u → 240u`, glider ceiling `2600u → 3900u`, VIP jet altitude `3000u → 4200u`, wake band `1800–2600u → 4200u–1400u` (plunge at `2400 u/s` to `1350u`), smog canopy `2700u → 4250u`, sunlight trace `to 3500u → +6000u` upward.
  * *Newly Documented Values:* Rim heights per type (Normal `1200u`, Clustered `600u`, Heavy `1400u`), projectile gravity `0.25` with `4.0s` lifetime, jet speed `2800 u/s`, `18s` pass interval and `2.5s` warning lead, canopy radius `800u → 3200u`.

### 2026-10-31 — C++ HUD & COP31 Summary Widgets, First Niagara Smoke/Steam Wiring, Rev3 Meshes, `SM_SolarGlider_V2` & City Kit Import

* **Milestone:** Added `UDoNotSmokeFlightHUDWidget` (altitude, solar battery and seal-cooldown bars, stacks/CO₂ readout, jet `!` and `"PULL UP!"` warnings, crosshair) and `UDoNotSmokeSummaryWidget` (`SetupSummaryData`: stars, carbon kg, time, accuracy, newspaper headline from `GetNewspaperHeadlineForStars`) to `DoNotSmokeGameMode`, backed by `WBP_FlightHUD` / `WBP_COP31_Summary`. Wired `NS_ToxicSmog` / `NS_CleanSteam` into `ASmokestack` (plus `NS_ValleySmog`), moved all smokestack and seal meshes to `Meshes/Rev3/`, the glider to `SM_SolarGlider_V2`, imported a 16-mesh low-poly `Meshes/City/` kit (incl. `SM_SmogLayer`, `SM_ValleyAsphaltFloor`) and expanded `L_Level01_Valley`.
* **Decisions & Rationale:**
  * *C++ Widget Classes with `BuildFallbackWidgetTree()`:* The widgets build their own widget tree when the WBP has no bound children, so HUD and summary work before Berf's visual pass; the WBPs stay thin skins, consistent with the AI-First C++ Core decision.
  * *Projectile Spawn from Glider Nose (`GetProjectileSpawnLocation`) with World-Space Velocity:* Aim and magnetic-lock math now run from the actual launch point instead of the actor origin, removing parallax misses between reticle and capsule.
  * *Aim Cone Widened (`MaxAimConeHalfAngleDeg` 45° → 135°, `AimAssistMaxRange` 8000 → 20000) — Ratified by Quark:* Only a 45° rear blind cone remains, so side, downward and slightly backward shots while passing over a rim are allowed. Rationale: set during playtesting for more forgiving high-speed flyby shots. This supersedes the earlier 45° "shallow dive approach, no backward shots" rule. GDD, one-pager, `technical-architecture.md` and `GliderPawn` comments are aligned to 135°.

### 2026-10-31 — Design Docs Consolidated in the Vault

* **Milestone:** Removed the duplicate `GameDesign/` docs (`DO_NOT_SMOKE_GDD.md`, `DO_NOT_SMOKE_One_Pager.md`, `TECHNICAL_ARCHITECTURE.md`) from the `DoNotSmoke` code repo and moved `CREDITS.txt` to the repo root.
* **Decisions & Rationale:**
  * *Vault as Single Source (Rule 2):* The repo copies had already drifted (e.g. still described a "Blueprint-First" engine architecture); the Vault versions were a superset, so nothing was lost. Rejected keeping a repo mirror because two editable copies will diverge again.
  * *`CREDITS.txt` Stays in the Repo:* It is the CC-BY-4.0 attribution for the `UNDERPOLY` glider model and must ship with the game, so it is a build asset rather than design documentation.

### 2026-10-31 — Unreal MCP Full Toolset Activation, Level 1 (`L_Level01_Valley`) 2D Basin Layout & `+50%` Flight Ceiling / Jet Wake Plunge

* **Milestone:** Enabled `AllToolsets`, `EditorToolset`, `UMGToolSet`, `NiagaraToolsets`, and `ConfigSettingsToolset` alongside `ModelContextProtocol` in `DoNotSmoke.uproject`, configured `/Game/Game/Maps/L_Level01_Valley` as the default Editor and Game startup map in `DefaultEngine.ini`, fixed per-mouth magnetic aim-assist (`GetUnsealedRimLocations()`), constructed the 2D `X/Y` city basin layout with randomized chimney `Yaw` orientations via MCP, raised `GliderMaxCeilingAltitude` by `+50%` (`2600u -> 3900u`), extended `APrivateJet::WakeTurbulenceBox` down to chimney rim height (`1400u`), and implemented rapid `2400 u/s` wake downwash plunging the glider down to `1350u`.
* **Decisions & Rationale:**
  * *Full Engine MCP Toolsets (`AllToolsets` + `EditorToolset` + `UMGToolSet` + `NiagaraToolsets`):* Enabling `AllToolsets` in `DoNotSmoke.uproject` exposes 32 native UE 5.8 AI toolsets over `127.0.0.1:8000/mcp`, allowing direct programmatic manipulation of actors, transforms, properties, UMG widget hierarchies, and Niagara emitter stacks without manual editor clicks.
  * *Per-Mouth Magnetic Aiming (`IBPI_SealableTarget::GetUnsealedRimLocations`):* Extended `IBPI_SealableTarget` to return all active unsealed rim coordinates on an actor so `AGliderPawn::ComputeClampedAimDirection()` and `ACarbonSealProjectile::Tick()` evaluate and lock onto each individual chimney mouth of a Twin/Triple `Clustered` stack independently.
  * *`+50%` Flight Ceiling (`3900u`) & Chimney-Skimming Jet Evasion (`1400u` Floor):* Raised `GliderMaxCeilingAltitude` to `3900u` (with `APrivateJet` at `4200u` and `CanopyWorldAltitude` at `4250u`) to give the player generous vertical room for banking and diving. Extended `APrivateJet::WakeTurbulenceBox` from `4200u` down to `1400u` (chimney rim height): to evade an approaching VIP jet (`!` warning), the player must dive down to chimney level (`<1400u`); getting caught in the wake above `1400u` triggers a rapid `2400 u/s` downward plunge that forces the glider down to `1350u` (`WakeTargetFloorAltitude`) before releasing control.
  * *2D City Basin Distribution & Randomized Chimney Yaw:* Replaced linear corridor placement with a 360° `X/Y` city perimeter layout around the central basin with randomized `Yaw` rotations on every `ASmokestack`, encouraging open 3D routing and banking turns.

### 2026-10-31 — 3D Asset Pipeline Integration (`SM_SolarGlider`, 3 Smokestack Archetypes, Multi-Mouth `Cluster_2`/`Cluster_3`, Seal Caps & `SM_PrivateJet`)

* **Milestone:** Baked multi-node `.glb` models from `Do Not Smoke.rar` into single-node static meshes (with `SM_SolarGlider` evaluated at its mid-flight gliding pose, retracted landing gear, and translucent canopy glass, plus baked Twin `SM_Smokestack_Cluster_2` and Triple `SM_Smokestack_Cluster_3` assemblies on shared base pads), imported all 10 hero meshes with their materials and textures into `Content/Game/Meshes/` via `UnrealEditor-Cmd -run=ImportAssets`, aligned aircraft forward axes (`Yaw = -90°`), wired multi-mouth per-chimney sealing into `ASmokestack` and `ADoNotSmokeGameMode`, and compiled `libUnrealEditor-DoNotSmoke.so` with zero errors.
* **Decisions & Rationale:**
  * *Pre-Import Single-Mesh Node Baking (`/tmp/merge_glb.py`):* The raw `.glb` files contained 25–56 separate child mesh nodes per asset (plus a ground-to-air landing gear animation on `Glider_With_Pilot.glb`). Baking world node transforms into a single multi-primitive mesh per `.glb` prior to Interchange import guaranteed 1 clean `.uasset` per model (`SM_SolarGlider`, `SM_PrivateJet`, `SM_Smokestack_Single/Cluster/Cluster_2/Cluster_3/Heavy`, `SM_Seal_Single/Cluster/Heavy`) instead of 150+ fragmented sub-meshes.
  * *Multi-Mouth `ESmokestackType::Clustered` (`ClusterStackCount = 2` or `3`):* Equipped `ASmokestack` with 3 independent sets of shaft collision capsules, `RimTargetSphere` hitboxes, `SealCapMesh` caps, and Niagara VFX slots (`X = -340, +340, Z = 1320u` for Twin; `X = -680, 0, +680, Z = 1320u` for Triple). Passing the impacting `ACarbonSealProjectile` actor into `TrySealTarget()` allows a single `ASmokestack` actor in `Clustered` mode to seal the exact chimney mouth that was hit and decrement `ADoNotSmokeGameMode::OpenSmokestackCount` per mouth across multiple flyby passes.
  * *Hollow Rim Collision Separation (`ChimneyBodyCollision` + `SealCapMesh`):* As specified in the model package documentation (`ONCE_OKU.txt`), disabled automatic mesh collision on `ChimneyBodyMesh` (which would block the open top mouth) in favor of dedicated `ChimneyBodyCollision` capsules covering each shaft up to `MouthHeightZ - 80u`, paired with `SealCapMesh` components that become visible the instant each mouth is sealed.

### 2026-10-31 — Milestone 2 C++ Completion (`APrivateJet`), Greybox Visual Fallbacks & On-Screen Debug Telemetry

* **Milestone:** Implemented `APrivateJet` (`PrivateJet.h` / `PrivateJet.cpp`), wired approach alerts into `ADoNotSmokeGameMode`, configured `GlobalDefaultGameMode=/Script/DoNotSmoke.DoNotSmokeGameMode` in `DefaultEngine.ini`, added built-in `/Engine/BasicShapes/` greybox meshes and on-screen C++ Debug HUD telemetry across all gameplay classes, and compiled `libUnrealEditor-DoNotSmoke.so` with zero errors.
* **Decisions & Rationale:**
  * *Built-In Greybox Meshes & C++ Debug Telemetry (`bShowDebugHUD`):* Assigned engine primitive meshes (`Cone`, `Cylinder`, `Sphere`, `Plane`) and `UTextRenderComponent` banner text as zero-setup defaults, alongside live on-screen telemetry (`AddOnScreenDebugMessage` + 3D `DrawDebugSphere` aim reticle). Rationale: Enables immediate in-editor flight, aiming, and level playtesting before 3D art (`Barlas`) and UMG widgets (`Berf`) are imported, while automatically hiding the greybox glider once `SK_SolarGlider` is assigned.

### 2026-10-31 — Milestone 1 C++ Core Implementation, HFS-SOLID Technical Architecture Spec & Overhead Smog Canopy

* **Milestone:** Authored the canonical AI-First Technical Architecture & Handover Specification (`technical-architecture.md` / `GameDesign/TECHNICAL_ARCHITECTURE.md`), implemented the complete Milestone 1 C++ core (`GameplayInterfaces.h`, `ASmokestack`, `ACarbonSealProjectile`, `AGliderPawn`, `ADoNotSmokeGameMode`) plus the `0.2s` vertical sunlight `LineTrace` and altitude hazard stack, and compiled `libUnrealEditor-DoNotSmoke.so` with zero errors.
* **Decisions & Rationale:**
  * *Bounded 6-Unit HFS-SOLID Architecture (`GameplayInterfaces.h` + 5 Classes):* Restricted the entire C++ codebase to 1 shared interface header (`IBPI_SealableTarget`, `IBPI_FlightHazard`) and 5 actors/gamemode classes (`ASmokestack`, `ACarbonSealProjectile`, `AGliderPawn`, `APrivateJet`, `ADoNotSmokeGameMode`) with zero concrete peer-to-peer `Cast<T>` calls. Rationale: Prevents spaghetti coupling during the remaining 30 hours of the Game Jam and guarantees that any AI model or fresh session can immediately pick up the project via `technical-architecture.md`.
  * *Per-Stack `OverheadSmogCanopySphere` for Shrinking Sun Pockets:* Added an overhead `ECC_Visibility`-blocking sphere component (`OverheadSmogCanopySphere` at `Z = 2750u`) directly to `ASmokestack`. As cumulative level carbon rises, `ADoNotSmokeGameMode` expands the canopy radius on all unsealed stacks (`SetCanopyExpansionAlpha`), gradually choking out upward `LineTrace` sunlight corridors; the instant a smokestack is sealed, its overhead canopy disables collision, opening a natural **Sun Pocket (God-Ray)** directly above the sealed stack without needing a separate sky manager class.

### 2026-10-31 — Repository Baseline, Content/Game Hierarchy & Pivot to AI-First C++ Architecture

* **Milestone:** Initialized solo-developer Git repository in `/home/quark/Documents/DoNotSmoke`, created the modular `Content/Game/` folder hierarchy (`Core`, `Player`, `Actors`, `Maps`, `UI`, `Audio`, `VFX`, `Meshes`), and transitioned the technical stack to an AI-First C++ Core architecture.
* **Decisions & Rationale:**
  * *AI-First C++ Core + Thin Blueprint Layer over Pure Blueprint:* Superseded the earlier Blueprint-First decision in favor of writing all core gameplay systems (`AGliderPawn`, `ACarbonSealProjectile`, `ASmokestack`, `APrivateJet`, `ADoNotSmokeGameMode`) directly in UE5 C++, using Blueprints strictly as thin data/visual wrappers for assigning meshes, VFX, audio, and UI widgets. Rationale: Given the tight 48-hour Game Jam window, AI-assisted C++ generation allows rapid, deterministic implementation of flight kinematics, 45° cone math, magnetic homing, and level state machines without manual Blueprint node wiring bottlenecks.
  * *Solo-Focused Local Git Setup & `Content/Game/` Root:* Configured `.gitignore` for solo UE5 development (excluding heavy unused template `Content/Characters/` assets while avoiding unnecessary team Git LFS overhead since Quark is the sole developer in Unreal) and consolidated all project assets under `Content/Game/`.

### 2026-10-31 — GDD Consolidation: Realistic Chimney Archetypes (Normal, Clustered, Heavy Plume), Progressive Level Mix, All-Level Sun Pockets & Altitude Stack

* **Milestone:** Conducted a comprehensive cross-document audit and locked the final Game Design specifications across `game-design-document-tr.md`, `do-not-smoke-one-pager.md`, and `TODO.md`.
* **Decisions & Rationale:**
  * *Grounded Industrial Chimney Archetypes (Normal, Clustered, Heavy Plume):* Rejected fantastical fire-shooting Flare Stacks (`NS_FlareFire`), Data Centers, and Power Lines in favor of 3 grounded real-world industrial smokestack types that can be placed at standard (`~1000–1400u`) or low (`~600u`) altitudes:
    * **Type A (`Normal` — `SM_Smokestack_Single`):** Standard single factory chimney with non-lethal smoke.
    * **Type B (`Çoklu / Clustered` — `SM_Smokestack_Cluster`):** Twin/triple thermal power plant chimneys placed side-by-side; because of the 5.0-second capsule cooldown, the player cannot seal all rims in one pass and must execute banking turnaround loops while avoiding adjacent chimney bodies.
    * **Type C (`Yoğun / Heavy Plume` — `SM_Smokestack_Heavy`):** High-pressure heavy industrial chimney emitting a pitch-black, dense smoke pillar (`NS_HeavyToxicSmog`). Entering this dense smoke column causes an immediate **Crash**, forcing the player to weave around the vertical smoke walls and shoot into the rim from an angled side flyby (`45°` cone).
  * *Progressive Cross-Level Archetype Distribution:* Rather than isolating one chimney type per level, established a progressive mix across all levels: **Level 1** (`80% Normal, 20% Clustered, 0% Heavy`), **Level 2** (`40% Normal, 50% Clustered, 10% Heavy` + low-altitude placements & occasional VIP jet), and **Level 3** (`20% Normal, 30% Clustered, 50% Heavy` + occasional VIP jet). Rationale: Creates smooth mechanical onboarding (introducing 1 clustered stack at the end of Level 1 and 1 heavy stack in Level 2) before the full multi-hazard climax in Level 3.
  * *Single Occasional Private Jet with Satirical Aerial Banner (`SM_JetBanner`) & Newspaper Front-Page Summary (`WBP_COP31_Summary`):* Removed chaotic cross-flying multi-jet fleets in Level 3 so difficulty remains squarely focused on the chimneys and their smoke columns. Instead, a single occasional VIP private jet passes overhead towing a banner with rotating satirical greenwashing slogans (*"ON MY WAY TO CLIMATE SUMMIT!"*, *"DON'T FORGET YOUR PAPER STRAW!"*, *"THOUGHTS & PRAYERS FOR THE PLANET"*, *"NET-ZERO BY 2099!"*), while the post-level summary screen presents a spinning newspaper front page whose headline dynamically reflects the player's 1–3 star rating. Rationale: Delivers sharp, memorable environmental satire in-game without bloating gameplay complexity.
  * *Explicit Vertical Altitude Stack (`200u` to `3000u`):* Standardized vertical thresholds across all specs: `200u` Kill Floor, `400u` Smog Bed ceiling (blocks sunlight and visibility), `600u` `"PULL UP!"` HUD/audio warning trigger (giving reaction margin before entering the smog at high dive speeds), `1800u` player spawn, `2600u` glider ceiling, and `3000u` stratospheric VIP jet spline altitude. Rationale: Eliminates the `400u` vs `600u` warning discrepancy between HUD and systems sections and gives Quark and Barlas a shared Z-axis reference prior to UE5 in-engine tuning.
  * *Expanded Crash Conditions (Ground, Chimney Bodies & Heavy Smoke Pillars):* Hitting the `200u` ground floor, physically colliding with chimney/factory bodies, or flying into an active Type C Heavy Smoke Pillar (`NS_HeavyToxicSmog`) triggers an immediate Crash and Instant Restart. Rationale: Makes flyby precision and angled side approaches high-stakes and tactile without needing fantastical fire traps.
  * *Binary Full-Speed Solar Charging via Shrinking Sun Pockets Across All Levels:* Rejected gradual solar charge-rate decay penalties in favor of binary charging (`%12.5/s` in open sunlight vs `%0/s` under the Smog Canopy) across all three levels. Unsealed chimneys progressively expand the overhead Smog Canopy, shrinking the open sunlight windows (**Sun Pockets / God-Rays**) and forcing the player to actively fly toward open light corridors to recharge.
  * *Baseline Carbon Emission Rate ($C_{\text{oran}} = 10\text{ kg/s}$):* Removed legacy `(60 sn)` timer text from Level 1 and established a concrete baseline emission rate of $10\text{ kg CO}_2/\text{s}$ per active chimney with initial $C_{\min}$ targets (`500 kg`, `1200 kg`, `2200 kg` for Levels 1–3).

### 2026-10-31 — Game Design Ratification: Net-Loss Energy Economy, Magnetic Aim Assist, Organic Sky Timer & Cumulative Carbon Scoring

* **Milestone:** Finalized and ratified all remaining core Game Design parameters with Quark, locking in flight physics rules, targeting assists, organic failure loops, stratospheric obstacles, and 3-star COP31 scoring architecture.
* **Decisions & Rationale:**
  * *Net-Loss Gliding & Strict Solar Boost Elevation:* Disallowed perpetual kinetic zoom-climb dynamics. Diving to gain speed and pitching up will never allow the glider to reach an altitude higher than its starting point; the glider always operates under a net altitude loss. Rationale: Eliminates abusive infinite soaring loops and firmly positions Solar Boost as the sole, deliberate currency for gaining altitude, reinforcing energy discipline and solar collection.
  * *Subtle Magnetic Aim Assist (~10° Cone) & Expanded Rim Hitbox:* Maintained the 45-degree forward cone clamp for launching, but added a subtle magnetic homing bias (~10° angle tolerance) towards the chimney rim paired with an expanded detection sphere (220–250 units radius). Rationale: In high-speed arcade flight (1800–3800 u/s), pure physical hole targeting leads to frustrating pixel-imperfect misses. Subtle magnetic homing retains high-skill flyby maneuvers while guaranteeing rewarding "flyby dunk" satisfaction.
  * *Natural Soft-to-Hard Timer over Digital Countdown:* Completely removed artificial digital countdown clocks from the HUD. Unsealed chimneys continuously darken the sky box, choking sunlight line traces and starving the glider of solar recharge. When battery depletes, the glider cannot climb, gradually sinking through the smog bed (400 units) into the kill floor (200 units). Rationale: Deepens thematic immersion (climate inaction organically terminates flight) and establishes ground collision (Crash) as the sole fail condition with instant restart.
  * *Stratospheric VIP Private Jets & HUD Warning:* Configured private jets to fly strictly above the glider's maximum ceiling altitude, eliminating direct pawn-to-mesh collisions. Warning telegraphed via a flashing red exclamation mark (`!`) at screen top and roaring jet SFX. Passing jets leave wake turbulence volumes (`NS_JetWake`) causing intense camera shake and downwash altitude loss. Rationale: Eliminates chaotic mid-air collision bugs in a 48h jam while emphasizing environmental/turbulence hazard and satirizing elite emissions.
  * *Continuous Cumulative Carbon Accounting & Minimum Baseline Star Rating:* Rather than arbitrary completion thresholds, active chimneys produce continuous carbon emissions ($ActiveChimneys \times Rate$). Established a theoretical minimum emission baseline ($C_{\min}$) for each level representing a perfect speed-run line. 3 stars awarded for $\le 1.25 \times C_{\min}$, 2 stars for $1.25 - 1.75 \times C_{\min}$, and 1 star for $> 1.75 \times C_{\min}$, alongside completion time and shot accuracy. Rationale: Directly connects player efficiency with COP31 carbon reduction metrics and encourages arcade replayability.

### 2026-10-30 — Smog Bed Environmental Hazard & Visual Ground Telegraph Architecture

* **Milestone:** Formulated and approved the "Smog Bed" environmental design, low-altitude visual telegraph system, and low-poly submerged city architecture.
* **Decisions & Rationale:**
  * *Volumetric Smog Bed as Altitude Telegraph:* Instead of exposing an intricate, high-detail city floor, an opaque, toxic smog/haze blanket (Exponential Height Fog up to 400 units altitude) will submerge the city level. Rationale: In 3D flight games, spatial depth perception and ground awareness are notoriously difficult; the smog layer clearly telegraphs the dangerous ground ceiling. Furthermore, it conceals the city, completely eliminating visual noise and keeping the player focused on high-priority targets (chimneys and private jets).
  * *Smog Bed Gameplay Hazard:* Diving below the smog layer blocks the upward vertical `LineTrace` to the sun (stopping solar recharge), triggers cockpit audio alarms, and flashes a prominent red "PULL UP!" HUD warning before hitting the lethal ground floor (200 units).
  * *Lightweight Free Low-Poly City Kit:* Rejected heavyweight photoreal city asset packs (which would bloat 48h jam build size and tank framerates). Opted for a lightweight, free stylized/low-poly building kit placed under the fog to provide ambient silhouette mass. Barlas will dedicate 100% of modeling bandwidth to high-impact hero assets (`SK_SolarGlider`, 3 modular chimneys, `SM_PrivateJet`, and logo branding).

### 2026-10-30 — Technical Architecture Lock, Kinematic Flight Model & Team Roster Finalization

* **Milestone:** Finalized gameplay technical parameters, kinematic flight physics model, 45-degree clamped trace targeting, 5.0-second cooldown baseline, discrete level maps architecture, and official team roster assignments.
* **Decisions & Rationale:**
  * *Kinematic Flight Math over Chaos Physics:* Decided against full physics simulation in favor of a deterministic, kinematic model calculated in `Tick` (or `FloatingPawnMovement`). Rationale: Chaos physics on an arcade glider during a 48-hour game jam introduces unpredictable collisions, floatiness, and balancing instability. Kinematic delta calculations with `RInterpTo` and velocity damping guarantee responsive, predictable controls.
  * *Decoupled Aiming with 45-Degree Cone Clamp:* The projectile launch vector points toward the free camera's crosshair screen-to-world trace target, clamped to a maximum 45-degree cone relative to the glider's `Forward Vector`. Rationale: Provides aiming flexibility during angled flyby passes while preventing absurd backward shots.
  * *5.0-Second Cooldown (Reduced from 7.0s):* Reduced capsule launcher reload cooldown from 7.0s to 5.0s. Rationale: 7.0 seconds penalized missed shots too harshly in a fast-paced 3-minute arcade run; 5.0 seconds maintains punishing turnaround risk while keeping the player in an engaging flow state.
  * *Discrete Multi-Map Architecture (`.umap` per Level):* Chose distinct level maps (`L_Level01_Valley`, `L_Level02_Corridor`, `L_Level03_Basin`) instead of a single continuous open-world streaming corridor. Rationale: Enables distinct post-level UI transitions (COP31 summary and carbon audit screens) and simplifies level staging and testing within 48 hours.
  * *Dynamic Vertical LineTrace for Sun Pockets:* Retained vertical line traces from the glider towards the sky (checking against volumetric smog collision) instead of static trigger boxes. Rationale: Dynamic cloud openings and shifting god-rays can be sampled without overhead from resizing or moving physical trigger volumes.
  * *Official Team Roster & Ownership:*
    * **Quark:** Lead Game Designer & Game Developer (Pawn Kinematics, Projectile Math, Splines, Level Architecture, Game Manager).
    * **Berf:** UI/UX Designer & Sound Designer (Flight HUD, COP31 Summary Screens, Audio Mix, SFX Suite).
    * **Barlas:** 3D Artist (Hero Glider, 3 Modular Chimneys, Private Jet Mesh, Environment Assets).

### 2026-10-30 — Title Lock ("Do Not Smoke"), Satirical Logo & 3-Level Obstacle Architecture

* **Milestone:** Game officially titled **"Do Not Smoke"**. Visual identity, 3-level environmental escalation with Private Jets, and Master GDD updated in Vault.
* **Decisions & Rationale:**
  * *Title & Visual Identity Selection:* Chosen "Do Not Smoke" with a satirical logo featuring the international red "No Smoking" prohibition circle where the cigarette is replaced by an industrial smoking chimney. Rationale: Instantly readable, witty, fits itch.io thumbnail visual demands, and bridges individual smoking prohibition with industrial pollution prohibition.
  * *3-Level Escalation Structure:* 
    * *Level 1 (Valley Workshops):* Pure onboarding. Static factory chimneys and vertical smoke columns only.
    * *Level 2 (Transport Corridor):* Introduces high-speed **Private Jets** passing close to the player. The wake turbulence and dirty contrails shake the glider and cause sudden altitude loss. Thematic rationale: Directly satirizes luxury high-carbon transport common at COP climate summits.
    * *Level 3 (Mega Petrochemical Basin):* Introduces **Flare Stacks** (periodic fire bursts) and dense cross-flying jet fleets under heavily constricted sky corridors.
  * *Shrinking Sunlight Pockets (Sun Pockets):* Open chimneys no longer just tick up an arbitrary failure bar; they cause the volumetric smog ceiling to swallow sunlight, leaving only isolated, moving God-Ray columns in Level 3 where the glider must dive and orbit to recharge.

### 2026-10-30 — Master Game Design Document (GDD) Completion & Systems Specification

* **Milestone:** Comprehensive Master GDD (`game-design-document-tr.md`) authored and ratified. Full numeric balancing tables, 3 target archetypes, sector progression beat chart, and dedicated asset bibles established for Ark1 and Ark2.
* **Decisions & Rationale:**
  * *Atomic Flight Parameters:* Established quantitative baselines (Base Velocity: 1800 units/s, Max Dive: 3800 units/s, Solar Charge Rate: 12.5%/s, Boost Thrust: +2200 units/s, Projectile Cooldown: 7.0s).
  * *Decoupled Flight and Aiming (WASD vs. Mouse Look):* Assigned 3D flight control exclusively to WASD keyboard inputs and reserved mouse movement strictly for free camera look and crosshair aiming.
  * *Missed Shot Penalty & Natural Turnaround:* Rejected forcing players into smoke collisions on missed shots to preserve 3D aerial freedom. Players can safely bank away from smoke, but capsule firing has an intentional reload cooldown (7.0s).

### 2026-10-30 — Concept Inception, Negative Feedback Resolution & One-Pager GDD Lock

* **Milestone:** Initial game design, COP31 thematic alignment, and 48-hour Unreal Engine 5 production scope finalized. One-pager GDD authored and stored in Vault.
