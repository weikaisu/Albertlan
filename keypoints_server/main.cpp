#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <iostream>
#include <string.h>
#include <iomanip>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp> 
#define PORT 8080

int main_png_s(int argc, char const *argv[])
{
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char rcv_msg[4];
    std::string msg_open = "OPEN";
    std::string msg_infer = "INFR";
    std::string msg_close = "CLSE";
    std::string msg_down = "DOWN";
    std::string msg_fail = "FAIL";

    double keypoints[110];
    keypoints[0] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    keypoints[1] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    keypoints[2] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    //std::cout<< std::setprecision (17) << keypoints[0] << std::endl;
    //std::cout<< std::setprecision (17) << keypoints[1] << std::endl;
    //std::cout<< std::setprecision (17) << keypoints[2] << std::endl;

    unsigned char img_enc[10*1024];
    std::vector<unsigned char> img_bits;

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    while ((valread = recv(new_socket, rcv_msg, sizeof(rcv_msg), 0)) >0 ) {
        if (!strncmp(rcv_msg, msg_open.c_str(), 4)) {
            std::cout << "OPEN" << std::endl;
            send(new_socket , msg_down.c_str() , msg_down.size() , 0 );
        } else if (!strncmp(rcv_msg, msg_infer.c_str(), 4)) {
            std::cout << "INFER" << std::endl;
            //int total=0;
            //while((valread = recv(new_socket, img_enc, sizeof(img_enc), 0)) >0) {
            valread=1;
            auto tInferenceBegins = cv::getTickCount();
            while(valread>0) {
                valread = recv(new_socket, img_enc, sizeof(img_enc), 0);
                //total+=valread;
                //std::cout << valread << "," << total << std::endl;
                img_bits.insert(img_bits.end(), img_enc, img_enc+valread);
                if(valread<10*1024) break;
            }
            auto tInferenceEnds = cv::getTickCount();
            cv::Mat img = cv::imdecode(img_bits, cv::IMREAD_COLOR);
            std::cout << (tInferenceEnds - tInferenceBegins) * 1000. / cv::getTickFrequency() << std::endl;
            cv::namedWindow( "Display window", cv::WINDOW_AUTOSIZE );
            cv::imshow( "Display window", img );
            //char key = cv::waitKey(30);
            send(new_socket , msg_down.c_str() , msg_down.size() , 0 );
            send(new_socket , &keypoints , sizeof(keypoints) , 0 );
        } else if (!strncmp(rcv_msg, msg_close.c_str(), 4)) {
            std::cout << "CLOSE" << std::endl;
            send(new_socket , msg_down.c_str() , msg_down.size() , 0 );
            break;
        } else {
            std::cout << "UNKNOWN_MSG" << std::endl;
            break;
        }
    }

    //std::cout << __LINE__ << std::endl;
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include<sys/socket.h>	//socket
#include<sys/types.h>
#include<netinet/in.h>

//using namespace std;
//using namespace cv;

//#define PORT 7200

#define FRAME_WIDTH         1280 //640
#define FRAME_HEIGHT        720 //480

void error(const char *msg)
{
  perror(msg);
  exit(1);
}

int main()
{
  int sockfd, newsockfd, portno, imgSize, bytes=0; //, IM_HEIGHT, IM_WIDTH;;
  socklen_t clilen;
  //char buffer[256];
  struct sockaddr_in serv_addr, cli_addr;
  cv::Mat img;

  sockfd=socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd<0) error("ERROR opening socket");

  bzero((char*)&serv_addr, sizeof(serv_addr));
  portno = PORT;

  serv_addr.sin_family=AF_INET;
  serv_addr.sin_addr.s_addr=INADDR_ANY;
  serv_addr.sin_port=htons(portno);

  if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))<0) 
    error("ERROR on binding");

  listen(sockfd,5);
  clilen=sizeof(cli_addr);

  newsockfd=accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
  if(newsockfd<0) error("ERROR on accept");

  uchar sock[3];
  std::cout << sock << std::endl;
  std::cout << sock+3 << std::endl;

  // bzero(buffer,1024);
  // n = read(newsockfd, buffer, 1023);
  // if(n<0) error("ERROR reading from socket");
  //printf("Here is the message: %s\n", buffer);

  // n=write(newsockfd, "I got your message", 18);
  // if(n<0) error("ERROR writing to socket");
  bool running = true;

  while(running)
  {
    //IM_HEIGHT = FRAME_HEIGHT;
    //IM_WIDTH = FRAME_WIDTH;
    img = cv::Mat::zeros(FRAME_HEIGHT, FRAME_WIDTH, CV_8UC3);

    imgSize = img.total()*img.elemSize(); std::cout << imgSize << std::endl;
    uchar sockData[imgSize];

    auto tInferenceBegins = cv::getTickCount();
    for(int i=0;i<imgSize;i+=bytes)
      if ((bytes=recv(newsockfd, sockData+i, imgSize-i,0))==-1) error("recv failed");

    int ptr=0;

    for(int i=0;i<img.rows;++i) {
      for(int j=0;j<img.cols;++j)
      {
        img.at<cv::Vec3b>(i,j) = cv::Vec3b(sockData[ptr+0],sockData[ptr+1],sockData[ptr+2]);
        ptr=ptr+3;
      }
    }
    auto tInferenceEnds = cv::getTickCount();
    std::cout << (tInferenceEnds - tInferenceBegins) * 1000. / cv::getTickFrequency() << std::endl;

    namedWindow( "Server", cv::WINDOW_AUTOSIZE );// Create a window for display.
    imshow( "Server", img );
    char key = cv::waitKey(30);
    running = key;
    //esc
    if(key==27) running =false;
  }

  close(newsockfd);
  close(sockfd);

  return 0;
}
