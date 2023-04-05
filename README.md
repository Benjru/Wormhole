## 🚧 Under construction 🚧

Wormhole is an X window manager written in C using the XCB library. Currently, functionality is somewhat limited and bugs are to be expected. Logic has been implemented to handle reading in keybind configurations, displaying a window when an application is launched, and reparenting (bar-drawing). However, features such as automatic tiling and resizing have not yet been implemented

## Demonstration
![Studio_Project(2)](https://user-images.githubusercontent.com/95383688/226232685-d41fd5b6-9af4-4179-9751-f59aae8b98b0.gif)

## Compiling from source

To compile this program from source, first you will need to clone this repository with the following command:

```git clone https://github.com/Benjru/Wormhole ```

Then, simply run the ```make``` command.

After compiling the program, you will need to ensure that it runs at startup. This can either be done manually by reconfiguring ```.xinitrc```, or alternatively, configuring the display/login manager of your choice to launch Wormhole.
