all: ex2.a

flash.o: flash.c flash.h common_defs.h
	mcc -c -a6 -g -w3 $< -o $@

input_panel.o: input_panel.c input_panel.h common_defs.h
	mcc -c -a6 -g -w3 $< -o $@
	
LCD.o: LCD.c LCD.h common_defs.h
	mcc -c -a6 -g -w3 $< -o $@

network.o: network.c network.h common_defs.h
	mcc -c -a6 -g -w3 $< -o $@	

timer.o: timer.c timer.h common_defs.h
	mcc -c -a6 -g -w3 $< -o $@

ivt.o: ivt.s
	mcc -c -a6 -g -w3 $< -o $@
	
ex2.a: flash.o input_panel.o LCD.o network.o timer.o ivt.o
	ar rv $@ $+ 

main.o: main.c flash.o input_panel.o LCD.o network.o timer.o common_defs.h 
	mcc -c -a6 -g -w3 $< -o $@

a.out: main.o flash.o input_panel.o LCD.o network.o timer.o ivt.o device.svr3
	mcc -a6 $+ -o $@ -Hldopt=-m -Hldopt=-C,output=device.map

tar:
	tar -cvf avishl02_matanp01 flash.* input_panel.* LCD.* network.* timer.* ivt.s README makefile

clean: 
	rm -f avishl02_matanp01 ex2.a *.o

test: clean all tar
	~embsys/ARC/MetaWare/arc/bin/embsys_check_ex2.sh avishl02_matanp01

