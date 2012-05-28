all: flash.o input_panel.o LCD.o timer.o main.o a.out

flash.o: flash/flash.c flash/flash.h
	mcc -O0 -c -a6 -g -w3 $< -o $@

input_panel.o: input_panel/input_panel.c input_panel/input_panel.h
	mcc -O0 -c -a6 -g -w3 $< -o $@
	
LCD.o: LCD/LCD.c LCD/LCD.h
	mcc -O0 -c -a6 -g -w3 $< -o $@

network.o: network/network.c network/network.h 
	mcc -O0 -c -a6 -g -w3 $< -o $@	

timer.o: timer/timer.c timer/timer.h
	mcc -O0 -c -a6 -g -w3 $< -o $@

main.o:	main.c timer.o LCD.o flash.o input_panel.o
	mcc -O0 -c -a6 -g -w3 $< -o $@   

a.out: main.o timer.o LCD.o input_panel.o flash.o ivt.s device.svr3
	mcc -a6 $+ -o $@ -Hldopt=-m -Hldopt=-C,output=device.map -Hldopt=-uInterruptTable

dump:
	rm -f hex.txt
	hexdump embsys_flash.img > hex.txt
	gedit hex.txt &

dump1: 
	rm -f hex.txt
	hexdump embsys_flash.img > hex.txt
	emacs hex.txt  	
tar:
	tar cvf avishl02_matanp01 ... device.svr3 makefile README

clean: 
	rm -f a.out main.o flash.o input_panel.o LCD.o network.o timer.o ivt.o

test: clean all tar
	~embsys/ARC/MetaWare/arc/bin/embsys_check_ex2.sh avishl02_matanp01

