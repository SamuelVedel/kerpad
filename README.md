# Kerpad

This program implements a customizable edge motion to make your mouse move automatically while touching the edge of your touchpad.

To do that, this program finds and listens to a `/dev/input/eventXX` file and simulates a mouse for the movement.

This program was made to work on Linux. It might work on other Unix-like operating systems, but there is no warranty.

This program does not support the multi-touch protocol yet.

The first version of this program used eBPF to work, but it was overkill and consumed more CPU cycles. If you are curious, you can still find this version on the ebpf branch of this repo.

## How to use it

### Dependencies

To compile, this program depends on:
 - gcc
 - make

### Compile and run it

Once you have all the required dependencies, you can compile it with the command:
```
make
```
And run it with the command:
```
sudo ./kerpad
```

While the program is running, you can stop it by typing `CTRL-C`. When you type `CTRL-C`, you have to touch the touchpad one last time for the program to stop.

By default, the edge motion only works when the touchpad is pressed or when you double-tap it. But you can have edge motion while touching it if you run:
```
sudo ./kerpad -a
```

By default, this program will try to find the device that looks the most like a touchpad, but you can tell it to listen to a specific device with:
```
sudo ./kerpad -n <device_name>
```

### Make it run at boot time

If you want this program to run at system boot, you can run:
```
make install
sudo systemctl daemon-reload
sudo systemctl enable kerpad.service
```
This will install and enable a systemd service for Kerpad (it will also copy `kerpad` to `/usr/bin`).

If you want to use additional `kerpad` options for the service, you can do:
```
make kerpad.service KERPAD_ARGS='<args>'
```
before the previous commands (you may have to remove `kerpad.service` if it already exists).

You can change the template for `kerpad.service` by editing `kerpad.service.template`.

## How to configure it

### Configure the edge limits

The edge limits are the boundaries on the touchpad beyond which the mouse will start to move automatically. By default, those limits are determined by the dimensions of the detected device. However, if you don't like the default values, you can easily change the edge thickness or directly set the limit values using `kerpad` options. See:
```
./kerpad -h
```

To determine which edge limits work best for you, you can run:
```
sudo ./kerpad -va
```
This will display the coordinates on the touchpad while you touch it.

### Configure the mouse speed

At the top of `src/main.c`, there are two macros: `SLEEP_TIME` and `CURSOR_SPEED`. While you are touching the edge of your touchpad, the mouse moves `CURSOR_SPEED` pixels every `SLEEP_TIME` microseconds (the sleep time is longer while touching a corner).

You can change these macros if you want to adjust the mouse speed.
