#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#define PORT 8080

int main(int argc, char const *argv[])
{
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char rcv_msg[4];
    std::string msg_open = "OPEN";
    std::string msg_infer = "INFR";
    std::string msg_close = "CLSE";
    std::string msg_down = "DOWN";
    std::string msg_fail = "FAIL";
    //double keypoints[110] = {0.0};
    int keypoints[110] = {0};
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