#include <linux/filter.h>

// To disable the dst-ipv4-based filter, find this value in the k values
// and change it to any other value.
__u32 bpf_code_dst_ipv4_enable = 0x90909090;

// To set the dst-ipv4-based filter's IP address, find this value in the
// k values and set it to the desired IP.
__u32 bpf_code_ipv4_standin = 0xa0a1a2a3;

// To set the dst-ipv6-based filter's IP address, find this value in the
// k values and set it to the desired IP.
__u32 bpf_code_ipv6_standin[][4] = { { 0xb0b1b2b3, 0xb4b5b6b7, 0xb8b9babb, 0xbcbdbebf } };

// A filter entry that simply accepts something
struct sock_filter accept_sock_filter = { 0x06, 0, 0, 0xffffffff };

struct sock_filter bpf_code[] = {
{ 0x28,  0,  0, 0x0000000c },
{ 0x15,  2,  0, 0x00000800 },
{ 0x15, 14,  0, 0x000086dd },
{ 0x06,  0,  0, 0xffffffff },
{ 0x30,  0,  0, 0x0000001a },
{ 0x15, 22,  0, 0x0000000a },
{ 0x28,  0,  0, 0x0000001a },
{ 0x54,  0,  0, 0x0000fff0 },
{ 0x15, 19,  0, 0x0000ac1f },
{ 0x28,  0,  0, 0x0000001a },
{ 0x15, 17,  0, 0x0000c0a8 },
{ 0000,  0,  0, 0x90909090 },
{ 0x04,  0,  0, 0x0000000a },
{ 0x15,  0, 13, 0x9090909a },
{ 0x20,  0,  0, 0x0000001e },
{ 0x15, 11,  0, 0xa0a1a2a3 },
{ 0x06,  0,  0, 0000000000 },
{ 0x20,  0,  0, 0x00000026 },
{ 0x15,  0,  7, 0xb0b1b2b3 },
{ 0x20,  0,  0, 0x0000002a },
{ 0x15,  0,  5, 0xb4b5b6b7 },
{ 0x20,  0,  0, 0x0000002e },
{ 0x15,  0,  3, 0xb8b9babb },
{ 0x20,  0,  0, 0x00000032 },
{ 0x15,  0,  1, 0xbcbdbebf },
{ 0x06,  0,  0, 0xffffffff },
{ 0x06,  0,  0, 0000000000 },
{ 0x06,  0,  0, 0xffffffff },
{ 0x06,  0,  0, 0000000000 },
};
