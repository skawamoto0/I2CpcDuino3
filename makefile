I2CpcDuino3 : I2CpcDuino3.o
	gcc -O2 -o I2CpcDuino3 I2CpcDuino3.o -lrt

I2CpcDuino3.o : I2CpcDuino3.c
	gcc -c I2CpcDuino3.c

