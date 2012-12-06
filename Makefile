all:
	make -C libebb
	make -f Makefile-mqtthttpd	

clean:
	-make -C libebb clean
	-make -f Makefile-mqtthttpd clean
	rm -f access.log mqtthttpd.pid

