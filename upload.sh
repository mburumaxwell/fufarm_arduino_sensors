#!/bin/bash
export PATH=/opt/platformio/bin:$PATH

usage="Usage $0 esp32e_firebeetle2|esp32s3_devkitc";

if [ $# -ne 1 ]; then
    echo $usage
    exit 1;
fi

if [ $1 = "esp32e_firebeetle2" ]
then
    board=esp32e_firebeetle2
elif [ $1 = "esp32s3_devkitc" ]
then
    board=esp32s3_devkitc
else
    echo $usage
    exit 1;
fi

if [ -c /dev/ttyACM0 ]; then
  atty=/dev/ttyACM0
elif [ -c /dev/ttyACM1 ]; then
  atty=/dev/ttyACM1
else
  echo Cannot find valid tty file
  exit 1
fi
echo Using terminal: $atty

# Command format is pio run --environment {env-name}
pio run --environment $board
if [ $? -ne 0 ]; then
  echo Failed to build!
  exit 1
fi

# Command format is pio run --environment {env-name} --target upload --upload-port {port}
# https://docs.platformio.org/en/latest/projectconf/sections/env/options/upload/upload_port.html
pio run --environment $board --target upload --upload-port $atty
if [ $? -ne 0 ]; then
  echo Failed to upload!
  exit 1
fi

echo "Done"
echo "To monitor the serial console run:  pio device monitor --environment $board"
