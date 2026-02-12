# ProStreet Time Trial Ghosts

A mod for Need for Speed: ProStreet that replaces Quick Race opponents with TrackMania-like replays

## Installation

- Make sure you have v1.1 of the game, as this is the only version this plugin is compatible with. (exe size of 3765248 or 28739656 bytes)
- Plop the files into your game folder.
- Start the game and press F5 to open the mod's menu and play the included Challenge Series.
- Enjoy, nya~ :3

## Features

- A built-in set of events in the Challenge Series to compete with other players on
- Drift scoring has been fixed, giving you the same amount of points regardless of whether you're using a controller or a keyboard, or have XtendedInput installed
- Ghosts are timed based on game ticks, meaning the timebug no longer applies anymore
- Ghosts include a checksum of the player's game files, allowing you to check if someone's made any potentially unfair VLT modifications
- Tank Unslapper is included in the mod, enabled by default, and ghosts are cleanly separated by whether they have the steering delay removed or not
- The mod includes a basic anti-cheat, preventing common methods of cheating
- The physics tickrate has been locked to 120, meaning Widescreen Fix no longer gives you an unfair advantage

## Building

Building is done on an Arch Linux system with CLion and vcpkg being used for the build process. 

Before you begin, clone [nya-common](https://github.com/gaycoderprincess/nya-common), [nya-common-nfsps](https://github.com/gaycoderprincess/nya-common-nfsps) and [CwoeeMenuLib](https://github.com/gaycoderprincess/CwoeeMenuLib) to folders next to this one, so they can be found.

Required packages: `mingw-w64-gcc`

You should be able to build the project now in CLion.
