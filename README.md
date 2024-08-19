# STM32 Custom Bootloader

The STM32 custom bootloader implements the device firmware update (DFU) using UART on the STM32 NUCLEO-L452RE board.

## How To Use

### For The First Time

Flash the STM32 custom bootloader for the first time:

-   Build the STM32 custom bootloader project:

```sh
make -C stm32l452ret6-bootloader
```

-   Set up before flashing:

```sh
mkdir output
cp stm32l452ret6-bootloader/jflash.jflash output
cp stm32l452ret6-bootloader/build/stm32l452ret6-bootloader.hex output
```

-   Copy your application hex file to the output folder.
-   Merge the STM32 custom bootloader and your application:

```sh
JFlash -openprjoutput/jflash.jflash \
       -openoutput/stm32l452ret6-bootloader.hex \
       -mergeoutput/<your_application>.hex \
       -saveasoutput/bootloader-and-app.hex \
       -hide -exit
```

-   Flash the STM32 custom bootloader and your application to the device:

```sh
JFlash -openprjoutput/jflash.jflash \
       -openoutput/bootloader-and-app.hex \
       -erasechip -auto -startapp -hide -exit
```

### From Now On

From now on, the device firmware update (DFU) can be implemented using the shell script `dfu.sh`:

-   Add the new version to the head of your new application hex file:

```sh
sed -i '1s/^/'01.01.00'\n/' output/<your_new_application>.hex
```

-   Update the power GPIO `gpiochip0 61` and the UART device `/dev/ttyHS3` inside the shell script `dfu.sh` based on your device schematic design.
-   Implement the device firmware update (DFU):

```sh
sh dfu.sh output/<your_new_application>.hex
```

-   Enjoy.
