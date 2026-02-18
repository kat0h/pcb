all : flash

TARGET:=demo_composite_hid
CH32FUN:=../lib/ch32fun/ch32fun
TARGET_MCU:=CH32V003

ADDITIONAL_C_FILES+=../lib/rv003usb/rv003usb/rv003usb.S ../lib/rv003usb/rv003usb/rv003usb.c
EXTRA_CFLAGS:=-I../lib/ch32fun/lib -I../lib/rv003usb/rv003usb -I../lib/rv003usb/lib

include $(CH32FUN)/ch32fun.mk

flash : cv_flash
clean : cv_clean
format:
	clang-format -i *.c *.h

