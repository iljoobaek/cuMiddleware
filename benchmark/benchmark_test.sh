

0.023 * 1000 = 23

1000 /23 = 



##### LSF
sudo taskset -c 1 schedtool -F -p 60 -e ./mid --policy lsf

sudo -E LD_LIBRARY_PATH=../../lib taskset -c 2 schedtool -F -p 58 -e python3.6 detect_ssd.py --fps 50

sudo LD_LIBRARY_PATH=../../lib taskset -c 3 schedtool -F -p 56 -e ./hog 30

sudo LD_LIBRARY_PATH=../../lib taskset -c 4 schedtool -F -p 52 -e ./surround_view

sudo LD_LIBRARY_PATH=./:../../lib taskset -c 6 schedtool -F -p 52 -e ./cuTestClient


##### Tag-Only
sudo taskset -c 1 schedtool -F -p 60 -e ./mid --policy lsf

sudo -E LD_LIBRARY_PATH=../../lib taskset -c 2 schedtool -F -p 58 -e python3.6 detect_ssd.py --fps 50

sudo LD_LIBRARY_PATH=../../lib taskset -c 3 schedtool -F -p 56 -e ./hog

sudo LD_LIBRARY_PATH=../../lib taskset -c 4 schedtool -F -p 52 -e ./surround_view

sudo LD_LIBRARY_PATH=./:../../lib taskset -c 6 schedtool -F -p 52 -e ./cuTestClient



export LD_LIBRARY_PATH=/home/iljoo/cuMiddleware_work/cuMiddleware/lib:$LD_LIBRARY_PATH
