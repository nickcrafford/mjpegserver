/*

MJPEGServer (copyright Nick Crafford 2012)

Simple HTTP/MJPEG server. Serves frames captured from the
passed device at the specified resolution over the passed port.
Once the server is started visit http://<host>:<port>/ to view the
MJPEG stream in your web browser of choice.

Usage:       MJPEGServer <port> <width> <height> <device_num>
Compilation: gcc MJPEGServer.cpp -o MJPEGServer -I/usr/local/include/opencv -I /usr/local/include/ -lm -lopencv_core -lopencv_highgui -lopencv_video -lopencv_imgproc -ljpeg

Dependencies:
  - OpenCV 2.x
  - libjpeg v8d (http://www.ijg.org/)

Inspiration via:
http://stackoverflow.com/questions/16980496/opencv-saving-image-captured-from-webcam
http://stackoverflow.com/questions/1443390/compressing-iplimage-to-jpeg-using-libjpeg-in-opencv

*/

#include <cv.h> 
#include <highgui.h> 
#include <stdio.h>  
#include <jpeglib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

const char *mjpg_header     = "HTTP/1.0 200 OK\r\nServer: Magnus\r\nConnection: close\r\nMax-Age: 0\r\nExpires: 0\r\nCache-Control: no-cache, private\r\nPragma: no-cache\r\nContent-Type: multipart/x-mixed-replace; boundary=--BoundaryString\r\n\r\n";
const char *mjpeg_boundary  = "--BoundaryString\r\nContent-type: image/jpg\r\nContent-Length: ";
char       *line_terminator = "\r\n\r\n";

// Convert an IplImage into a jpeg in memory
bool ipl2jpeg(IplImage *frame, unsigned char **outbuffer, long unsigned int *outlen) {
  unsigned char *outdata = (uchar *) frame->imageData;
  struct jpeg_compress_struct cinfo = {0};
  struct jpeg_error_mgr jerr;
  
  JSAMPROW row_ptr[1];
  int row_stride;

  *outbuffer = NULL;
  *outlen    = 0;

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);
  jpeg_mem_dest(&cinfo, outbuffer, outlen);

  cinfo.image_width      = frame->width;
  cinfo.image_height     = frame->height;
  cinfo.input_components = frame->nChannels;
  cinfo.in_color_space   = JCS_RGB;

  jpeg_set_defaults(&cinfo);
  jpeg_start_compress(&cinfo, TRUE);
  row_stride = frame->width * frame->nChannels;

  while (cinfo.next_scanline < cinfo.image_height) {
    row_ptr[0] = &outdata[cinfo.next_scanline * row_stride];
    jpeg_write_scanlines(&cinfo, row_ptr, 1);
  }

  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);

  return true;
}

int main(int argc, char *argv[]) {

  if(argc < 5) {
    printf("Usage: MJPEGServer <port> <width> <height> <device_num>\n");
    return -1;    
  }

  // Command line args
  int portno    = atoi(argv[1]);
  int width     = atoi(argv[2]);
  int height    = atoi(argv[3]);
  int deviceNum = atoi(argv[4]);
  
  CvCapture* capture = cvCaptureFromCAM(deviceNum);

  cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH,  width);
  cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, height);

  if(!capture) {
    fprintf(stderr, "ERROR: capture is NULL \n");
    getchar();
    return -1;
  }

  // Setup socket
  int       sockfd;
  int       newsockfd;  
  socklen_t clilen;
  char      buffer[256];
  struct sockaddr_in serv_addr;
  struct sockaddr_in cli_addr;
  int       n;
  
  sockfd = socket(AF_INET, SOCK_STREAM, 0);  

  if(sockfd < 0)  {
    printf("ERROR opening socket\n");
  }
  
  bzero((char *) &serv_addr, sizeof(serv_addr));
  
  serv_addr.sin_family      = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port        = htons(portno);
    
  if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    printf("ERROR binding port\n");
  }

  listen(sockfd,5);
  clilen = sizeof(cli_addr);

  while(1) {
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,  &clilen);

    if(newsockfd < 0) {
      printf("ERROR on accept\n");
    }

    // Write the header to the socket
    int n2 = 0;
    n2 = send(newsockfd, mjpg_header, strlen(mjpg_header), MSG_NOSIGNAL);
    if(n2 == -1) continue;

    // Write the each captured frame to the socket
    while(1) {

      // Capture a frame from the web cam
      IplImage* frame = cvQueryFrame(capture);

      if(!frame) {
        fprintf( stderr, "ERROR: frame is null...\n" );
        getchar();
        break;
      }

      // Convert frame to JPEG
      unsigned char     *outbuffer;
      long unsigned int  outlen;
      ipl2jpeg(frame, &outbuffer, &outlen);

      // Write the MJPEG initial boundary        
      n2 = send(newsockfd, mjpeg_boundary, strlen(mjpeg_boundary), MSG_NOSIGNAL);
      if(n2 == -1) break;

      // Write the length of the JPEG
      char mjpeg_length[20];
      sprintf(mjpeg_length, "%d", (int)outlen);
      n2 = send(newsockfd, mjpeg_length, strlen(mjpeg_length), MSG_NOSIGNAL);
      if(n2 == -1) break;


      // Write a line terminator
      n2 = send(newsockfd, line_terminator, strlen(line_terminator), MSG_NOSIGNAL);
      if(n2 == -1) break;
    

      // Write the JPEG itself    
      n2 = send(newsockfd,outbuffer,outlen, MSG_NOSIGNAL);
      if(n2 == -1) break;


      // Write a line terminator again
      n2 = send(newsockfd, line_terminator, strlen(line_terminator), MSG_NOSIGNAL);
      if(n2 == -1) break;

      free(outbuffer);
    }

    close(newsockfd);
  }

  close(sockfd);

  // Release the capture device housekeeping
  cvReleaseCapture(&capture);

  return 0;
}
