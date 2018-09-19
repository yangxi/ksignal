KDIR = /lib/modules/`uname -r`/build
obj-m := ksignal.o
M := make -C ${KDIR} M=`pwd`

all:
	${M} modules
clean-kernel:
	${M} clean


user: read_signal_page
read_signal_page:read_signal_page.c
	gcc -o $@ $< -lpthread
clean-user:
	rm read_signal_page

clean: clean-user clean-kernel
