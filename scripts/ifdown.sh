#!/bin/env sh

tap="$1"
iface="$2"
bridge="$3"

# if [ -z "$tap" -o -z "$bridge" ];then
# 	echo "Usage: qemu-ifdown.sh ethX brX" >&2
# 	exit 1
# fi

##### TAP Device #####
sudo ip link set $tap down
sudo ip tuntap del dev $tap mode tap

#### Bridge #####
sudo ip link delete $bridge type bridge
