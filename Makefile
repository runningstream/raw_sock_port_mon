CC := gcc
#NO_DEBUG := -DNDEBUG
CCOPTS := -Wall -Werror -O2 -g ${NO_DEBUG} -Ibpf_source
OUTPUTS := raw_sock_port_mon
SUBDIRS := bpf_source
CLEAN_SUBDIRS := ${foreach dir, ${SUBDIRS}, ${dir}-SUBDIRCLEAN}

.PHONY: clean all ${SUBDIRS} subdirs

all: ${OUTPUTS}

raw_sock_port_mon: raw_sock.o output_help.o accounting.o frame_parsing.o tcp_logger.o json_serialize.o
	${CC} ${CCOPTS} $^ -o $@
	sudo setcap cap_net_raw+ep $@

json_serial_test: json_serial_test.o json_serialize.o
	${CC} ${CCOPTS} $^ -o $@

subdirs: ${SUBDIRS}

${SUBDIRS}:
	${MAKE} -C $@

raw_sock.o: bpf_source

%.o: %.c
	${CC} ${CCOPTS} -c $< -o $@

%-SUBDIRCLEAN:
	${MAKE} -C $* clean

# Don't recursively clean subdirs, so we don't delete bpf_ .h
clean:
	rm -f *.o ${OUTPUTS}

test: json_serial_test
	valgrind --leak-check=yes ./json_serial_test
