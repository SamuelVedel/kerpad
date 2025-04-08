// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>
#include <linux/input-event-codes.h>

char LICENSE[] SEC("license") = "Dual BSD/GPL";

struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, 2);
	__type(key, int);
	__type(value, int);
} kerpad_map SEC(".maps");

#define MINX 100
#define MINY 100

#define MAXX 3100
#define MAXY 2300

SEC("kprobe/input_event")
int BPF_KPROBE(handle_touchpad, struct input_dev *dev,
			   unsigned int type, unsigned int code, int value) {
	
	if (type == EV_ABS) {
		//bpf_printk("%d, %d, %d", type, code, value);
		if (code == 0) {// x
			int key = code;
			int new_value = 0;
			if (value < MINX) new_value = -1;
			else if (value > MAXX) new_value = 1;
			bpf_map_update_elem(&kerpad_map, &key, &new_value, BPF_ANY);
		} else if (code == 1) { // y
			int key = code;
			int new_value = 0;
			if (value < MINY) new_value = -1;
			else if (value > MAXY) new_value = 1;
			bpf_map_update_elem(&kerpad_map, &key, &new_value, BPF_ANY);
		}
	}
	
	return 0;
}
