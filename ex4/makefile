all: a.out

flash.o: flash/flash.c flash/flash.h common_defs.h
	mcc -c -a6 -g -w3 $< -o $@

fs.o: fs/fs.c fs/fs.h flash/flash.h common_defs.h
	mcc -c -a6 -g -w3 $< -o $@	

input_panel.o: input_panel/input_panel.c input_panel/input_panel.h common_defs.h
	mcc -c -a6 -g -w3 $< -o $@

LCD.o: LCD/LCD.c LCD/LCD.h common_defs.h
	mcc -c -a6 -g -w3 $< -o $@

network.o: network/network.c network/network.h common_defs.h
	mcc -c -a6 -g -w3 $< -o $@	

timer.o: timer/timer.c timer/timer.h common_defs.h
	mcc -c -a6 -g -w3 $< -o $@

smsController.o: smsController.c smsController.h smsModel.h smsView.h timer/timer.h TX/tx_api.h common_defs.h input_panel/input_panel.h network/network.h embsys_sms_protocol_mine.h
	mcc -c -a6 -g -w3 $< -o $@

smsModel.o: smsModel.c smsModel.h input_panel/input_panel.h fs/fs.h common_defs.h TX/tx_api.h embsys_sms_protocol_mine.h
	mcc -c -a6 -g -w3 $< -o $@

smsView.o: smsView.c smsView.h smsModel.h LCD/LCD.h TX/tx_api.h common_defs.h
	mcc -c -a6 -g -w3 $< -o $@

embsys_sms_protocol_mine.o: embsys_sms_protocol_mine.c embsys_sms_protocol_mine.h
	mcc -c -a6 -g -w3 $< -o $@

ivt.o: ivt.s
	mcc -c -a6 -g -w3 $< -o $@

main.o: main.c TX/tx_api.h timer/timer.h common_defs.h smsController.h smsView.h smsModel.h
	mcc -c -a6 -g -w3 $< -o $@

a.out: main.o flash.o fs.o input_panel.o LCD.o network.o timer.o smsController.o smsModel.o smsView.o embsys_sms_protocol_mine.o ivt.o device.svr3 
	mcc -a6 $+ TX/tx.a -o $@ -Hldopt=-m -Hldopt=-uInterruptTable -Hldopt=-C,output=device.map

dump:
	rm -f flash_dump.txt
	hexdump embsys_flash.img > flash_dump.txt
	gedit flash_dump.txt &
	
dumpbug:
	rm -f flash_dump.txt
	rm -f embsys_flash_bug.img
	cp -i embsys_flash.img embsys_flash_bug.img
	hexdump embsys_flash.img > flash_dump.txt
	gedit flash_dump.txt &
	
tar:
	tar --exclude=".*" -cvjf avishl02_matanp01 flash fs input_panel LCD network timer TX common_defs.h device.svr3 embsys_sms_protocol_mine.c embsys_sms_protocol_mine.h ex4.args ivt.s main.c makefile README smsModel.c smsModel.h smsView.c smsView.h smsController.c smsController.h 

clean: 
	rm -f avishl02_matanp01 a.out *.o

test: clean all tar
	~embsys/ARC/MetaWare/arc/bin/embsys_check_ex4.sh avishl02_matanp01

