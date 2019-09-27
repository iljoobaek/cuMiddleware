""" The file defines object detector. """
import functools
import time
import sys
import os
dirname = os.path.dirname(__file__)
slim_path = os.path.join(dirname, './slim/')
sys.path.append(slim_path)

import cv2
import tensorflow as tf
import numpy as np
import argparse

from google.protobuf import text_format
from object_detection.builders import model_builder
from object_detection.protos import pipeline_pb2
from object_detection.utils import label_map_util
from utils import np_methods
from utils import visualization
from utils import hot_key

DEFAULT_NETWORK_INPUT_SIZE=300

class ObjectDetector(object):
    """ A class of DNN object detector. """
    def __init__(self, checkpoint_path, pipeline_config_path, label_map_path,
                 network_size=None, gpu_memusage=-1):
        """
        Args:
            checkpoint_path: a string containing the path to
                the checkpoint files.
            pipeline_config_path: a string containing the path to
                the configuration file.
            label_path: a string containing path to the label map.
            network_size: the size of the network input,
                1D int32 [height, width]. The minimum input size is
                DEFAULT_NETWORK_INPUT_SIZE. If set to None, the network_size
                will auto adjust to the input image size.
            gpu_memusage: the percentage of the gpu memory to use, valid value
                should be (0,1]. It is set dynamically if set a negative value.
        """
        self._checkpoint_path = checkpoint_path
        self._pipeline_config_path = pipeline_config_path
        self._label_map_path = label_map_path
        self._network_size = network_size
        # =====================================================================
        # Configure model
        # =====================================================================
        model_config = self._get_configs_from_pipeline_file(
            self._pipeline_config_path)
        model_fn = functools.partial(
              model_builder.build,
              model_config=model_config,
              is_training=False)
        # instantiate model architecture
        model = model_fn()
        # replace the default non-max function with a dummy
        model._non_max_suppression_fn = self._dummy_nms
        # get the labels
        label_map = label_map_util.load_labelmap(self._label_map_path)
        max_num_classes = max([item.id for item in label_map.item])
        categories = label_map_util.convert_label_map_to_categories(
            label_map, max_num_classes=max_num_classes, use_display_name=True)
        self._category_index = label_map_util.create_category_index(categories)
        # =====================================================================
        # Construct inference graph
        # =====================================================================
        self._image = tf.placeholder(dtype=tf.float32, shape=(None,None,3))
        # opencv image is BGR by default, reverse its channels
        image_RGB = tf.reverse(self._image, axis=[-1])
        image_4D = tf.stack([image_RGB], axis=0)
        # resize image to fit network size
        if self._network_size:
            input_size = tf.cast(self._network_size, tf.float32)
            input_height = input_size[0]
            input_width = input_size[1]
        else:
            # Auto adjust network size for the image
            image_shape = tf.cast(tf.shape(self._image), tf.float32)
            input_height = image_shape[0]
            input_width =  image_shape[1]
        min_input_size = tf.minimum(input_width, input_height)
        min_input_size = tf.cast(min_input_size, tf.float32)
        scale = DEFAULT_NETWORK_INPUT_SIZE / min_input_size
        resized_image_height = tf.cond(scale > 1.0,
            lambda: scale*input_height, lambda: input_height)
        resized_image_width = tf.cond(scale > 1.0,
            lambda: scale*input_width, lambda: input_width)
        self._resized_image_size = tf.stack([resized_image_height,
                                             resized_image_width],
                                             axis=0)
        self._resized_image_size = tf.cast(self._resized_image_size,
                                           tf.int32)
        # replace resizer and anchor generator for ROI
        model._image_resizer_fn = functools.partial(self._resizer_fn,
            new_image_size=self._resized_image_size)

        model._anchor_generator._base_anchor_size = [tf.minimum(scale, 1.0),
                                                     tf.minimum(scale, 1.0)]
        preprocessed_image, true_image_shapes = model.preprocess(
            tf.to_float(image_4D))
        prediction_dict = model.predict(preprocessed_image, true_image_shapes)
        self._detection_dict = model.postprocess(prediction_dict, true_image_shapes)
        # =====================================================================
        # Fire up session
        # =====================================================================
        saver = tf.train.Saver()
        if gpu_memusage < 0:
            gpu_options = tf.GPUOptions(allow_growth=True)
        else:
            gpu_options = tf.GPUOptions(
                per_process_gpu_memory_fraction=gpu_memusage)
        self._sess = tf.Session(config=tf.ConfigProto(gpu_options=gpu_options))
        saver.restore(self._sess, self._checkpoint_path)

        # Memory stats ops
        self.mem_ops = tf.contrib.memory_stats.BytesInUse()
        self.max_mem_ops = tf.contrib.memory_stats.MaxBytesInUse()

    def __del__(self):
        """ Destructor"""
        self._sess.close()

    def detect(self, image, profile_flag=False):
        """ Detect objects in image.
        Args:
            image: a uint8 [height, width, depth] array representing the image.
            profile_flag: a boolean, determines whether to produce a run_meta object
        Returns:
            vis_img: a cv2 image containing the bounding boxes
            run_meta: a tf.RunMetadata() object with run profiling statistics, None if profile_flag=False
        """
        run_meta = None
        run_options = None
        if profile_flag:
            run_options = tf.RunOptions(trace_level=tf.RunOptions.FULL_TRACE)
            run_meta = tf.RunMetadata()

        rbboxes, rscores, rclasses, mem_ops, max_mem_ops = self._sess.run(
            [self._detection_dict['detection_boxes'],
             self._detection_dict['detection_scores'],
             self._detection_dict['detection_classes'],
             self.mem_ops, self.max_mem_ops],
            feed_dict={self._image:image},
            options=run_options,
            run_metadata=run_meta)
        # numpy NMS method
        rbboxes, rscores, rclasses = self._np_nms(rbboxes,rscores)
        #visualization
        vis_img = visualization.plt_bboxes(
            image,
            rclasses+1,
            rscores,
            rbboxes,
            self._category_index,
            Threshold=None
        )
        return vis_img, run_meta

    def _dummy_nms(self, detection_boxes, detection_scores, additional_fields=None,
                   clip_window=None):
        """ A dummy function to replace the tensorflow non_max_suppression_fn.
        We use a numpy version for a more flexiable post_processing. """
        return (detection_boxes, detection_scores, detection_scores,
                detection_scores, additional_fields, detection_scores)

    def _resizer_fn(self, image, new_image_size):
        """ Image resizer. """
        with tf.device('/gpu:0'):
            image = tf.image.resize_images(image, new_image_size)
        return [image, tf.concat([new_image_size, tf.constant([3])], axis=0)]

    def _np_nms(
        self,
        detection_boxes,
        detection_scores,
        clip_window=(0,0,1.0,1.0),
        # select_threshold=(
        #                     0.7,    #Car
        #                     0.8,    #Van
        #                     0.5,    #Truck
        #                     0.5,    #Cyclist
        #                     0.6,    #Pedestrian
        #                     0.5,    #Person_sitting
        #                     0.5,    #Tram
        #                     0.5,    #Misc
        #                     0.5,    #Sign
        #                     0.5,    #Traffic_cone
        #                     0.5,    #Traffic_light
        #                     0.5,    #School_bus
        #                     0.75,   #Bus
        #                     0.5,    #Barrier
        #                     0.5,    #Bike
        #                     0.5,    #Moter
        #                     ),
        select_threshold=(0.5,)*16,
        nms_threshold=0.1):
        # Get classes and bboxes from the net outputs.
        rclasses, rscores, rbboxes = np_methods.ssd_bboxes_select_layer(
                detection_scores,
                detection_boxes,
                anchors_layer=None,
                select_threshold=select_threshold,
                img_shape=None,
                num_classes=None,
                decode=False,
                label_offset=0)
        rbboxes = np_methods.bboxes_clip(clip_window, rbboxes)
        rclasses, rscores, rbboxes = np_methods.bboxes_sort(
            rclasses, rscores, rbboxes, top_k=400)
        rclasses, rscores, rbboxes = np_methods.bboxes_nms(
            rclasses, rscores, rbboxes, nms_threshold=nms_threshold)
        # Resize bboxes to original image shape. Note: useless for Resize.WARP!
        rbboxes = np_methods.bboxes_resize(clip_window, rbboxes)
        return rbboxes, rscores, rclasses

    def _get_configs_from_pipeline_file(self, pipeline_config_path):
        """Reads model configuration from a pipeline_pb2.TrainEvalPipelineConfig.
        Reads model config from file specified by pipeline_config_path.
        Returns:
        model_config: a model_pb2.DetectionModel
        """
        pipeline_config = pipeline_pb2.TrainEvalPipelineConfig()
        with tf.gfile.GFile(pipeline_config_path, 'r') as f:
            text_format.Merge(f.read(), pipeline_config)
            model_config = pipeline_config.model
        return model_config

if __name__ == '__main__':
    # If you want to test the code with your images, just add path to the images to the TEST_IMAGE_PATHS.
    PATH_TO_TEST_IMAGES_DIR = '/home/droid/Downloads/kitti_data'
    TEST_IMAGE_PATHS = [ os.path.join(PATH_TO_TEST_IMAGES_DIR, '{:010d}.png'.format(i)) for i in range(0, 154) ]
    #PATH_TO_TEST_IMAGES_DIR = '/home/iljoo/cuMiddleware_work/cuMiddleware_decorator/benchmark/data/kitti_data'
    #TEST_IMAGE_PATHS = [ os.path.join(PATH_TO_TEST_IMAGES_DIR, '{:06d}.png'.format(i)) for i in range(0, 154) ]

    parse = argparse.ArgumentParser("Run an SSD with or without tagging")
    parse.add_argument("--fps", dest="fps", default=-1, help="Enable fps control with tagging at desired fps [default -1, disabled]")
    args = parse.parse_args()

    fps = args.fps
    tagging_enabled = False
    if str(fps) == "-1":
        print("Tagging is NOT enabled")
        time.sleep(1)
    else:
        fps = float(fps)
        if fps < 0.0:
            print("Please provide a positive FPS, quitting")
            sys.exit(0)
        else:
            print("Tagging enabled, desired FPS is " + str(fps))
            tagging_enabled = True

    #######
    # To enable frame-job tagging of SSD Tensorflow's Session.run() function
    if tagging_enabled:
        sys.path.append("/home/droid/mhwork/cuMiddleware_v1/SourceCode/cu_wrapper/python")
        #sys.path.append("/home/iljoo/cuMiddleware_work/cuMiddleware/python")
        import tag_layer_fps
        allow_frame_drop = False
        fc = tag_layer_fps.FrameController("Tensorflow 1.0 SSD", fps, allow_frame_drop)
        tag_layer_fps.tag_tf_session_run(fc, False)
        print("Tagging ENABLED")
    else:
        print("Tagging NOT enabled")
    ######

    checkpoint_path = os.path.join(dirname,
        './train_kitti_mot_lisa_bdd_distort_color_focal_loss_300k/model.ckpt-530316')
    pipeline_config_path = os.path.join(dirname,
        './train_kitti_mot_lisa_bdd_distort_color_focal_loss_300k/ssd_mobilenet_v1_kitti_mot_lisa_bdd_distort_color.config')
    label_map_path = os.path.join(dirname,
        './train_kitti_mot_lisa_bdd_distort_color_focal_loss_300k/kitti_mot_bdd100k_lisaExtended2Coco_(train).tfrecord.pbtxt')
    detector = ObjectDetector(checkpoint_path, pipeline_config_path, label_map_path,
        network_size=None)

    drn_l = []
    start = None
    end = None
    frm_cnt = 0
    for imgpath in TEST_IMAGE_PATHS:
        if tagging_enabled:
            fc.frame_start()
        else:
            start = time.perf_counter()

        img = cv2.imread(imgpath, cv2.IMREAD_COLOR)
        #crop ROI
        crop_height = 300
        crop_width = 1280
        crop_ymin = 80
        crop_xmin = 0
        crop_ymax = crop_ymin+crop_height
        crop_xmax = crop_xmin+crop_width

        img_crop = img[crop_ymin:crop_ymax,
                       crop_xmin:crop_xmax]

        # draw ROI
        cv2.rectangle(img, (crop_xmin, crop_ymin), (crop_xmax, crop_ymax),
            (0,0,255), 2)
        cv2.putText(img, '{:s}'.format('ROI normal (300)'),
            (crop_xmin, crop_ymin+30), cv2.FONT_HERSHEY_SIMPLEX, 1, (0,0,255),2)

        vis_img, run_meta = detector.detect(img_crop, False)

        if tagging_enabled:
            fc.frame_end()
        else:
            # Save frame duration
            frm_cnt+=1
            if frm_cnt > 15:  # ignore first a few frame result
                end = time.perf_counter()
                #print("Duration of frame: %12.10f" % (end-start))
                drn = (end - start)
                drn_l.append(drn)
        cv2.imshow("TF demo output", vis_img)
        cv2.waitKey(1)  # Non-blocking form

    # Print summary of execution stats
    if tagging_enabled:
        fc.print_exec_stats()
    else:
        print("Demo execution stats over %d frames:" % len(drn_l))
        print("Min: %s [s], Avg: %s [s]" % (min(drn_l), sum(drn_l)/float(len(drn_l))))
        worst_ms = 1000*max(drn_l)
        avg_ms = 1000*(sum(drn_l)/float(len(drn_l)))
        print('-'*60+'\nAvg FPS: {:.2f}'.format(1000/avg_ms))
        print('Worst FPS: {:.2f}'.format(1000/worst_ms))
