# Wait for caller to disconnect adb
sleep 20

# Fake run to get some stats
sh ./run_test.sh

mkdir af
cd af

sleep 240

mkdir 48
cd 48
sh ../../run_test.sh -b af -r 48000 ../../../g6-48.raw
cd ..

sleep 240

mkdir 44
cd 44
sh ../../run_test.sh -b af -r 44100 ../../../g6-44.raw
cd ..

cd ..

# wait for AF to release the device
sleep 240

start-pulseaudio-android -D
pactl set-sink-port 0 Handsfree

mkdir pa
cd pa

sleep 240

mkdir 48
cd 48
sh ../../run_test.sh -b pa -r 48000 ../../../g6-48.raw
cd ..

sleep 240

mkdir 44
cd 44
sh ../../run_test.sh -b pa -r 44100 ../../../g6-44.raw
cd ..
