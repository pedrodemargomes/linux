#!/bin/bash
echo never >  /sys/kernel/mm/transparent_hugepage/enabled 

echo 1 >  /sys/kernel/mm/ksm/sleep_millisecs
echo 999999999 >  /sys/kernel/mm/ksm/pages_to_scan
echo 0 >  /sys/kernel/mm/ksm/use_zero_pages
echo 0 > /sys/kernel/mm/ksm/smart_scan
echo 1 >  /sys/kernel/mm/ksm/run

