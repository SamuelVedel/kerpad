# Kerpad

This program makes your mouse move automaticaly while touching the edge of a touchpad.

To do that it uses an ebpf program that hooks a specific function of the linux kernel, and comunicates with another program in userpace. This solution is maybe overkill but it allows it to work in every situations.

## How to use it

### Dependencies

To compile, this program depends on
 - clang
 - llvm
 - libsystemd
 - libbpf (see [https://github.com/libbpf/libbpf](https://github.com/libbpf/libbpf))
 - ecc and ecli (see [https://github.com/eunomia-bpf/eunomia-bpf](https://github.com/eunomia-bpf/eunomia-bpf))

ecc and ecli program need to be in a place referenced by `$PATH` (like `/usr/bin/` for instance).
ecli is not necessary if you dont use coorpad (see n[Configure the edge limits](#configure-the-edge-limits))

### Compile and run it

Once you have all the required dependencies, you can compile it with the command:
```
make
```
And run it with the command:
```
sudo ./kerpad
```

While the program is running, you can stop it by typing `CTRL-C`

If you are not in the project directory or if you have moved the `build/kerpad.bpf.o` file, you can specify the path to this file by typing:
```
sudo ./kerpad -b <path/to/kerpad.bpf.o>
```

If you want this program to run at the boot of you system, you can run:
```
make install
sudo systemctl daemon-reload
sudo systemctl enable kerpad.service
```
This will install and enable a systemd service for kerpad (it will also copy `kerpad` to `/usr/bin`).

If you have moved `build/kerpad.bpf.o`, run
```
BPF_OBJECT=</absolute/path/to/kerpad.bpf.o> make kerpad.service
```
before the previous commands (you may have to remove `kerpad.service` if it already exists)

You can change the template for `kerpad.service` by editing `kerpad.service.template`

## How to configurate it

### Configure the edge limits

The edge limits are the limits on the touchpad after which the the mouse will start to move automaticaly. Those limits are defined by macro in the `bpf/kerpad.bpf.c` program. Those limits might not be relevent for your personal touchpad so you are free to change them and recompile the code.

To determine which edge limits fits the best for you, I programmed another ebpf program that print the coordinates on the touchpad while touching it. To use it, you can simply do:
```
make coorpad
make run_coorpad
```
And then you will see the coordinates while touching the touchpad.

### Configure mouse speed

At the top of the `user/usrpad.c` there are two macro `SLEEP_TIME` and `CURSOR_SPEED`. While you are touching the edge of your touchpad, the mouse move `CURSOR_SPEED` pixels each `SLEEP_TIME` miliseconds (the sleep time is longer while toucher a corner).

You can change those macro if you want to change the mouse speed.
