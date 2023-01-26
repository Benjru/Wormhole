## 🚧 Under construction 🚧

Wormhole is an X window manager written in C using the XCB library. Currently, functionality is very limited. Logic has been included to handle reading in keybind configurations, connecting to the X server, and displaying a window when an application is launched. However, features such as automatic tiling, reparenting and resizing floating windows have not yet been implemented

## Compiling from source

To compile this program from source using GCC, clone the repository, cd into the directory, and run the following command:

```gcc event_handler.c keybinds.c action_processor.c $(pkg-config --cflags --libs x11 xcb xcb-keysyms xkbcommon xkbcommon-x11) -o wormhole ```

After compiling the program, you will need to ensure that it runs at startup. This can be done either by manually reconfiguring ```.xinitrc```, or alternatively, configuring your display/login manager of choice to launch Wormhole.