CC = msp430-elf-gcc
CFLAGS = -I /opt/ti/gcc/include -L /opt/ti/gcc/include -mmcu=msp430fr5969 -Os

receive: receive.c common.c common.h
	$(CC) $(CFLAGS) -o receive.elf receive.c common.c
	mspdebug tilib "prog receive.elf"

emit: emit.c common.c common.h
	$(CC) $(CFLAGS) -o emit.elf emit.c common.c
	mspdebug tilib "prog emit.elf"

.PHONY: clean
clean:
	rm -f receive.elf
	rm -f emit.elf
