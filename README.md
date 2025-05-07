# Kerpad

This program makes your mouse move automaticaly while touching the edge of a touchpad.

To do that, this program finds and listens to a `/dev/input/eventXX` file, and simulates a mouse for the movement.

This progam was made to work on linux. It might work on other unix-like operating systems, but there is no warranty.

The first version of this program was using ebpf to work, this was overkill and consumed more cpu cycles. If you are curious, you can still find this version on the ebpf branch of this repo.

## How to use it

### Dependencies

To compile, this program depends on
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

While the program is running, you can stop it by typing `CTRL-C`. When you type `CTRL-C`, you have to touch the touchpad a last time for the program to stop.

By default, the edge motion only work when the touchpad is pressed, or when you doubled-touched it. But you can have edge motion by just touching it if you run:
```
sudo ./kerpad -a
```

If you want this program to run at the boot of your system, you can run:
```
make install
sudo systemctl daemon-reload
sudo systemctl enable kerpad.service
```
This will install and enable a systemd service for kerpad (it will also copy `kerpad` to `/usr/bin`).

If you want to use aditional kerpad arguments for the service, you can do:
```
make kerpad.service KERPAD_ARGS='<args>'
```
before the previous commands (you may have to remove `kerpad.service` if it already exists)

You can change the template for `kerpad.service` by editing `kerpad.service.template`

## How to configure it

### Configure the edge limits

The edge limits are the limits on the touchpad after which the the mouse will start to move automaticaly. There are defaults values for those limits. However, those default values might not be relevent for your personal touchpad so you can configure them with kerpad arguments. See:
```
./kerpad -h
```

To determine which edge limits fits the best for you, you can do:
```
sudo ./kerpad -va
```
This will display the coordinates on the touchpad while touching it.

### Configure mouse speed

At the top of the `src/main.c` there are two macro `SLEEP_TIME` and `CURSOR_SPEED`. While you are touching the edge of your touchpad, the mouse move `CURSOR_SPEED` pixels each `SLEEP_TIME` microseconds (the sleep time is longer while touching a corner).

You can change those macro if you want to change the mouse speed.
