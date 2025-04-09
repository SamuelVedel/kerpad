// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>
#include <linux/input-event-codes.h>

#include "coorpad.h"

char LICENSE[] SEC("license") = "Dual BSD/GPL";

struct {
	__uint(type, BPF_MAP_TYPE_PERF_EVENT_ARRAY);
	__uint(key_size, sizeof(u32));
	__uint(value_size, sizeof(u32));
} perf_map SEC(".maps");

SEC("kprobe/input_event")
int BPF_KPROBE(handle_touchpad, struct input_dev *dev,
			   unsigned int type, unsigned int code, int value) {
	
	if (type == EV_ABS && (code == 0 || code == 1)) {
		struct coor pos = {.pos = value};
		if (code == 0) pos.xy[0] = 'x';
		else pos.xy[0] = 'y';
		bpf_perf_event_output(ctx, &perf_map, BPF_F_CURRENT_CPU, &pos, sizeof(pos));
	}
	
	return 0;
}

