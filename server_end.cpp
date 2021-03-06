#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <iostream>
#include <time.h>
#include <iomanip>
#include <pthread.h>
#include <unistd.h>

using namespace std;

//===============================================
// write your code in here!
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include "pen.pb.h"
#define PORT 8845
#define BUFSIZE 100
char flag='2';
pthread_mutex_t mu;
//===============================================

void* server_send_data(void*);
void *server_recv_cmd(void*);

int main(){
    //socket init code
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    //===========================================
    //socket connect 
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("socket failed");
        exit(EXIT_FAILURE);
    }else{cout<<"Socket create success!\n";}

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT , &opt, sizeof(opt))){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }else{cout<<"set socker opt OK!\n";}

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
       
    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }else{cout<<"Socket binding success!\n";}

    if (listen(server_fd, 3) < 0){
        perror("listen");
        exit(EXIT_FAILURE);
    }else{cout<<"Socket listening...\n";}

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0){
        perror("accept");
        exit(EXIT_FAILURE);
    }else{cout<<"Socket accept!\n";}
    
    //===============================================
    //thread
    /*
     t1 is send 3-axis data to client
     t2 is recv cmd from client
     do recv cmd from client and send 3-axis data to client
    */
    pthread_t t1, t2;
    pthread_mutex_init(&mu, 0);
    pthread_create(&t2,NULL,server_recv_cmd, &new_socket);
    pthread_create(&t1,NULL,server_send_data, &new_socket);
    pthread_join(t1, 0);
    pthread_join(t2, 0);
    pthread_mutex_destroy(&mu);
    //================================================
    return 0;
}
/*
 get and send 3-axis data to client
*/
void* server_send_data(void* argv)
{
    //===============================================
    // 3-axis init code
    int new_socket = *(int *)argv;
    char reg[1] = {0x32};
    char data[6] ={0};
    uint16_t xAccl, yAccl, zAccl ;
    char buf[BUFSIZE];
    code::file s2, s1;
    char tmp_flag;
    int c=0;
    int file;
    char *bus = "/dev/i2c-1";
    if ((file = open(bus, O_RDWR)) < 0) {
	printf("Failed to open the bus.");
	exit(1);
    }
    ioctl(file, I2C_SLAVE, 0x53);
    char config[2]={0};
    config[0] = 0x2C;
    config[1] = 0x0F;
    write(file, config, 2);
    config[0] = 0x2D;
    config[1] = 0x08;
    write(file, config, 2);
    config[0] = 0x31;
    config[1] = 0x0B;
    write(file, config, 2);
    sleep(1);
    cout<<"===============ready to send=================\n";
    // ===========================================
    //get and send 3-axis data
    while(1){
	pthread_mutex_lock( &mu);
	tmp_flag=flag;
	pthread_mutex_unlock( &mu);
        if(tmp_flag == '1'){
	    //usleep(0.1*1000000); 
	    // 3-axis get XYZ data
	    write(file, reg, 1);
	    if(read(file, data, 6) != 6){
		std::cout << "Erorr : Input/output Erorr " << std::endl;
		exit(1);
	    }
	    else
	    {
		xAccl = ((data[1] & 31) * 256 + (data[0] & 0xFF));
		yAccl = ((data[3] & 0x1F) * 256 + (data[2] & 0xFF));
		zAccl = ((data[5] & 0x1F) * 256 + (data[4] & 0xFF));
	    }
	  /*
	    cout << "Acceleration in X-Axis : " << xAccl << std::endl;
	    cout << "Acceleration in Y-Axis : " << yAccl << std::endl;
	    cout << "Acceleration in Z-Axis : " << zAccl << std::endl;
	  */
	    s1.set_x(xAccl);
	    s1.set_y(yAccl);
	    s1.set_z(zAccl);
	    s1.set_freq(c);
	    s1.set_code("3-axis\n");
	    // 3-axis data pack
	    s1.SerializeToArray(buf, BUFSIZE);
	    send(new_socket, buf, BUFSIZE, 0);
	    cout << c << endl;;
	    c++;
	    // ===========================================
            //cout<<"Send data OK\n";
        }
        else if(tmp_flag == '2'){
            continue;
        }
        else
            break;
    }
    cout<<endl;
    pthread_exit((void *)0);
}
/*
  recv cmd from client
*/
void *server_recv_cmd(void* argv){
    cout<<"===================================\n";
    int sock = *(int *)argv;
    char tmp_flag;
    while(1){
	recv(sock, &tmp_flag, 1, MSG_WAITALL);
	cout<<"Recv cmd: "<<tmp_flag<<" OK!\n";
	pthread_mutex_lock( &mu);
	flag = tmp_flag;
	pthread_mutex_unlock( &mu);
        if(tmp_flag == '2'){
            continue;
	}
        else if(tmp_flag == '3'){
            break;
	}
    }
    cout<<"Socket close!\n";
    close(sock);
    pthread_exit((void *)0);
}

