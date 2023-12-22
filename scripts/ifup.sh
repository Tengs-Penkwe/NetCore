#!/bin/env sh

tap="$1"
iface="$2"
bridge="$3"

# if [ -z "$tap" -o -z "$bridge" ];then
# 	echo "Usage: qemu-ifup.sh ethX brX" >&2
# 	exit 1
# fi

##### TAP Device #####
sudo ip link set $tap up
sudo ip link set $tap promisc on
sudo ip addr add 10.0.2.1/24 dev $tap

#### Bridge #####
# sudo ip link add name $bridge type bridge
# sudo ip link set $bridge up
# sudo ip link set $tap master $bridge

#### IP of Bridge #####
# Use dhclient
# sudo dhclient br0
# Or use static IP
# sudo ip addr add 10.0.2.2/24 dev $bridge
# sudo ip route add default via 10.0.2.1
