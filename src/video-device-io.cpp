#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <iostream>

#include <linux/videodev2.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

uchar* buffer;
 
//wrapper for iotcl to retry on system interrupt..
static int ioctl_internal(int fd, int request, void *arg)
{
        int r = 0;     
        int16_t retries = 10; //retry 10 times.
        bool retry = false;
        
        do 
        {
            //make driver request..
            r = ioctl (fd, request, arg);
            
            //retry on system interrupt..
            if ((r == -1) && (errno == EINTR))
            {
                retries--;
                retry = true;
                usleep(5000);//delay read with 5ms and try again..
                std::cerr << "ERROR: IOCTL Interrupt.." << std::endl;
            }
            else
            {
                retry = false;                        ;
            }
        }
        while (retry && retries > 0);
         
        return r;
}
 
int print_caps(int fd)
{
        struct v4l2_capability caps = {};
        if (-1 == ioctl_internal(fd, VIDIOC_QUERYCAP, &caps))
        {
                perror("Querying Capabilities");
                return 1;
        }
 
        printf( "Driver Caps:\n"
                "  Driver: \"%s\"\n"
                "  Card: \"%s\"\n"
                "  Bus: \"%s\"\n"
                "  Version: %d.%d\n"
                "  Capabilities: %08x\n",
                caps.driver,
                caps.card,
                caps.bus_info,
                (caps.version>>16)&&0xff,
                (caps.version>>24)&&0xff,
                caps.capabilities);
 
 
        struct v4l2_cropcap cropcap = {0};
        cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == ioctl_internal (fd, VIDIOC_CROPCAP, &cropcap))
        {
                perror("Querying Cropping Capabilities");
                return 1;
        }
 
        printf( "Camera Cropping:\n"
                "  Bounds: %dx%d+%d+%d\n"
                "  Default: %dx%d+%d+%d\n"
                "  Aspect: %d/%d\n",
                cropcap.bounds.width, cropcap.bounds.height, cropcap.bounds.left, cropcap.bounds.top,
                cropcap.defrect.width, cropcap.defrect.height, cropcap.defrect.left, cropcap.defrect.top,
                cropcap.pixelaspect.numerator, cropcap.pixelaspect.denominator);
 
        int support_grbg10 = 0;
 
        struct v4l2_fmtdesc fmtdesc = {0};
        fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        char fourcc[5] = {0};
        char c, e;
        printf("  FMT : CE Desc\n--------------------\n");
        while (0 == ioctl_internal(fd, VIDIOC_ENUM_FMT, &fmtdesc))
        {
                strncpy(fourcc, (char *)&fmtdesc.pixelformat, 4);
                if (fmtdesc.pixelformat == V4L2_PIX_FMT_SGRBG10)
                    support_grbg10 = 1;
                c = fmtdesc.flags & 1? 'C' : ' ';
                e = fmtdesc.flags & 2? 'E' : ' ';
                printf("  %s: %c%c %s\n", fourcc, c, e, fmtdesc.description);
                fmtdesc.index++;
        }
        /*
        if (!support_grbg10)
        {
            printf("Doesn't support GRBG10.\n");
            return 1;
        }*/
 
        struct v4l2_format fmt = {0};
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width = 1920;
        fmt.fmt.pix.height = 1080;
        //fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR24;
        //fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_GREY;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    //    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV422M;
        // fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV422P;
        // fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; //USB CAMERA!!
    //    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
//        fmt.fmt.pix.field = V4L2_FIELD_NONE;
        
        if (-1 == ioctl_internal(fd, VIDIOC_S_FMT, &fmt))
        {
            perror("Setting Pixel Format");
            return 1;
        }
 


        strncpy(fourcc, (char *)&fmt.fmt.pix.pixelformat, 4);
        printf( "Selected Camera Mode:\n"
                "  Width: %d\n"
                "  Height: %d\n"
                "  PixFmt: %s\n"
                "  Field: %d\n",
                fmt.fmt.pix.width,
                fmt.fmt.pix.height,
                fourcc,
                fmt.fmt.pix.field);
        return 0;
}
 
int init_mmap(int fd)
{
    //request  memory mapped buffer of video capture..
    struct v4l2_requestbuffers buff_req;
    memset(&buff_req, 0, sizeof(buff_req));
    
    buff_req.count = 1;
    buff_req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buff_req.memory = V4L2_MEMORY_MMAP;
 
    if (-1 == ioctl_internal(fd, VIDIOC_REQBUFS, &buff_req))
    {
        perror("IOCTL Request Buffer Failed.");
        return 1;
    }
 
    //query buffer info
    struct v4l2_buffer buffer_info;
    memset(&buffer_info, 0, sizeof(buffer_info));

    buffer_info.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer_info.memory = V4L2_MEMORY_MMAP;
    buffer_info.index = 0;
    if(-1 == ioctl_internal(fd, VIDIOC_QUERYBUF, &buffer_info))
    {
        perror("Querying Buffer");
        return 1;
    }
 
    //map the buffer..
    buffer = (uchar*) mmap (NULL, buffer_info.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buffer_info.m.offset);

    //turn stream on..
    if(-1 == ioctl_internal(fd, VIDIOC_STREAMON, &buffer_info.type))
    {
        perror("Start Capture");
        return 1;
    }
    
    printf("Length: %d\nAddress: %p\n", buffer_info.length, buffer);
    printf("Image Length: %d\n", buffer_info.bytesused);
 
    return 0;
}
 
int capture_image(int fd)
{
    struct v4l2_buffer buffer_info;
    memset(&buffer_info, 0, sizeof(buffer_info));
            
    buffer_info.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer_info.memory = V4L2_MEMORY_MMAP;
    buffer_info.index = 0;
    if(-1 == ioctl_internal(fd, VIDIOC_QBUF, &buffer_info))
    {
        perror("Queue Buffer");
        return 1;
    }

 
 
//    //use fd select to wait for frame to become ready..
//    fd_set fds;
//    FD_ZERO(&fds);
//    FD_SET(fd, &fds);
//    struct timeval tv = {0};
//    tv.tv_sec = 2;
//    //wait 2sec for frame to be ready.
//    int r = select(fd+1, &fds, NULL, NULL, &tv);
//    if(-1 == r)
//    {
//        perror("Waiting for Frame");
//        return 1;
//    }
// 
    //Dequeue frame - this is actually a blocking call.. so no need to "select" and wait..
    if(-1 == ioctl_internal(fd, VIDIOC_DQBUF, &buffer_info))
    {
        perror("Retrieving Frame");
        return 1;
    }


    
    //ignore first frame (always corrupt jpeg)
    if (buffer_info.sequence > 0)
    {
        cv::Mat cvmat;

        //convert buffer to opencv Mat.& decode JPEG
        //cv::Mat inmat(600, 800,  CV_8UC2, buffer);//, buffer_info.length);
        // cv::cvtColor (inmat, cvmat, cv::COLOR_YUV2BGR_YUYV);
        
        //decode
        std::vector<uchar> data = std::vector<uchar>(buffer, buffer + buffer_info.length);
        cvmat = cv::imdecode(data, cv::IMREAD_COLOR);

        cv::imshow ("foo", cvmat);    
        
        }

    return cv::waitKey(1);

}
 
int main()
{
    int fd;

    fd = open("/dev/video2", O_RDWR);
    if (fd == -1)
    {
            perror("Opening video device");
            return 1;
    }
    if(print_caps(fd))
        return 1;
    
    if(init_mmap(fd))
        return 1;
    int i;
    
    uint32_t frames = 0;
    float fps = 0;

    cv::namedWindow("foo", cv::WINDOW_AUTOSIZE);

    while (capture_image(fd) != 99)
    {

    }
    close(fd);
    return 0;
}