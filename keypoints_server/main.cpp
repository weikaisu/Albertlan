#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <iostream>
#include <string>
#include <iomanip>
#define PORT 8080

int main(int argc, char const *argv[])
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
    std::cout<< std::setprecision (17) << keypoints[0] << std::endl;
    std::cout<< std::setprecision (17) << keypoints[1] << std::endl;
    std::cout<< std::setprecision (17) << keypoints[2] << std::endl;

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