## 🌌 Wormhole [Window Manager] 🌌

[![CI](https://github.com/Benjru/Wormhole/actions/workflows/ci.yml/badge.svg)](https://https://github.com/Benjru/Wormhole/actions/workflows/ci.yml)

Wormhole is a lightweight X window manager written in C using the XCB library. Currently, functionality is somewhat limited and bugs are to be expected. Logic has been implemented to handle reading in keybind configurations, displaying a window when an application is launched, reparenting (bar-drawing) and resizing windows. However, features such as automatic tiling have not yet been implemented.

## Gallery
### Screenshot
![wh_screenshot](https://github.com/Benjru/Wormhole/assets/95383688/e23278dd-a228-49b1-9336-7500154fb3c1)
### Demonstration
![Desktop_2023 05 23_-_01 12 05 05_V3](https://github.com/Benjru/Wormhole/assets/95383688/184c398d-67f0-4ca4-983b-87bc9d35fe83)


## Compiling from source

To compile this program from source, first you will need to clone this repository with the following command:

```git clone https://github.com/Benjru/Wormhole ```

Then, simply run the ```make``` command.

After compiling the program, you will need to ensure that it runs at startup. This can either be done manually by reconfiguring ```.xinitrc```, or alternatively, configuring the display/login manager of your choice to launch Wormhole.

## Configuring Wormhole
Currently, only keybinds are configurable (more coming soon). To change your keybinds, open the keybinds.conf file and follow the patterns specified there. This file is pre-loaded with a sample configuration.

## Known issues
This software is still in the very early stages of development; a list of known bugs is available under the issues tab. Until this software becomes more stable, I do not recommend installing it outside of a virtualized environment.
