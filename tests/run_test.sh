#!/bin/sh

# Runs audio_test with the provided arguments and dumps a number of benchmarks
# to the current working directory.

# Switch to lowest available power mode
echo "powersave" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
echo "powersave" > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor

# Start playback
audio_test ${@} &

# Wait for things to settle down a bit
sleep 5

# amixer settings, for sanity
amixer scontents > amixer.scontents
amixer contents > mixer.scontents

# Get some vmstat numbers
vmstat -n 6 > vmstat

# Collect per-thread top numbers as well
top -m 100 -n 1 -s cpu -t > top.0
top -m 100 -n 1 -s cpu -t > top.1
top -m 100 -n 1 -s cpu -t > top.2
top -m 100 -n 1 -s cpu -t > top.3
top -m 100 -n 1 -s cpu -t > top.4
top -m 100 -n 1 -s cpu -t > top.5

# And some wakeup statistics

powertop -d > powertop.0
powertop -d > powertop.1
powertop -d > powertop.2
powertop -d > powertop.3
powertop -d > powertop.4
powertop -d > powertop.5

# Switch back to the normal governer
echo "interactive" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
echo "interactive" > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor
