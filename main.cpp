// ============================================================================
//  Step 1 of building a TCP stack: open a TUN device and print raw packets.
//
//  A TUN device is a fake network interface. The Linux kernel treats it like a
//  network card, but instead of sending packets to hardware, it hands them to
//  THIS program through a file descriptor. When we read() from that fd, we get
//  a raw IP packet:  [ IP header ][ TCP/UDP/... ][ data ]
//
//  This file does nothing clever yet. It just proves the plumbing works:
//  it opens the TUN device and prints the bytes of whatever packet arrives.
//  Everything else (parsing IP, parsing TCP, the handshake) gets built on top.
// ============================================================================

#include <cstdio>       // printf, perror
#include <cstring>      // memset, strncpy, strcpy
#include <cstdint>      // uint8_t etc.
#include <unistd.h>     // read(), close()
#include <fcntl.h>      // open(), O_RDWR

// These four headers are LINUX-ONLY. This is why we compile in WSL, not Windows.
#include <net/if.h>         // struct ifreq  (describes a network interface)
#include <linux/if_tun.h>   // IFF_TUN, IFF_NO_PI, TUNSETIFF  (TUN-specific stuff)
#include <sys/ioctl.h>      // ioctl()  (the "configure a device" system call)
#include <sys/types.h>

// ----------------------------------------------------------------------------
//  tun_alloc: create/attach to a TUN device and return a file descriptor.
//
//  `dev` is the name we want (e.g. "tun0"). The kernel fills it back in with the
//  name it actually gave us. The returned int is a "file descriptor" — just a
//  number that represents our open connection to the device, like a ticket stub.
// ----------------------------------------------------------------------------
int tun_alloc(char *dev) {
    struct ifreq ifr;   // a struct the kernel uses to talk about interfaces
    int fd;

    // /dev/net/tun is the special file that lets us create TUN/TAP devices.
    // Opening it gives us a file descriptor to work with.
    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        perror("opening /dev/net/tun (are you root? does the device exist?)");
        return fd;
    }

    // Zero the struct out so no garbage fields are set.
    memset(&ifr, 0, sizeof(ifr));

    // Configure what KIND of device we want:
    //   IFF_TUN   -> a TUN device (Layer 3: we get raw IP packets, no Ethernet)
    //   IFF_NO_PI -> "No Packet Information": don't prepend 4 extra bytes of
    //                metadata to every packet. We just want the raw IP packet.
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

    // Copy in the desired device name ("tun0").
    if (*dev) {
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

    // ioctl = "do a device-specific command." TUNSETIFF means:
    // "register this file descriptor as the TUN device described by ifr."
    // After this succeeds, reading `fd` gives us packets destined for the device.
    if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0) {
        perror("ioctl(TUNSETIFF)");
        close(fd);
        return -1;
    }

    // The kernel may have adjusted the name; copy the real name back out.
    strcpy(dev, ifr.ifr_name);
    return fd;
}

int main() {
    char tun_name[IFNAMSIZ] = "tun0";   // the name we ask the kernel for

    int fd = tun_alloc(tun_name);       // open + configure the TUN device
    if (fd < 0) {
        printf("Failed to open TUN device. Did you run with sudo?\n");
        return 1;
    }

    printf("TUN device '%s' is open. Waiting for packets...\n", tun_name);
    printf("(Send some traffic to it from another terminal, e.g. ping 10.0.0.5)\n\n");

    unsigned char buffer[2048];   // a spot to hold one incoming packet

    while (true) {
        // read() blocks until a packet arrives, then copies its bytes into
        // `buffer` and returns how many bytes it wrote. This is a RAW IP packet.
        ssize_t nbytes = read(fd, buffer, sizeof(buffer));
        if (nbytes < 0) {
            perror("read from tun");
            close(fd);
            return 1;
        }

        // Print the packet as raw hex bytes, 16 per line, so you can SEE it.
        printf("Got a packet: %zd bytes\n", nbytes);
        for (ssize_t i = 0; i < nbytes; i++) {
            printf("%02x ", buffer[i]);   // %02x = one byte as 2 hex digits
            if ((i + 1) % 16 == 0) printf("\n");
        }
        printf("\n\n");

        // Next step (later): parse buffer[0..] as an IP header, then a TCP
        // header, and actually respond. For now we just look at the bytes.
    }

    // (We never reach here in this simple version.)
    close(fd);
    return 0;
}
