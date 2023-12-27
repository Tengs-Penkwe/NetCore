#!/bin/env sh

# Default values
default_tap="tap0"
default_bridge="br0"
default_ip="10.0.2.1/24"
disable_ipv6=false
use_dhcp=false

# Function to print usage
usage() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "    -t <tap-device>      Specify TAP device (default: $default_tap)"
    echo "    -b <bridge-device>   Specify Bridge device (default: $default_bridge)"
    echo "    -i <ip-address>      Specify IP address (default: $default_ip)"
    echo "    --disable-ipv6       Disable IPv6 on the TAP device"
    echo "    --use-dhcp           Use DHCP for the bridge device"
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
        -i ) shift
             ip=$1
             ;;
        --disable-ipv6 )
             disable_ipv6=true
             ;;
        --use-dhcp )
             use_dhcp=true
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
ip=${ip:-$default_ip}

##### TAP Device Setup #####
if [ ! -z "$tap" ]; then
    sudo ip tuntap add dev $tap mode tap user tengs
    sudo ip link set $tap up
    sudo ip link set $tap promisc on
    sudo ip addr add $ip dev $tap
    if [ "$disable_ipv6" = true ]; then
        sudo sysctl -w net.ipv6.conf.$tap.disable_ipv6=1
    fi
fi

#### Bridge Setup #####
# if [ ! -z "$bridge" ]; then
#     sudo ip link add name $bridge type bridge
#     sudo ip link set $bridge up
#     sudo ip link set $tap master $bridge
#     if [ "$use_dhcp" = true ];
#     then  # Use dhclient
#         sudo dhclient $bridge
#     else  # Or use static IP
#         sudo ip addr add 10.0.2.2/24 dev $bridge
#         sudo ip route add default via 10.0.2.1
#     fi
# fi
