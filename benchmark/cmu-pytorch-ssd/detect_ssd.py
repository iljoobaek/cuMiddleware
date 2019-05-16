from vision.ssd.vgg_ssd import create_vgg_ssd, create_vgg_ssd_predictor
from vision.ssd.mobilenetv1_ssd import create_mobilenetv1_ssd, create_mobilenetv1_ssd_predictor
from vision.ssd.mobilenetv1_ssd_lite import create_mobilenetv1_ssd_lite, create_mobilenetv1_ssd_lite_predictor
from vision.ssd.squeezenet_ssd_lite import create_squeezenet_ssd_lite, create_squeezenet_ssd_lite_predictor
from vision.ssd.mobilenet_v2_ssd_lite import create_mobilenetv2_ssd_lite, create_mobilenetv2_ssd_lite_predictor
from vision.utils.misc import Timer
import torch
import cv2
import sys
import os
import flops_counter

from vision.nn_profile import Profiler

#######
# To enable pytorch tagging of SSD Pytorch module
sys.path.append("/home/droid/mhwork/cuMiddleware_v1/SourceCode/cu_wrapper/python")
import tag_layer
import vision
fns_to_tag = ["conv2d", "linear", "relu", "avg_pool2d", "batch_norm"]
tag_layer.tag_pytorch_nn_functions_list(fns_to_tag, enable=True)
######

# If you want to test the code with your images, just add path to the images to the TEST_IMAGE_PATHS.
PATH_TO_TEST_IMAGES_DIR = '/home/droid/Downloads/kitti_data'
TEST_IMAGE_PATHS = [ os.path.join(PATH_TO_TEST_IMAGES_DIR, '{:010d}.png'.format(i)) for i in range(0, 154) ]

if len(sys.argv) < 4:
    print('Usage: python run_ssd_example.py <net type>  <model path> <label path>')
    sys.exit(0)
net_type = sys.argv[1]
model_path = sys.argv[2]
label_path = sys.argv[3]

class_names = [name.strip() for name in open(label_path).readlines()]
num_classes = len(class_names)


if net_type == 'vgg16-ssd':
    net = create_vgg_ssd(len(class_names), is_test=True)
elif net_type == 'mb1-ssd':
    net = create_mobilenetv1_ssd(len(class_names), is_test=True)
elif net_type == 'mb1-ssd-lite':
    net = create_mobilenetv1_ssd_lite(len(class_names), is_test=True)
elif net_type == 'mb2-ssd-lite':
    net = create_mobilenetv2_ssd_lite(len(class_names), is_test=True)
elif net_type == 'sq-ssd-lite':
    net = create_squeezenet_ssd_lite(len(class_names), is_test=True)
else:
    print("The net type is wrong. It should be one of vgg16-ssd, mb1-ssd and mb1-ssd-lite.")
    sys.exit(1)
net.load(model_path)

if net_type == 'vgg16-ssd':
    predictor = create_vgg_ssd_predictor(net, candidate_size=200)
elif net_type == 'mb1-ssd':
    predictor = create_mobilenetv1_ssd_predictor(net, candidate_size=200)
elif net_type == 'mb1-ssd-lite':
    predictor = create_mobilenetv1_ssd_lite_predictor(net, candidate_size=200)
elif net_type == 'mb2-ssd-lite':
    predictor = create_mobilenetv2_ssd_lite_predictor(net, candidate_size=200)
elif net_type == 'sq-ssd-lite':
    predictor = create_squeezenet_ssd_lite_predictor(net, candidate_size=200)
else:
    print("The net type is wrong. It should be one of vgg16-ssd, mb1-ssd and mb1-ssd-lite.")
    sys.exit(1)

# Get flops count for this network:
input_res = (1242, 375)
#import pdb; pdb.set_trace()
flops, params = flops_counter.get_model_complexity_info(predictor.net.base_net.cuda(), input_res, as_strings=True,
                                          print_per_layer_stat=False)

# Set up layer-level profiling
enable_prof = False
prof = Profiler(predictor.net.base_net, enabled=enable_prof)

timer = Timer()
tot_fps = 0.
tenth_flag = 10
autograd_prof_flag = False

for image_path in TEST_IMAGE_PATHS:
    tenth_flag -= 1 
    orig_image = cv2.imread(image_path, cv2.IMREAD_COLOR)
    image = orig_image.copy()
    timer.start()
    if tenth_flag == 0 and enable_prof:
        if autograd_prof_flag:
            with torch.autograd.profiler.profile(use_cuda=True) as prof:
                predictor.predict(image, 10, 0.4, ret_fps=True)
        else:
            prof.set_enable(True)
            predictor.predict(image, 10, 0.4, ret_fps=True)
            prof.set_enable(False)
            print("Profiling results:")
            #print(prof)

    boxes, labels, probs, fps = predictor.predict(image, 10, 0.4, ret_fps=True)
    # Keep track of average FPS
    tot_fps += fps
    interval = timer.end()
    print('Time: {:.2f}s, Detect Objects: {:d}.'.format(interval, labels.size(0)))
    for i in range(boxes.size(0)):
        box = boxes[i, :]
        label = f"{class_names[labels[i]]}: {probs[i]:.2f}"
        cv2.rectangle(orig_image, (box[0], box[1]), (box[2], box[3]), (255, 255, 0), 4)

        cv2.putText(orig_image, label,
                    (box[0]+20, box[1]+40),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    1,  # font scale
                    (255, 0, 255),
                    2)  # line type
   # cv2.imshow('SSD (PyTorch) annotated', orig_image)
   # if cv2.waitKey(1) & 0xFF == ord('q'):
   #     break
cv2.destroyAllWindows()
print('-'*60+'\nAverage FPS:', tot_fps / len(TEST_IMAGE_PATHS))
if tenth_flag < 0 and enable_prof:
    print("Profiling results:")
    print(prof)
