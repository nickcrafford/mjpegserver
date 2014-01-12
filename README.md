mjpegserver
===========

Simple MJPEG server for robotics projects etc.

#### Dependencies
* OpenCV 2.x: http://opencv.org/downloads.html
* libjpeg v8d: http://www.ijg.org/

##### Compiling
```
gcc MJPEGServer.cpp -o MJPEGServer -I <path_to_opencv> -I <path_to_ljpeg> -lm -lopencv_core -lopencv_highgui -lopencv_video -lopencv_imgproc -ljpeg
```

#### Usage
```
MJPEGServer <listen_port> <width> <height> <opencv_device_num>
```

#### Inspriation from
* http://stackoverflow.com/questions/16980496/opencv-saving-image-captured-from-webcam
* http://stackoverflow.com/questions/1443390/compressing-iplimage-to-jpeg-using-libjpeg-in-opencv