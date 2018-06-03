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

char data_received[25];

int lives = 3;
char response[BUFLEN] = "";

struct player {
	struct in_addr ip;
	int lives;
};
struct player players[4];

void die(char *s) {
	perror(s);
	exit(1);
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

int addPlayer(int player_pos, struct in_addr ip_address){
	//int is_player_added = 0;
	printf("Player ip: %s\n", inet_ntoa(ip_address));
	//Add player to array if it doesnt exist
	//if (playerExists(inet_ntoa(ip_address)) == 0){
		players[player_pos].ip =  ip_address;
		players[player_pos].lives = 3;
		printf("Player added: %s\n", inet_ntoa(players[player_pos].ip));
		//is_player_added = 1;
		player_pos++;
		//Create new player thread
		createNewPlayer(3);
	/*} else {
		printf("Already playing\n");
	}*/

	checkPlayerSlots();
	return player_pos;
}

int hitPlayer(char ip[]){
	printf("Player %s got hit\n", ip);
	for (int i = 0; i < 4; i++){
		if (strstr(inet_ntoa(players[i].ip), ip)){
			players[i].lives--;
			printf("Remaining lives %d\n", players[i].lives);
			return players[i].lives;
		}
	}
	return 0;
}

void checkPlayerSlots(){
	for (int i = 0; i < 4; i++){
		printf("Player %d %s has %d lives\n", i, inet_ntoa(players[i].ip), players[i].lives);
	}
}

int strCompare(char ip1[], char ip2[]){

	printf("Incoming ip: %s \n",ip1);
	printf("Current ip: %s \n",ip2);

	int result;

	/* Create two arrays to hold our data */
	char example1[50];
	char example2[50];

	/* Copy two strings into our data arrays */
	strcpy(example1, ip1);
	strcpy(example2, ip2);

	/* Compare the two strings provided */
	result = strcmp(example1, example2);

	/* If the two strings are the same say so */
	if (result == 0) {
		printf("Strings are the same\n");
	}

	/* If the first string is less than the second say so
	 (This is because the 'a' in the word 'at' is less than
	 the 'i' in the word 'is' */
	if (result < 0) printf("Second string is less than the first\n");

	return result;
}

int playerExists(char ip[]){
	for (int i = 0; i < 4; i++){
		//printf("Player exists? %s \n"+strstr(inet_ntoa(players[i].ip), ip));
		if (strCompare(inet_ntoa(players[i].ip), ip) == 0){
			printf("Player %d %s has %d lives\n", i, inet_ntoa(players[i].ip), players[i].lives);
			return 1;
		}
	}
	return 0;
}

void gameOver(int s, size_t len, int flags, socklen_t slen){

	struct sockaddr_in si_player;

	int player_pos = 0;
	//While there is an ip and size is less than 4
	while (!strstr("0.0.0.0", inet_ntoa(players[player_pos].ip)) && player_pos < 4) {
		// zero out the structure

		si_player.sin_family = AF_INET;
		si_player.sin_port = htons(PORT);
		si_player.sin_addr = players[player_pos].ip;

		printf("Sending message to %s \n", inet_ntoa(si_player.sin_addr));
		if (sendto(s, "end", len, 0, (struct sockaddr*) &si_player, slen)
				== -1) {
			die("sendto()");
		}

		//Initialize lives for next game
		players[player_pos].lives = 3;

		player_pos++;
	}
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
		strcpy(buf, "");
		strcpy(response, "");
		printf("Waiting for players...\n");
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
		if (strstr(buf, "play") != NULL) {
			if (player_pos == 4)
				printf("No more players allowed\n");

			player_pos = addPlayer(player_pos, si_other.sin_addr);

			strcpy(response, "play");
			//createNewPlayer(3);
		} else if (strstr(buf, "hit") != NULL){
			lives = hitPlayer(inet_ntoa(si_other.sin_addr));
			strcpy(response, "play");
		} else {
			strcpy(response, buf);
		}

		if (lives == 0){
			strcpy(response, "die");
			gameOver(s, recv_len, 0, slen);
			//die("Game Over");
		}

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
