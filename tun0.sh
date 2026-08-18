#!/bin/bash
# ============================================================================
#  setup-tun.sh  —  creates the pretend network card 'tun0' that our TCP
#  program attaches to. Run this once whenever WSL has been restarted and
#  the card is gone. Usage from a WSL terminal:
#
#      bash setup-tun.sh
#
#  (It will ask for your Linux password, because creating a network card
#   is an admin-only action.)
# ============================================================================

# If a leftover tun0 already exists, delete it so we start clean.
# The "2>/dev/null || true" part just means "don't complain if it wasn't there."
sudo ip tuntap del dev tun0 mode tun 2>/dev/null || true

# 1. Create the pretend card, named tun0, of the TUN kind, owned by YOU.
sudo ip tuntap add dev tun0 mode tun user "$(whoami)"

# 2. Give the card an address so Linux routes 10.0.0.x traffic to it.
sudo ip addr add 10.0.0.1/24 dev tun0

# 3. Turn the card on.
sudo ip link set tun0 up

echo ""
echo "Done. tun0 is ready. You can now Run the program."
echo ""
echo "Running tcpdump to view the packets travelling through tun0."
echo ""
sudo tcpdump -i tun0 -n -v