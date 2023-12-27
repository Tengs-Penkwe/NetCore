#!/bin/env sh

# Default values
default_tap="tap0"
default_bridge="br0"

# Function to print usage
usage() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "    -t <tap-device>      Specify TAP device to be deleted (default: $default_tap)"
    echo "    -b <bridge-device>   Specify Bridge device to be deleted (default: $default_bridge)"
    exit 1
}

# Parse command-line arguments
while [ "$1" != "" ]; do
    case $1 in
        -t ) shift
             tap=$1
             ;;
        -b ) shift
             bridge=$1
             ;;
        -h | --help )
             usage
             ;;
        * )  usage
             ;;
    esac
    shift
done

# Set defaults if not provided
tap=${tap:-$default_tap}
bridge=${bridge:-$default_bridge}

##### TAP Device Teardown #####
if [ ! -z "$tap" ]; then
    sudo ip link set $tap down
    sudo ip tuntap del dev $tap mode tap
fi

#### Bridge Teardown #####
if [ ! -z "$bridge" ]; then
    sudo ip link delete $bridge type bridge
fi

# Add other teardown procedures as needed
