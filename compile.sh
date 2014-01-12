#!/bin/bash
gcc MJPEGServer.cpp -o MJPEGServer -I/usr/local/include/opencv -I /usr/local/include/ -lm -lopencv_core -lopencv_highgui -lopencv_video -lopencv_imgproc -ljpeg
