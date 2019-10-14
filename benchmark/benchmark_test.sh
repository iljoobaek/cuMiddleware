

########## LSF Test on GTX 1070
sudo taskset -c 1 schedtool -F -p 60 -e ./mid --policy lsf

# tag layer depth 0
sudo -E LD_LIBRARY_PATH=../../lib taskset -c 2 schedtool -F -p 58 -e python3.6 detect_ssd.py --fps 60
sudo LD_LIBRARY_PATH=../../lib taskset -c 3 schedtool -F -p 56 -e ./hog 30
sudo LD_LIBRARY_PATH=../../lib taskset -c 4 schedtool -F -p 52 -e ./surround_view
sudo LD_LIBRARY_PATH=./:../../lib taskset -c 6 schedtool -F -p 52 -e ./cuTestClient


########## Tag-Only Test on GTX 1070
sudo taskset -c 1 schedtool -F -p 60 -e ./mid --policy fifo

# tag layer depth 0
sudo -E LD_LIBRARY_PATH=../../lib taskset -c 2 schedtool -F -p 52 -e python3.6 detect_ssd.py --fps 50
sudo LD_LIBRARY_PATH=../../lib taskset -c 3 schedtool -F -p 52 -e ./hog
sudo LD_LIBRARY_PATH=../../lib taskset -c 4 schedtool -F -p 52 -e ./surround_view
sudo LD_LIBRARY_PATH=../:../../lib taskset -c 6 schedtool -F -p 52 -e ./cuTestClient




########## No CARSS Test on xavier

# tag layer depth 0
python3.6 detect_ssd.py
./hog
./surround_view
./cuTestClient


########## Tag-only Test on Xavier
sudo taskset -c 1 schedtool -F -p 60 -e ./mid --policy fifo

# tag layer depth 0
sudo -E LD_LIBRARY_PATH=../../lib taskset -c 2 schedtool -F -p 58 -e python3.6 detect_ssd.py
sudo LD_LIBRARY_PATH=../../lib taskset -c 3 schedtool -F -p 56 -e ./hog
sudo LD_LIBRARY_PATH=../../lib taskset -c 4 schedtool -F -p 52 -e ./surround_view
sudo LD_LIBRARY_PATH=./:../../lib taskset -c 6 schedtool -F -p 52 -e ./cuTestClient



########## LSF Test on Xavier
sudo taskset -c 1 schedtool -F -p 60 -e ./mid --policy lsf

# tag layer depth 0
sudo -E LD_LIBRARY_PATH=../../lib taskset -c 2 schedtool -F -p 58 -e python3.6 detect_ssd.py --fps 60
sudo LD_LIBRARY_PATH=../../lib taskset -c 3 schedtool -F -p 56 -e ./hog 30
sudo LD_LIBRARY_PATH=../../lib taskset -c 4 schedtool -F -p 52 -e ./surround_view
sudo LD_LIBRARY_PATH=./:../../lib taskset -c 6 schedtool -F -p 52 -e ./cuTestClient



########## tensorflow example
source dnn_env/bin/activate
python3.5 detector.py
deactivate



########## Overhead Test on GTX 1070

## Opengl Tutorial17 - No CARSS
./cuTestClient
-> We run the graphics task without real-time prioirity to avoid blocking rendering
## Opengl Tutorial17 - CARSS - Tag-only
sudo taskset -c 1 schedtool -F -p 60 -e ./mid --policy fifo
sudo LD_LIBRARY_PATH=./:../../lib taskset -c 6 schedtool -F -p 52 -e ./cuTestClient
## Opengl Tutorial17 - CARSS - lsf
sudo taskset -c 1 schedtool -F -p 60 -e ./mid --policy lsf
sudo LD_LIBRARY_PATH=./:../../lib taskset -c 6 schedtool -F -p 52 -e ./cuTestClient
-> FrameController fc(frame_name, 60, false);
-> It is NOT ideal method to measure the overhead...


## Opengl 3D surround view - No CARSS
./surround_view
-> We run the graphics task without real-time prioirity to avoid blocking rendering
## Opengl 3D surround view - CARSS - Tag-only
./mid --policy fifo
./surround_view
-> We run the graphics task without real-time prioirity to avoid blocking rendering
## Opengl 3D surround view - CARSS - lsf
./mid --policy lsf
./surround_view
-> set FrameController fc(frame_name, 30, false);
-> We run the graphics task without real-time prioirity to avoid blocking rendering


## PyTorch SSD MobileNet - No CARSS
sudo -E LD_LIBRARY_PATH=../../lib taskset -c 2 schedtool -F -p 52 -e python3.6 detect_ssd.py
## PyTorch SSD MobileNet - CARSS - Tag-only
sudo taskset -c 1 schedtool -F -p 60 -e ./mid --policy fifo
sudo -E LD_LIBRARY_PATH=../../lib taskset -c 2 schedtool -F -p 58 -e python3.6 detect_ssd.py --fps 60
-> depth 0 - frame level tagging
## PyTorch SSD MobileNet - CARSS - lsf
sudo taskset -c 1 schedtool -F -p 60 -e ./mid --policy lsf
sudo -E LD_LIBRARY_PATH=../../lib taskset -c 2 schedtool -F -p 58 -e python3.6 detect_ssd.py --fps 60
-> depth 0 - frame level tagging


## OpenCV Hog - NO CARSS
sudo LD_LIBRARY_PATH=../../lib taskset -c 3 schedtool -F -p 52 -e ./hog
## OpenCV Hog - CARSS - Tag-Only
sudo taskset -c 1 schedtool -F -p 60 -e ./mid --policy fifo
sudo LD_LIBRARY_PATH=../../lib taskset -c 3 schedtool -F -p 52 -e ./hog
## OpenCV Hog - CARSS - Tag-Only
sudo taskset -c 1 schedtool -F -p 60 -e ./mid --policy lsf
sudo LD_LIBRARY_PATH=../../lib taskset -c 3 schedtool -F -p 52 -e ./hog 30




########## Overhead Test on Xavier

## Opengl Tutorial17 - No CARSS
./cuTestClient
## Opengl Tutorial17 - CARSS - Tag-only
./mid --policy fifo
./cuTestClient
## Opengl Tutorial17 - CARSS - lsf
./mid --policy lsf
./cuTestClient
-> FrameController fc(frame_name, 30, false);
-> It is NOT ideal method to measure the overhead...


## Opengl 3D surround view - No CARSS
./surround_view
-> We run the graphics task without real-time prioirity to avoid blocking rendering
## Opengl 3D surround view - CARSS - Tag-only
./mid --policy fifo
./surround_view
-> We run the graphics task without real-time prioirity to avoid blocking rendering
## Opengl 3D surround view - CARSS - lsf
./mid --policy lsf
./surround_view
-> set FrameController fc(frame_name, 30, false);
-> We run the graphics task without real-time prioirity to avoid blocking rendering


## PyTorch SSD MobileNet - No CARSS
sudo -E LD_LIBRARY_PATH=../../lib taskset -c 2 schedtool -F -p 52 -e python3.6 detect_ssd.py
## PyTorch SSD MobileNet - CARSS - Tag-Only
sudo taskset -c 1 schedtool -F -p 60 -e ./mid --policy fifo
sudo -E LD_LIBRARY_PATH=../../lib taskset -c 2 schedtool -F -p 58 -e python3.6 detect_ssd.py --fps 60
-> depth 0 - frame level tagging
## PyTorch SSD MobileNet - CARSS - lsf
sudo taskset -c 1 schedtool -F -p 60 -e ./mid --policy lsf
sudo -E LD_LIBRARY_PATH=../../lib taskset -c 2 schedtool -F -p 58 -e python3.6 detect_ssd.py --fps 60
-> depth 0 - frame level tagging



## OpenCV Hog - NO CARSS
./hog
-> it shows low performance with real-time priority
## OpenCV Hog - CARSS - Tag-Only
./mid --policy fifo
./hog
## OpenCV Hog - CARSS - Tag-Only
./mid --policy lsf
./hog 11

