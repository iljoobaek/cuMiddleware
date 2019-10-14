To run:

`$ make test_server;`

Compiling with tag_lib is not yet portable, so you have to have a Makefile target like:

```
test_tag.o: test_tag.c tag_lib.o common.o mid_queue.o
	$(GCC) $(MIDFLAGS) -o test_tag.o test_tag.c tag_lib.o common.o mid_queue.o $(MID_LOAD)
```


############ Benchmark Related ############

##### OpenGL 3D Surround View
sudo apt-get install libglfw3-dev
sudo apt-get install libglm-dev

sudo apt install libgl1-mesa-dev
sudo rm /usr/lib/x86_64-linux-gnu/libGL.so
sudo ln -s /usr/lib/x86_64-linux-gnu/mesa/libGL.so /usr/lib/x86_64-linux-gnu/libGL.so


##### OpenGL Tuturial 17
Install AntTweakBar_116
http://anttweakbar.sourceforge.net/doc/tools:anttweakbar:download
unzip  AntTweakBar_116.zip
goto src
make

############ Test on Xavier ############

##### Set GPU and CPU freq to max_time
sudo /usr/bin/jetson_clocks
sudo /usr/bin/jetson_clocks --show


##### Enable real time priority
sudo vi /sys/fs/cgroup/cpu/user.slice/cpu.rt_runtime_us
950000

##### Opencv for Xavier
sudo apt-get purge libopencv*

git clone https://github.com/jetsonhacks/buildOpenCVXavier.git
cd buildOpenCVXavier
git checkout v1.0

change in  buildOpenCV.sh

OPENCV_VERSION=3.3.1
-D WITH_QT=OFF

mkdir /home/rtml/opencv
./buildOpenCV.sh -s /home/rtml/opencv
=> This example will build OpenCV in the given file directory and install OpenCV in the /usr/local directory.

##### Install Pytorch
wget https://nvidia.box.com/shared/static/2ls48wc6h0kp1e58fjk21zast96lpt70.whl -O torch-1.0.0a0+bb15580-cp36-cp36m-linux_aarch64.whl
pip3 install numpy torch-1.0.0a0+bb15580-cp36-cp36m-linux_aarch64.whl
pip3 install torchvision

Version Check
$ python3.6
>> import torch
>> print(torch.__version__)
