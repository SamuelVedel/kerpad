---
title: KERPAD
section: 1
header: Kerpad Manual
footer: kerpad
date: 2025-06-04
---

# NAME
kerpad - touchpad edge motion

# SYNOPSIS
**kerpad** [OPTION]...

# DESCRIPTION
This program implements a customizable edge motion to make your mouse move automatically while touching the edge of your touchpad. To do that, this program finds and listens to a /dev/input/eventXX file and simulates a mouse for the movement.

```
┌───────────────────────────────────┐
│   min_x     thickness     max_x   │
│min_y──────────────────────────┐   │
│   │                           │   │
│   │                           │   │
│   │                           │   │
│   │         touchpad          │   │
│   │                           │   │
│   │                           │   │
│   │                           │   │
│max_y──────────────────────────┘   │
│                                   │
└───────────────────────────────────┘
```
The above drawing represents you touchpad. When your  touchpad is pressed (or double tapped), if your finger is between the two squares (at the edge of the touchpad), then this program will make the mouse move automatically. By  default, the edge limits are defined automatically in relation to your touchpad size and a default thickness (=250). If you don't like the default limits, you can use options to change the thickness or directly the limit values.

# OPTIONS
**-t** edge_thickness, **-\-thickness**=edge_thickness
: Change the edge thickness.

**-x** min_x, **-\-minx**=min_x
: Change min_x.

**-X** max_x, **-\-maxx**=max_x
: Change max_x.

**-y** min_y, **-\-miny**=min_y
: Change min_y.

**-Y** max_y, **-\-maxy**=max_y
: Change max_y.

**-s** sleep_time, **-\-sleep_time**=sleep_time
: When edge motion is triggered, the mouse will move one pixel each sleep_time microseconds. The sleep time will be slightly longer when touching a corner. The default sleep time is 3000.

**-n** name, **-\-name**=name
: Specify the touchpad name.

**-a**, **-\-always**
: Activate edge motion even when the touchpad is justed touched.

**-\-no_edge_protection**
: Don't ignore touches made beyond the edge limits.

**-\-disable_double_tap**
: Don't concider the touchpad pressed, when it is double tapped.

**-l**, **-\-list**[=WHICH]
: List caracteritics of input devices. WHICH value can be:

> candidates: list only candidate devices (default value)

> all: list all input devices

**-v**, **-\-verbose**
: Display coordinates while pressing the touchpad. If combine with **-a**, it will display the coordinates even when the touchpad is just touched, this is useful to configure to edge limits.

**-h**, **-\-help**
: Display an help and exit.

# EXIT STATUS

The following exit values shall be returned:

0
: The program worked correclty.

\>0
: An error occurred.

# BUGS

This program does not support the multi-touch protocol yet.

# AUTHOR
Written by Samuel Vedel.

# SEE ALSO
GitHub: [https://github.com/SamuelVedel/kerpad](https://github.com/SamuelVedel/kerpad)
