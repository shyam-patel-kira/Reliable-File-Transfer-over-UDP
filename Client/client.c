/*
  Filename	client.c
  Authors	Shyam Patel 1741030
		Harshiv Patel 1741005
  Client implementation for reliable UDP Assignment.
  Date		October,2019
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include <time.h>

#define BUF_SIZE 10240	//Max buffer size of the data in a frame

/*A frame packet with unique id, length and data*/
struct frame_t {
	long int ID;
	long int length;
	char data[BUF_SIZE];
};
//----print_error------
static void print_error(char *msg){
	perror(msg);
	exit(EXIT_FAILURE);
}
/*----------------------Main function--------------------*/
int main(int argc, char **argv)
{
	if ((argc < 3) || (argc >3)) {			//exactly 3 arguments
		printf("Client: Enter a valid IP Address and Port number --> ./[%s] [IP Address] [Port Number]\n", argv[0]);  			//Should have the IP of the server
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in send_addr, from_addr;
	struct stat st;
	struct frame_t frame;
	struct timeval t_out = {0, 0};

	char cmd_send[50];
	char flname[20];
	char cmd[10];
	char ack_send[4] = "ACK";

	ssize_t numRead = 0;
	ssize_t length = 0;
	off_t f_size = 0;
	long int ack_num = 0;
	int cfd, ack_recv = 0;
	clock_t t;

	FILE *fptr;

	/*Clear all the data buffer and structure*/
	memset(ack_send, 0, sizeof(ack_send));
	memset(&send_addr, 0, sizeof(send_addr));
	memset(&from_addr, 0, sizeof(from_addr));

	/*Populate send_addr structure with IP address and Port*/
	send_addr.sin_family = AF_INET;
	send_addr.sin_port = htons(atoi(argv[2]));
	send_addr.sin_addr.s_addr = inet_addr(argv[1]);

	if ((cfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		print_error("CLient: socket");

	for (;;) {

		memset(cmd_send, 0, sizeof(cmd_send));
		memset(cmd, 0, sizeof(cmd));
		memset(flname, 0, sizeof(flname));

		printf("\n Menu \n Enter any of the following commands (commands not numbers!)\n 1.) get [file_name] \n 2.) ls \n 3.) exit \n");
		scanf(" %[^\n]%*c", cmd_send);
		sscanf(cmd_send, "%s %s", cmd, flname);		//parse the user input into command and filename
		if (sendto(cfd, cmd_send, sizeof(cmd_send), 0, (struct sockaddr *) &send_addr, sizeof(send_addr)) == -1) {
			print_error("Client: send");
		}
/*--------------------"get case"-------------------------------*/
		if ((strcmp(cmd, "get") == 0) && (flname[0] != '\0' )) {
			long int total_frame = 0;
			long int bytes_rec = 0, i = 0;
			t_out.tv_sec = 2;
			setsockopt(cfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&t_out, sizeof(struct timeval));
			//Enable the timeout option if client does not respond
			recvfrom(cfd, &(total_frame), sizeof(total_frame), 0, (struct sockaddr *) &from_addr, (socklen_t *) &length);
			//Get the total number of frame to recieve

			t_out.tv_sec = 0;
                	setsockopt(cfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&t_out, sizeof(struct timeval));
			//Disable the timeout option
			t = clock();
			if (total_frame > 0) {
				sendto(cfd, &(total_frame), sizeof(total_frame), 0, (struct sockaddr *) &send_addr, sizeof(send_addr));
				printf("----> %ld\n", total_frame);
				fptr = fopen(flname, "wb");	//open the file in write mode
				/*Recieve all the frames and send the acknowledgement sequentially*/
				for (i = 1; i <= total_frame; i++)
				{
				memset(&frame, 0, sizeof(frame));
				recvfrom(cfd, &(frame), sizeof(frame), 0, (struct sockaddr *) &from_addr, (socklen_t *) &length);
				//Recieve the frame
				sendto(cfd, &(frame.ID), sizeof(frame.ID), 0, (struct sockaddr *) &send_addr, sizeof(send_addr));
				//Send the ack
				/*Drop the repeated frame*/
					if ((frame.ID < i) || (frame.ID > i))
						i--;
					else {
						fwrite(frame.data, 1, frame.length, fptr);   /*Write the recieved data to the file*/
						printf("frame.ID ---> %ld	frame.length ---> %ld\n", frame.ID, frame.length);
						bytes_rec += frame.length;
					}
					if (i == total_frame) {
						printf("File recieved\n");
					}
				}
				printf("Total bytes recieved ---> %ld\n", bytes_rec);
				t = clock() - t;
				double time_taken = ((double)t)/CLOCKS_PER_SEC; // in seconds
				printf("file tranfer took %f seconds to execute \n", time_taken);
				printf("Data rate is %f bytes per sec \n", ((double)bytes_rec)/time_taken);
				fclose(fptr);
			}
			else {
				printf("File is empty\n");
			}
		}
/*-------------------"ls case"----------------------------------*/
		else if (strcmp(cmd, "ls") == 0) {
		char filename[200];
		memset(filename, 0, sizeof(filename));
		length = sizeof(from_addr);
			if ((numRead = recvfrom(cfd, filename, sizeof(filename), 0,  (struct sockaddr *) &from_addr, (socklen_t *) &length)) < 0)
				print_error("recieve");
			if (filename[0] != '\0') {
				printf("Number of bytes recieved = %ld\n", numRead);
				printf("\nThis is the List of files and directories --> \n%s \n", filename);
			}
			else {
				printf("Recieved buffer is empty\n");
				continue;
			}
		}
//-----------------exit-----------------
		else if (strcmp(cmd, "exit") == 0) {
			exit(EXIT_SUCCESS);
		}
//--------------Invalid case---------------
		else {
			printf("--------Invalid Command!----------\n");
		}
	}
	close(cfd);
	exit(EXIT_SUCCESS);
}
