#!/bin/bash
if [ -n "`lsmod | grep kempld_core`" ]
then
	if [ -n "`lsmod | grep i2c_kempld`" ]
	then
		rmmod i2c_kempld
	fi

	rmmod kempld_core
	insmod drivers/kempld-master/kempld-core.ko
	echo "Module kempld-core charge"
else
	insmod drivers/kempld-master/kempld-core.ko
	echo "Module kempld-core charge"
fi

if [ -n "`lsmod | grep i2c_kempld`" ]
then
	rmmod i2c_kempld
	insmod drivers/kempld-master/i2c-kempld.ko scl_frequency=100
	echo "Module i2c-kempld charge"
else
	insmod drivers/kempld-master/i2c-kempld.ko scl_frequency=100
	echo "Module i2c-kempld charge"
fi

echo "I2C 0 :"
i2cdetect -y 0
chmod 666 /dev/i2c-0
#echo ""
#echo "I2C 1 :"
#i2cdetect -y 1

#programs/main3

