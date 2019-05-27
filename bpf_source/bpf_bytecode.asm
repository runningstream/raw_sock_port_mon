define(`m_accept', `ret #-1')
define(`m_drop', `ret #0')

ldh [12]
jeq #0x0800, ipv4
jeq #0x86DD, ipv6_1
m_accept

ipv4:
# Drop traffic sourced from local hosts
ldb [eval(`14 + 12')]
jeq #10, drop
ldh [eval(`14 + 12')]
and #0xfff0 
# 0xac1f is 172.16
jeq #0xac1f, drop
# 0xc0a8 is 192.168
ldh [eval(`14 + 12')]
jeq #0xc0a8, drop

# Determine if destination-based filtering is enabled
# Replace this instance of 0x90909090 (only one in file...)
# with some other value to disable dest-based filtering
ld #0x90909090
add #0xa
jne #0x9090909a, accept

# Only accept traffic destined to a specific IP
ld [eval(`14 + 16')]
# Replace a0a1a2a3 with the 4 byte IP address to accept
jeq #0xa0a1a2a3, accept
m_drop

ipv6_1:
ld [eval(`14 + 24 + 0')]
jne #0xb0b1b2b3, ipv6_2 
ld [eval(`14 + 24 + 4')]
jne #0xb4b5b6b7, ipv6_2
ld [eval(`14 + 24 + 8')]
jne #0xb8b9babb, ipv6_2
ld [eval(`14 + 24 + 12')]
jne #0xbcbdbebf, ipv6_2
m_accept

ipv6_2:
# implement mult ipv6s by doing the same here as ipv6_1
m_drop

accept: m_accept
drop: m_drop
