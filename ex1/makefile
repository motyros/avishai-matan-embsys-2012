all: a.out

7segments.o: 7segments.c 7segments.h defs.h
	mcc -c -a6 -g -w3 $< -o $@
 
clock.o: clock.c clock.h defs.h
	mcc -c -a6 -g -w3 $< -o $@

uart.o: uart.c uart.h defs.h
	mcc -c -a6 -g -w3 $< -o $@

cyclicBuffer.o: cyclicBuffer.c cyclicBuffer.h defs.h
	mcc -c -a6 -g -w3 $< -o $@

main.o: main.c 7segments.h clock.h defs.h
	mcc -c -a6 -g -w3 $< -o $@

a.out: main.o 7segments.o clock.o uart.o cyclicBuffer.o device.svr3
	mcc -a6 $+ -o $@ -Hldopt=-m -Hldopt=-C,output=device.map

7segments_test:
	mcc -a6 -g -w3 7segments_test.c 7segments.c device.svr3 -o a.out 
tar:
	tar cvf avishl02_matanp01 ... makefile README

clean:
	rm -f a.out 7segments.o main.o devices.map 7segments_test.o clock.o uart.o cyclicBuffer.o

test: clean all tar
	~embsys/ARC/MetaWare/arc/bin/embsys_check_ex1.sh avishl02_matanp01

