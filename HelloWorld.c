/*
 ============================================================================
 Name        : HelloWorld.c
 Author      : Daniel Sierra
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

/*
 Simple udp server
 */
#include<stdio.h> //printf
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include<arpa/inet.h>
#include<sys/socket.h>
#include <pthread.h>

#define BUFLEN 512  //Max length of buffer
#define PORT 8888   //The port on which to listen for incoming data

char players[4][25];
char data_received[25];

int lives = 3;
char response[BUFLEN] = "";
char ip_single_player[11] = "";

void die(char *s) {
	perror(s);
	exit(1);
}

void createSocket(){
	struct sockaddr_in si_me, si_other;

	int s, slen = sizeof(si_other), recv_len;
	char buf[BUFLEN];

	//create a UDP socket
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		die("socket");
	}

	// zero out the structure
	memset((char *) &si_me, 0, sizeof(si_me));

	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(PORT);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);

	//bind socket to port
	if (bind(s, (struct sockaddr*) &si_me, sizeof(si_me)) == -1) {
		die("bind");
	}

	//now reply the client with the same data
	if (sendto(s, buf, recv_len, 0, (struct sockaddr*) &si_other, slen) == -1) {
		die("sendto()");
	}
}

void setPlayerLives(int lives) {
	printf("New player lives %i \n", lives);
}

void createNewPlayer(int lives) {
	pthread_t thread_id;
	//printf("Before thread\n");
	pthread_create(&thread_id, NULL, setPlayerLives, lives);
	pthread_join(thread_id, NULL);
	//printf("Thread id %i \n", thread_id);
	//exit(0);
}

int main(void) {
	struct sockaddr_in si_me, si_other;

	int s, slen = sizeof(si_other), recv_len;
	char buf[BUFLEN];
	int player_pos = 0;

	//create a UDP socket
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		die("socket");
	}

	// zero out the structure
	memset((char *) &si_me, 0, sizeof(si_me));

	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(PORT);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);

	//bind socket to port
	if (bind(s, (struct sockaddr*) &si_me, sizeof(si_me)) == -1) {
		die("bind");
	}

	//keep listening for data
	while (1) {
		printf("Waiting for players to connect...");
		fflush(stdout);

		//try to receive some data, this is a blocking call
		if ((recv_len = recvfrom(s, buf, BUFLEN, 0,
				(struct sockaddr *) &si_other, &slen)) == -1) {
			die("recvfrom()");
		}

		//print details of the client/peer and the data received
		printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr),
				ntohs(si_other.sin_port));
		printf("Data: %s\n", buf);

		printf("Play string %d\n", strstr(buf, "play") != NULL);
		//If play command and player is not already playing
		if (strstr(buf, "play") != NULL
				&& strstr(ip_single_player, inet_ntoa(si_other.sin_addr)) == NULL) {
			if (player_pos == 4)
				printf("No more players allowed\n");
			else{
				printf("Start playing \n");
				//TODO: Remove after implementing multiplayer
				strcpy(ip_single_player, inet_ntoa(si_other.sin_addr));
			}

			int is_player_added = 0;
			/*while (!strcmp("", players[player_pos]) && !is_player_added
					&& player_pos < 4) {
				strcpy(players[player_pos], inet_ntoa(si_other.sin_addr));
				printf("Player added: %s\n", players[player_pos]);
				is_player_added = 1;
				player_pos++;
				//Create new player thread
				createNewPlayer(3);
			}*/
			//strcpy(response, buf);
			strcpy(response, "3");
			createNewPlayer(3);
		} else if (strstr(buf, "hit") != NULL){
			printf("Player %s got hit\n", inet_ntoa(si_other.sin_addr));
			lives--;
			printf("%d lives left \n", lives);
			if (lives == 0)
				strcpy(response, "Game over");
		} else if (strstr(ip_single_player, inet_ntoa(si_other.sin_addr)) != NULL
				&& strstr(buf, "play") != NULL){
			strcpy(response, "Already playing");
		} else {
			strcpy(response, buf);
		}

		if (lives == 0)
			die("Game Over");

		printf("Response message %s \n", response);
		//now reply the client with the same data
		if (sendto(s, response, recv_len, 0, (struct sockaddr*) &si_other, slen)
				== -1) {
			die("sendto()");
		}
	}

	close(s);
	return 0;
}
