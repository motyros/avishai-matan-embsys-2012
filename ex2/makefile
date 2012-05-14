all: 

flash.o: 
	mcc -c -a6 -g -w3 $< -o $@

input_panel.o: 
	mcc -c -a6 -g -w3 $< -o $@
	
LCD.o: 
	mcc -c -a6 -g -w3 $< -o $@

network.o: 
	mcc -c -a6 -g -w3 $< -o $@	

timer.o: 
	mcc -c -a6 -g -w3 $< -o $@
 
a.out: main.o flash.o uart.o input_panel.o LCD.o network.o timer.o cyclicBuffer.o device.svr3
	mcc -a6 $+ -o $@ -Hldopt=-m -Hldopt=-C,output=device.map

7segments_test:
	mcc -a6 -g -w3 7segments_test.c 7segments.c device.svr3 -o a.out 
tar:
	tar cvf avishl02_matanp01 ... device.svr3 makefile README

clean: 
	rm -f a.out flash.o input_panel.o LCD.o network.o timer.o cyclicBuffer.o

test: clean all tar
	~embsys/ARC/MetaWare/arc/bin/embsys_check_ex2.sh avishl02_matanp01

