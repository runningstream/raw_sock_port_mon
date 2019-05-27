bpf_bytecode.h: bpf_bytecode.asm bpf_bytecode_head
	cat bpf_bytecode_head > $@
	m4 $< | ./bpf_asm -c >> $@ || rm $@
	echo "};" >> $@

clean:
	rm -f bpf_bytecode.h
