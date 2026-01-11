![License](https://img.shields.io/github/license/mausimus/ShaderBeam?color=red) ![GitHub Stars](https://img.shields.io/github/stars/mausimus/ShaderBeam?color=yellow) ![Downloads](https://img.shields.io/github/downloads/mausimus/ShaderBeam/total) ![Latest Release](https://img.shields.io/github/release-date/mausimus/ShaderBeam?label=latest%20release&color=blue) ![Beta Release](https://img.shields.io/github/release-date-pre/mausimus/ShaderBeam?label=beta%20release&color=orange)

## ShaderBeam

Overlay for running BFI/CRT Beam Simulation shaders on top of Windows desktop.

ShaderBeam allows you to experience motion clarity delivered by
[Blur Buster's CRT simulation technology](https://blurbusters.com/crt-simulation-in-a-gpu-shader-looks-better-than-bfi/)
on top of games, video and any other content.

ShaderBeam focuses on motion clarity only, if you're looking for scanline emulation,
check out its sister app [ShaderGlass](https://github.com/mausimus/ShaderGlass).

### Requirements

* High-refresh monitor (100 Hz or more, 240 Hz+ recommended)
* Windows 10/11 (latest Windows 11 recommended, Windows 10 will have yellow border)
* Recommended: a second dGPU (or iGPU) to reduce desync issues
* Optional: RivaTuner Statistics Server (RTSS) for frame-limiting content

### Download

First public beta (v0.1, 28 Dec 2025) - [report feedback here](https://github.com/mausimus/ShaderBeam/discussions/2)

https://github.com/mausimus/ShaderBeam/releases/download/v0.1/ShaderBeam-0.1-win-x64.zip

> [!WARNING]
> This app has the potential to create rapidly flashing images.
If you are photosensitive please take necessary precautions.

### What To Expect

If you have a high refresh monitor, you can try the CRT Beam Simulator
in browser here: [https://testufo.com/crt](https://testufo.com/crt)

Flat-panel displays create a lot of blur around moving objects and this technology
brings back motion clarity that CRTs had.

ShaderBeam applies the same effect to your whole desktop, including
fullscreen gaming.

### Demo

Click to view on YouTube

[![ShaderBeam (YouTube)](https://img.youtube.com/vi/LAbb6RqXdIg/maxresdefault.jpg)](https://www.youtube.com/watch?v=LAbb6RqXdIg)

### Performance Tips

Due to extreme timing requirements ShaderBeam performs best in simple setups
with no other apps or overlays running. Make sure to try these:

* Disable "Hardware-accelerated GPU Scheduling" (Windows Settings: Display -> Graphics -> Advanced)
* Disable VRR (G-Sync, FreeSync etc.) and Smart VSync (ShaderBeam needs to run at VSync)
* Disable HDR, it's not currently supported
* Disable MPO using registry files from [here](https://nvidia.custhelp.com/app/answers/detail/a_id/5157/~/after-updating-to-nvidia-game-ready-driver-461.09-or-newer%2C-some-desktop-apps) 
* Use only one monitor and disconnect any others
* Set GPU settings to maximum performance (NVIDIA Control Panel: Power management mode)
* Ensure "Background Application Max Frame Rate" is Off (NVIDIA Control Panel)
* Use Process Lasso to max GPU priority of ShaderBeam process
* Desktop can be a mixed bag, best used with borderless fullscreen games

### Second GPU provides best experience

By using a second GPU to run the shader, it's possible to **eliminate desync/flashing
issues** in most games. The shader has been optimized and at lower resolutions/refresh rates you
can even use an iGPU (for example Intel UHD 770 can do 1080p at 800fps+). You can use the Benchmark
button in ShaderBeam to estimate maximum shading rate of your GPU.

You do not need to connect a display to the secondary GPU, keep everything plugged into
your primary GPU. In ShaderBeam, simply change Shader GPU to your iGPU or secondary dGPU.

### Usage Guide

> [!IMPORTANT]
> If you have an OLED, make sure to select it to disable LCD Anti-retention. It causes occasional
> stutters as it desyncs CRT refresh rate from content refresh rate, and is not needed for OLEDs.

* ShaderBeam is fullscreen-only, you can change the display to use via UI.
* Upon startup ShaderBeam will automatically start simulation using default parameters.
* You can change various render and shader parameters using the UI overlay.
* Use pop-up tooltips to learn more about available options.
* Click away from the UI to hide it and get back into the game (might need to click twice), use the hotkey to bring menu back.
* Current global hotkeys are:
  * Toggle UI -- Ctrl+Shift+B
  * Force on top -- Ctrl+Shift+G
  * Restart -- Ctrl+Shift+A
  * Quit -- Ctrl+Shift+Q

### Troubleshooting

#### > I get irregular flashing

This happens when ShaderBeam isn't given enough GPU time by the OS to present a frame
on time. It's more likely to happen the heavier game you are playing, and also some
games (especially Unreal Engine ones) create unavoidable GPU stalls even at low details.

Try all the Tips above, especially disabling HAGS and MPO, or an older game.
The best way to avoid this issue is to use a **second GPU for ShaderBeam**.
You can also try "Simple BFI" shader which is much simpler and extremely fast,
but can leave temporary afterimages after longer use.

Also double-check that Rendered FPS equals Display Hz. If Rendered FPS is
below Display Hz it means your Shader GPU isn't keeping up (try lower resolution
or refresh rate). If Rendered FPS is above Display Hz it means VSync
isn't being applied - check GPU settings and disable any VSync overrides
(like Smart VSync etc.).

#### > Game capture is choppy

For best results you can try limiting your game's frame rate to exact time of a subframe,
i.e. on a "240 Hz" display which could actually be running at 239.76 Hz, the frame rate should be
59.94 (~240/60 = 4 subframes). The best way to do this is:
* in-game set unlimited frames and v-sync on
* use RivaTuner Statistics Server (RTSS) to limit the game's framerate to exact
value shown under "Content FPS" in ShaderBeam (including decimals)
* be careful not to limit ShaderBeam's framerate in RTSS! use profiles!

Sometimes capture API can start lagging, press Ctrl+Shift+A to
restart ShaderBeam and resync.

You can also try disabling LCD Anti-retention (it's not necessary on OLEDs at all, only on LCDs)
as it can introduce choppiness to the capture.

#### > ShaderBeam isn't on top of my game

Whenever this happens, press Ctrl+Shift+G to force ShaderBeam on top.
Some games will occasionally kick ShaderBeam out as they use the same topmost window flag.

### License

[MIT](LICENSE)

### Acknowledgements

ShaderBeam incorporates the following software under MIT License:
* [Blur Busters CRT Beam Simulator](https://github.com/blurbusters/crt-beam-simulator) Copyright (c) 2024 Mark Rejhon & Timothy Lottes
* [Dear ImGui](https://github.com/ocornut/imgui) Copyright (c) 2014-2025 Omar Cornut
* [Proggy Vector Font](https://github.com/bluescan/proggyfonts) Copyright (c) 2004, 2005 Tristan Grimmer
* [mINI](https://github.com/metayeti/mINI) Copyright (c) 2018 Danijel Durakovic
