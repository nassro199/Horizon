/**
 * ipv4.c - IPv4 protocol implementation
 * 
 * This file contains the implementation of the IPv4 protocol.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/net.h>
#include <horizon/mm.h>

/* IPv4 header structure */
typedef struct ipv4_header {
    u8 version_ihl;             /* Version and header length */
    u8 tos;                     /* Type of service */
    u16 total_length;           /* Total length */
    u16 id;                     /* Identification */
    u16 flags_fragment_offset;  /* Flags and fragment offset */
    u8 ttl;                     /* Time to live */
    u8 protocol;                /* Protocol */
    u16 checksum;               /* Header checksum */
    u32 src_addr;               /* Source address */
    u32 dst_addr;               /* Destination address */
} __attribute__((packed)) ipv4_header_t;

/* TCP header structure */
typedef struct tcp_header {
    u16 src_port;               /* Source port */
    u16 dst_port;               /* Destination port */
    u32 seq_num;                /* Sequence number */
    u32 ack_num;                /* Acknowledgment number */
    u16 flags;                  /* Flags */
    u16 window;                 /* Window size */
    u16 checksum;               /* Checksum */
    u16 urgent_ptr;             /* Urgent pointer */
} __attribute__((packed)) tcp_header_t;

/* UDP header structure */
typedef struct udp_header {
    u16 src_port;               /* Source port */
    u16 dst_port;               /* Destination port */
    u16 length;                 /* Length */
    u16 checksum;               /* Checksum */
} __attribute__((packed)) udp_header_t;

/* ICMP header structure */
typedef struct icmp_header {
    u8 type;                    /* Type */
    u8 code;                    /* Code */
    u16 checksum;               /* Checksum */
    u32 rest;                   /* Rest of header */
} __attribute__((packed)) icmp_header_t;

/* Protocol handlers */
static int ipv4_tcp_handler(void *data, size_t len);
static int ipv4_udp_handler(void *data, size_t len);
static int ipv4_icmp_handler(void *data, size_t len);

/* Protocol registrations */
static net_protocol_t ipv4_tcp_protocol = {
    .protocol = IPPROTO_TCP,
    .handler = ipv4_tcp_handler,
    .next = NULL
};

static net_protocol_t ipv4_udp_protocol = {
    .protocol = IPPROTO_UDP,
    .handler = ipv4_udp_handler,
    .next = NULL
};

static net_protocol_t ipv4_icmp_protocol = {
    .protocol = IPPROTO_ICMP,
    .handler = ipv4_icmp_handler,
    .next = NULL
};

/* Calculate the IPv4 header checksum */
static u16 ipv4_checksum(void *data, size_t len)
{
    u16 *buf = (u16 *)data;
    u32 sum = 0;
    
    /* Sum up 16-bit words */
    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }
    
    /* Add left-over byte, if any */
    if (len > 0) {
        sum += *(u8 *)buf;
    }
    
    /* Fold 32-bit sum to 16 bits */
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    /* Take the one's complement of the sum */
    return ~sum;
}

/* Handle an IPv4 packet */
int ipv4_handler(void *data, size_t len)
{
    ipv4_header_t *header = (ipv4_header_t *)data;
    
    /* Check the header length */
    u8 ihl = header->version_ihl & 0x0F;
    if (ihl < 5) {
        /* Invalid header length */
        return -1;
    }
    
    /* Check the version */
    u8 version = (header->version_ihl >> 4) & 0x0F;
    if (version != 4) {
        /* Invalid version */
        return -1;
    }
    
    /* Check the header checksum */
    u16 checksum = header->checksum;
    header->checksum = 0;
    if (ipv4_checksum(header, ihl * 4) != checksum) {
        /* Invalid checksum */
        return -1;
    }
    header->checksum = checksum;
    
    /* Handle the protocol */
    switch (header->protocol) {
        case IPPROTO_TCP:
            return ipv4_tcp_handler((u8 *)data + ihl * 4, len - ihl * 4);
        
        case IPPROTO_UDP:
            return ipv4_udp_handler((u8 *)data + ihl * 4, len - ihl * 4);
        
        case IPPROTO_ICMP:
            return ipv4_icmp_handler((u8 *)data + ihl * 4, len - ihl * 4);
        
        default:
            /* Unsupported protocol */
            return -1;
    }
}

/* Handle a TCP packet */
static int ipv4_tcp_handler(void *data, size_t len)
{
    tcp_header_t *header = (tcp_header_t *)data;
    
    /* Check the header length */
    if (len < sizeof(tcp_header_t)) {
        /* Invalid header length */
        return -1;
    }
    
    /* Process the TCP packet */
    /* This would be implemented with actual TCP processing */
    
    return 0;
}

/* Handle a UDP packet */
static int ipv4_udp_handler(void *data, size_t len)
{
    udp_header_t *header = (udp_header_t *)data;
    
    /* Check the header length */
    if (len < sizeof(udp_header_t)) {
        /* Invalid header length */
        return -1;
    }
    
    /* Process the UDP packet */
    /* This would be implemented with actual UDP processing */
    
    return 0;
}

/* Handle an ICMP packet */
static int ipv4_icmp_handler(void *data, size_t len)
{
    icmp_header_t *header = (icmp_header_t *)data;
    
    /* Check the header length */
    if (len < sizeof(icmp_header_t)) {
        /* Invalid header length */
        return -1;
    }
    
    /* Process the ICMP packet */
    /* This would be implemented with actual ICMP processing */
    
    return 0;
}

/* Initialize the IPv4 protocol */
void ipv4_init(void)
{
    /* Register the protocol handlers */
    net_protocol_register(&ipv4_tcp_protocol);
    net_protocol_register(&ipv4_udp_protocol);
    net_protocol_register(&ipv4_icmp_protocol);
}
