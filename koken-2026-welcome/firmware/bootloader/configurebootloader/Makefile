all : flash

TARGET:=configurebootloader
TARGET_MCU?=CH32V003

CH32FUN:=../../lib/ch32fun/ch32fun
include $(CH32FUN)/ch32fun.mk

flash : cv_flash
clean : cv_clean

