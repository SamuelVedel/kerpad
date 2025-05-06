# Kerpad

This program makes your mouse move automaticaly while touching the edge of a touchpad.

To do that, this program finds and listens to a `/dev/input/eventXX` file, and simulates a mouse for the movement.

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

If you want this program to run at the boot of your system, you can run:
```
make install
sudo systemctl daemon-reload
sudo systemctl enable kerpad.service
```
This will install and enable a systemd service for kerpad (it will also copy `kerpad` to `/usr/bin`).

If you want to use aditional kerpad arguments for the service, you can do:
```
make kerpad.service KERPAD_ARGS=<args>
```
before the previous commands (you may have to remove `kerpad.service` if it already exists)

You can change the template for `kerpad.service` by editing `kerpad.service.template`

## How to configurate it

### Configure the edge limits

The edge limits are the limits on the touchpad after which the the mouse will start to move automaticaly. There are defaults values for those limits. However, those default values might not be relevent for your personal touchpad so you can configure them with kerpad arguments. See:
```
./kerpad -h
```

To determine which edge limits fits the best for you, you can do:
```
sudo ./kerpad -d
```
This will display the coordinates on the touchpad while touching it, instead of moving the mouse.

### Configure mouse speed

At the top of the `src/main.c` there are two macro `SLEEP_TIME` and `CURSOR_SPEED`. While you are touching the edge of your touchpad, the mouse move `CURSOR_SPEED` pixels each `SLEEP_TIME` miliseconds (the sleep time is longer while touching a corner).

You can change those macro if you want to change the mouse speed.
