# DESCRIPTION
This repository contains the source code for a simple stand-alone zero-config XKB keyboard layout monitor.
This program has _very_ minimal RAM/CPU footprint and might come in handy if you are using [i3blocks](https://github.com/vivien/i3blocks).

Note: This program _only_ shows currently selected keyboard layout and makes no attempts to manage layouts based on active window ID or active window's process ID.

# COMPILATION/INSTALLATION
Run `make` in your command line to compile.

Note: A C11-capable compiler is required; Xlib development headers must be installed.

The compiled binary can be copied to the `/usr/local/bin/` directory using the following command.

```
sudo make install
```

The installed binary can be removed from the `/usr/local/bin/` directory using the following command.

```
sudo make uninstall
```

# USAGE

## Command line
To start monitoring the keyboard layouts, run:

```
xkbmon
```

## i3blocks
Add the following block to the [i3blocks](https://github.com/vivien/i3blocks) config file:

```
[xkbmon]
command=xkbmon
interval=persist
```

# LICENSE
Copyright Nezametdinov E. Ildus 2020.

Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
