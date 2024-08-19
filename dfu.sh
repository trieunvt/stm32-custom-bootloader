#!/system/bin/sh

echo "> Reset The Device"

gpioset gpiochip0 61=1
sleep 0.5
gpioset gpiochip0 61=0
sleep 0.5

echo "> Configure UART Device"

stty -F /dev/ttyHS3 115200 -echo

echo "> Query Device Bootloader Property Vault"

echo -en "ALL" > /dev/ttyHS3
read -n 256 ret < /dev/ttyHS3

echo "$ret"
ret=""

echo "> Trigger Device Firmware Update (DFU)"

echo -en "DFU" > /dev/ttyHS3
read -n 256 ret < /dev/ttyHS3

if [[ "$ret" != "OK" && "$ret" != "ok" ]]
then
    echo "FAILED"
    exit 1
fi

echo "$ret"
ret=""

echo "> Send New Device Firmware"

while read -r line
do
    echo -en "$line\n"
    echo -en "$line\n" > /dev/ttyHS3
    read -n 256 ret < /dev/ttyHS3

    if [[ "$ret" != "OK" && "$ret" != "ok" ]]
    then
        echo "FAILED"
        exit 1
    fi

    ret=""
done < "$1"

echo "Done"
