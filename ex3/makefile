all: a.out

flash.o: flash/flash.c flash/flash.h common_defs.h
	mcc -c -a6 -g -w3 $< -o $@

input_panel.o: input_panel/input_panel.c input_panel/input_panel.h common_defs.h
	mcc -c -a6 -g -w3 $< -o $@

LCD.o: LCD/LCD.c LCD/LCD.h common_defs.h
	mcc -c -a6 -g -w3 $< -o $@

network.o: network/network.c network/network.h common_defs.h
	mcc -c -a6 -g -w3 $< -o $@	

timer.o: timer/timer.c timer/timer.h common_defs.h
	mcc -c -a6 -g -w3 $< -o $@

ivt.o: ivt.s
	mcc -c -a6 -g -w3 $< -o $@

main.o: main.c TX/tx_api.h common_defs.h 
	mcc -c -a6 -g -w3 $< -o $@

a.out: main.o input_panel.o LCD.o network.o timer.o ivt.o device.svr3
	mcc -a6 $+ TX/tx.a -o $@ -Hldopt=-m -Hldopt=-C,output=device.map

tar:
	tar --exclude=".*" -cvjf avishl02_matanp01 flash input_panel LCD network timer ivt.s common_defs.h README makefile

clean: 
	rm -f avishl02_matanp01 a.out *.o

test: clean all tar
	~embsys/ARC/MetaWare/arc/bin/embsys_check_ex2.sh avishl02_matanp01

