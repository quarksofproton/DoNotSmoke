---
title: "Do Not Smoke — Task Backlog"
domain: 01-Projects
category: Do-Not-Smoke
status: active
---

# TODO — Do Not Smoke

## Critical

* **L1 Star Thresholds (after the L1 expansion):** Quark finishes an optimal run at ~5500 kg, so 4000 kg is too low. L1 now has 18 stacks / 23 mouths (was 13). Working proposal: 3 stars 5000–6000 kg, 2 stars 6000–7000 kg, 1 star 7000+ kg. Not locked: L1 will get more stacks and a wider area first, then measure again and set `TargetMinCarbonKg` and the star bands. The single Type C stack stays for now as a test (Quark); remove it later to match the GDD 0% Heavy mix.
* **Jet Routes & Pass Length (Quark):** Jets fly at 4500u now, but each spline is ~162 km (≈58 s per pass at 2800 u/s), so passes are rare. Shorten the splines around the ±25000 play area, or raise `JetFlightSpeed`.
* **Focus — Polish L1 First:** Bring L1 to a good, complete state before L2/L3; the later sectors reuse its systems.

## High

* **Seal → Solar Panel Pivot (Quark & Berf):** Hitting a rim installs a solar panel on the chimney instead of sealing it shut, so the message reads as fossil-to-renewable transition. Update the GDD, architecture, one-pager and TODO wording first, then rename or rework the seal code (`IBPI_SealableTarget`, `CarbonSealProjectile`, `SM_Seal_*`) and the `SFX_Seal_Latch` SFX. The white steam is already gone: sealed rims show the `NS_SealSparkle` glint.
* **Solar Panel Install Effect (Berf & Barlas):** Every sealed mouth now shows Barlas's solar-panel cap (`SM_Seal_Single`, scaled to each shaft). Still open: a final install effect; `NS_SealSparkle` is the placeholder glint.

* **Remaining Sector Maps (`L_Level02_Corridor` & `L_Level03_Basin`):** Setup `L_Level02_Corridor` (40/50/10 mix + low stacks & VIP jet) and `L_Level03_Basin` (20/30/50 mix + heavy lethal plumes & VIP jet) under `Content/Game/Maps/`.
* **Audio Pass (Quark):** Barlas's pack is wired in (wind, alarm, fire, boost, latch, jet roar, heavy rumble, L1 music; the music is the first 3 min of the 8 min track). Listen in game and tune or drop weak ones (`WindVolumeRange`, `LevelMusicVolume`, attenuation). Still missing: dive turbulence, menu click. L2 music is only 6 s, so L2/L3 tracks need a second look.

## Medium

* **Next-Sector Flow:** The newspaper has a placeholder Next Level button; wire it once `L_Level02_Corridor` exists (after L1 polish).

* **Submerged City Blockout Pass (Barlas):** `Meshes/City/` kit is imported; verify placement under the `0–400u` Smog Bed in all levels and keep it performance-light.
* **Clean Up Superseded Meshes:** Old `SM_Smokestack_*`, `SM_Seal_*` and `SM_SolarGlider` folders are superseded by `Rev3/` and `SM_SolarGlider_V2`, and `Rev3/SM_Seal_Cluster` / `SM_Seal_Heavy` are no longer used by code; remove them once no map references them.
* **Dynamic Post-Processing & Lighting (Quark & Berf):** Configure Lumen lighting with cloud shadows and god-ray bursts breaking through cleared smoke sectors.
* **Promo Art & Packaging (Berf & Barlas):** The game icon is in place (exe, Linux window, project thumbnail, menu). Still needed: key art, itch.io cover banner and thumbnail.

## Low

* **VRAM Headroom:** The Editor uses ~4 GB of the 6 GB RTX 3060 on `L_Level01_Valley` and once crashed with a Vulkan out-of-memory while loading it. Not blocking now; if it recurs, profile Lumen, `VolumetricCloud` and Nanite first. The unused `NS_ValleySmog`, `M_SmokeCanopy` / `MI_SmokeCanopy` and the empty `WBP_FlightHUD` / `WBP_COP31_Summary` assets can also be deleted.
