tool\arduino-cli core install arduino:mbed_nano
::tool\arduino-cli lib install ArduinoBLE
::tool\arduino-cli lib install QuickStats
::tool\arduino-cli lib install CircularBuffer
tool\arduino-cli.exe --fqbn arduino:mbed_nano:nano33ble --libraries ./libraries compile BLE_Pastable1.1
