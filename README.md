# Description
This tool counts the number of connections received at all TCP and UDP ports, all non-TCP/UDP IP protocols, and all non-IP ethernet DIX values.  The purpose is to provide you insight into the general shape of all traffic received by your computer.  It uses raw sockets to do this, so the tool doesn't have to listen on any specific ports.  It counts all packets/frames received.  It accounts for TCP connections with only SYN set separately from other TCP packets, considering those packets indicative of new connection attempts.

Count values are output every 10 minutes (time increment is configured via command line) via JSON message to a port and address specified via the command line.  I've designed this data to be received by FluentD or Logstash, slammed into Elasticsearch, and viewed via Kibana.  Thus, every port/new or all/source combination counted during the time period gets output as a separate message (aka document, if you've consumed enough Elasticsearch Kool Aid).  Messages are separated by newlines.

Low resource usage is a design goal - there are some large arrays allocated to count traffic which total 2 MB.  CPU usage is low when I've observed it, as work is kept minimal for each frame received.  Berkely Packet Filters are used to eliminate unwanted traffic before the program processes it.  Traffic eliminated by BPF includes IP traffic with a local-only source IP address (10.0.0.0/8, 172.16.0.0/12, 192.168.0.0/16), and traffic with a destination IP address different than the one specified on the command line (this latter filtering is optional).

IPV6 isn't currently handled well.

# Usage
```
./raw_sock_port_mon [-a log_addr] [-p log_port] [-e iface] [-d dst_ip]
         [-v] [-t output_period]
  -a log_addr    Address to connect to via TCP to output JSON logs
                 (default localhost)
  -p log_port    JSON logging TCP port (default 3334)
  -e iface       The interface on which to listen for connections
  -d dst_ip      Drop all IPV4 traffic except destined to this IP
  -t out_period  The time, in seconds, between JSON log output
  -v             Outputs verbose text info.  Specifying more than
                 once increases verbosity.
```

# Prerequisites
GCC is required.  For testing, valgrind is also required.  To recompile the Berkely Packet Filter header, you need m4 (a unix-standard templating/macro tool) and a copy of bpf\_asm.  You can build bpf\_asm with the Linux kernel - it's in the net tools directory.

You will also need a destination for the logging output - some TCP listener.  I recommend FluentD.

# Building
Just run `make`.

# Installation
Copy raw\_sock\_port\_mon into /usr/local/bin/.  If you have moved raw\_sock\_port\_mon across systems via scp or similar, you must set the raw sockets capability again (typically, this is accomplished via Makefile).  Set the capability with `sudo setcap cap_net_raw+ep /usr/local/bin/raw_sock_port_mon`.

You may want this to startup with your computer, managed by SystemD.  Below is an example service file that you'd drop in /etc/systemd/system/, after copying the executable into /usr/local/bin/raw\_sock\_port\_mon.  You should modify the command line options to meet your requirements - specifically, set the ethernet device you want to listen on.

```systemd
[Unit]
Description=Port Monitor Based on Raw Sockets

[Service]
Type=simple
User=nobody
Group=nogroup
ExecStart=/usr/local/bin/raw_sock_port_mon -e {{ ethernet_device }}
Restart=always
RestartSec=10
WorkingDirectory=/

[Install]
WantedBy=multi-user.target
```

# Testing
Testing is only built-in for the JSON serialization, verifying that everything allocated gets freed.  Having valgrind installed is a prerequisite...  Then run:

`make test`
