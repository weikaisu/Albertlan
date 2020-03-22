#include <iostream>

/*
int main(int argc, char *argv[])
{
    std::cout << "Hello" << std::endl;

    return 0;

}
*/

#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string> 
#define PORT 8080 
int main_ori(int argc, char const *argv[]) 
{ 
    int server_fd, new_socket, valread; 
    struct sockaddr_in address; 
    int opt = 1; 
    int addrlen = sizeof(address); 
    //char buffer[1024] = {0};
    char msg[4];
    std::string hello = "Hello from server"; 
    //std::vector<double> keypoints(5, 0.0);
    double keypoints[5];
    keypoints[0] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    keypoints[1] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    keypoints[2] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
       
    // Creating socket file descriptor 
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    { 
        perror("socket failed"); 
        exit(EXIT_FAILURE); 
    } 
       
    // Forcefully attaching socket to the port 8080 
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) { 
        perror("setsockopt"); 
        exit(EXIT_FAILURE); 
    } 
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons( PORT ); 
       
    // Forcefully attaching socket to the port 8080 
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0)
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    }

    if (listen(server_fd, 3) < 0) 
    { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    } 

    //while(true)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
            perror("accept"); 
            exit(EXIT_FAILURE); 
        }

        while ((valread = recv(new_socket, msg, sizeof(msg), 0)) >0 ) {
            std::cout << valread << std::endl;
            std::string msg_s(msg);

            if(msg_s == "OPEN") {
                std::cout << "OPEN" << std::endl;
            } else if (msg_s == "INFR") {
                std::cout << "INFER" << std::endl;
            } else if (msg_s == "CLSE") {
                std::cout << "CLOSE" << std::endl;
                break;
            }
        }
    }


    
    //valread = read( new_socket , buffer, 1024); 
    //valread = recv( new_socket , buffer, 1024, 0); 
    //printf("%s, %d\n",buffer, valread); 
    //send(new_socket , hello.c_str() , hello.size() , 0 ); 
    send(new_socket , &keypoints , sizeof(keypoints) , 0 ); 
    printf("%f\n",keypoints[0]);
    printf("%f\n",keypoints[1]);
    printf("%f\n",keypoints[2]);
    printf("Hello message sent\n"); 
    return 0; 
} 