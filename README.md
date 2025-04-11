# Kerpad

This program makes your mouse move automaticaly while touching the edge of a touchpad.

To do that it uses an ebpf program that hooks a specific function of the linux kernel, and comunicates with another program in userpace. This solution is maybe overkill but it allows it to work in every situations.

## How to use it

### Dependencies

To compile, this program depends on
 - clang
 - llvm
 - libbpf (see [https://github.com/libbpf/libbpf](https://github.com/libbpf/libbpf))
 - ecc and ecli (see [https://github.com/eunomia-bpf/eunomia-bpf](https://github.com/eunomia-bpf/eunomia-bpf))

ecc and ecli program need to be in a place referenced by `$PATH` (like `/usr/bin/` for instance).
ecli is not necessary if you dont use coorpad (see [Configure the edge limits](#configure-the-edge-limits))

### Compile and run it

Once you have all the required dependencies, you can compile it with the command:
```
make
```
And run it with the command:
```
sudo ./kerpad
```

Note that you need to be in the kerpad directory to run this program. It is something to improve in the future.

While the program is running, you can stop it by typing CTRL-C

## How to configurate it

### Configure the edge limits

The edge limits are the limits on the touchpad after which the the mouse will start to move automaticaly. Those limits are defined by macro in the `bpf/kerpad.bpf.c` program. Those limits might not be relevent for your personal touchpad so you are free to change them and recompile the code.

To determine which edge limits fits the best for you, I programmed another ebpf program that print the coordinates on the touchpad while touching it. To use it, you can simply do:
```
make coorpad
sudo make run_coorpad
```
And then you will see the coordinates while touching the touchpad.

### Configure mouse speed

At the top of the `user/usrpad.c` there are two macro `SLEEP_TIME` and `CURSOR_SPEED`. While you are touching the edge of your touchpad, the mouse move `CURSOR_SPEED` pixels each `SLEEP_TIME` miliseconds (the sleep time is longer while toucher a corner).

You can change those macro if you want to change the mouse speed.
