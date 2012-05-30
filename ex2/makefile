all: ex2.a

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

ivt.o: ivt.s
	mcc -c -a6 -g -w3 $< -o $@
	
ex2.a: flash.o input_panel.o LCD.o network.o timer.o ivt.o
	ar rv $@ $+ 

main.o: main.c
	mcc -c -a6 -g -w3 $< -o $@
	
a.out: main.o ex2.a device.svr3
	mcc -a6 $+ -o $@ -Hldopt=-m -Hldopt=-C,output=device.map -Hldopt=-uInterruptTable


tar:
	tar cvf avishl02_matanp01 flash input_panel LCD network timer ivt.s README makefile

clean: 
	rm -f avishl02_matanp01 ex2.a *.o

test: clean all tar
	~embsys/ARC/MetaWare/arc/bin/embsys_check_ex2.sh avishl02_matanp01

