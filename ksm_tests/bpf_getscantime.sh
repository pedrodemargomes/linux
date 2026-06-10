#!/bin/bash

bpftrace -e 'tracepoint:ksm:ksm_start_scan { @start_ns = nsecs; } tracepoint:ksm:ksm_stop_scan { $elapsed = (nsecs - @start_ns); printf("KSM scan finished in %llu ns\n", $elapsed); delete(@start_ns); }'
