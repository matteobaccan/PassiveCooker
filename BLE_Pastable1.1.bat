tool\arduino-cli core install arduino:mbed_nano
tool\arduino-cli.exe --fqbn arduino:mbed_nano:nano33ble --libraries ./libraries compile BLE_Pastable1.1

::tool\arduino-cli upload -p com3 --fqbn arduino:mbed_nano:nano33ble -i BLE_Pastable1.1/BLE_Pastable1.1.ino.bin
