#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <vector>
#include <iomanip>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#define PORT 8080

int main_png_c(int argc, char const *argv[])
{
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char rcv_msg[4];
    std::string msg_open = "OPEN";
    std::string msg_infer = "INFR";
    std::string msg_close = "CLSE";
    std::string msg_down = "DOWN";
    std::string msg_fail = "FAIL";
    int keypoints[110] = {0};

    //cv::Mat img = cv::imread("./ryan.bmp");
    //cv::namedWindow( "Display window", cv::WINDOW_AUTOSIZE );
    //cv::imshow( "Display window", img );
    //cv::waitKey(0);
    //return 0;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    // Send OPEN message
    send(sock , msg_open.c_str() , msg_open.size() , 0 );
    std::cout << "OPEN" << std::endl;
    valread = recv(sock, rcv_msg, sizeof(rcv_msg), 0);
    if (!strncmp(rcv_msg, msg_fail.c_str(), 4)) {
        std::cout << "ERROR" << std::endl;
        return 0;
    }
    
    // Send INFR message
    for(int times=1 ;times<=2; times++) {
        send(sock , msg_infer.c_str() , msg_infer.size() , 0 );
        cv::Mat img = cv::imread("./ryan.bmp");
        std::vector<uchar> img_enc;
        imencode(".png", img, img_enc);
        int valsend = send(sock , img_enc.data() , img_enc.size() , 0 );
        std::cout << img_enc.size() << "," << valsend << std::endl;
        valread = recv(sock, rcv_msg, sizeof(rcv_msg), 0);
        if (!strncmp(rcv_msg, msg_fail.c_str(), 4)) {
            std::cout << "ERROR" << std::endl;
            return 0;
        }
        std::cout << "---------------------------------INFER(" << times << ")" << std::endl;
        valread = read( sock , keypoints, sizeof(keypoints));
        if(valread<=0) {
            std::cout << "ERROR : " << valread << std::endl;
            exit(0);
        }
        //for(int i=0; i<110; i++)
        for(int i=0; i<51; i++)
            std::cout<< i << ":" << keypoints[i] << std::endl;
    }
    
    // Send CLSE message
    send(sock , msg_close.c_str() , msg_close.size() , 0 );
    std::cout << "CLOSE" << std::endl;
    valread = recv(sock, rcv_msg, sizeof(rcv_msg), 0);

    //std::cout << __LINE__ << std::endl;
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#define LOCALHOST "127.0.0.1"
//#define PORT 7200
#define FRAME_WIDTH         1280 //640
#define FRAME_HEIGHT        720 //480

using namespace std;
using namespace cv;

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main()
{
  int sockfd, portno, n, imgSize, IM_HEIGHT, IM_WIDTH;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  char buffer[256];
  Mat cameraFeed;

  portno = PORT;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0) error("ERROR opening socket");

  server = gethostbyname(LOCALHOST);

  if (server == NULL) {
      fprintf(stderr,"ERROR, no such host\n");
      exit(0);
  }

  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr,
       (char *)&serv_addr.sin_addr.s_addr,
       server->h_length);
  serv_addr.sin_port = htons(portno);

  if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
      error("ERROR connecting");

  VideoCapture capture;

	capture.open(0);
    capture.set(cv::CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
    capture.set(cv::CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);
    cout << capture.get(cv::CAP_PROP_FRAME_WIDTH) << endl;
    cout << capture.get(cv::CAP_PROP_FRAME_HEIGHT) << endl;

  while(true)
  {
    /* store image to matrix && test frame */
    if(!capture.read(cameraFeed))
    {
        cout << "Video Ended" << endl;
        break;
    }

    int height = cameraFeed.rows;
    int width = cameraFeed.cols;

    Mat cropped = Mat(cameraFeed, Rect(width/2 - width/7,
                                       height/2 - height/9,
                                       2*width/7, 2*height/7));
    cameraFeed = cropped;

    IM_HEIGHT = FRAME_HEIGHT;
    IM_WIDTH = FRAME_WIDTH;

    resize(cameraFeed, cameraFeed, Size( IM_WIDTH , IM_HEIGHT ));

    imgSize=cameraFeed.total()*cameraFeed.elemSize();

    n = send(sockfd, cameraFeed.data, imgSize, 0);
    if (n < 0) error("ERROR writing to socket");
  }

  close(sockfd);

  return 0;
}
