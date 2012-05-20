all: 

flash.o: flash/flash.c flash/flash.h
	mcc -c -a6 -g -w3 $< -o $@

input_panel.o: input_panel/input_panel.c input_panel/input_panel.h
	mcc -c -a6 -g -w3 $< -o $@
	
LCD.o: LCD/LCD.c LCD/LCD.h
	mcc -c -a6 -g -w3 $< -o $@

network.o: network/network.c network/network.h 
	mcc -c -a6 -g -w3 $< -o $@	

timer.o: timer/timer.c timer/timer.h
	mcc -c -a6 -g -w3 $< -o $@

main.o:	main.c timer.o
	mcc -c -a6 -g -w3 $< -o $@   

a.out: main.o timer.o ivt.s device.svr3
	mcc -a6 $+ -o $@ -Hldopt=-m -Hldopt=-C,output=device.map -Hldopt=-uInterruptTable

7segments_test:
	mcc -a6 -g -w3 7segments_test.c 7segments.c device.svr3 -o a.out 
tar:
	tar cvf avishl02_matanp01 ... device.svr3 makefile README

clean: 
	rm -f a.out main.o flash.o input_panel.o LCD.o network.o timer.o

test: clean all tar
	~embsys/ARC/MetaWare/arc/bin/embsys_check_ex2.sh avishl02_matanp01

