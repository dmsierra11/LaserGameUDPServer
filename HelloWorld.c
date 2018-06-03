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

int addPlayer(int player_pos, struct in_addr ip_address){
	//int is_player_added = 0;
	printf("Player ip: %s\n", inet_ntoa(ip_address));
	players[player_pos].ip =  ip_address;
	players[player_pos].lives = 3;
	printf("Player added: %s\n", inet_ntoa(players[player_pos].ip));
	//is_player_added = 1;
	player_pos++;

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
		} else if (strstr(buf, "hit") != NULL){
			lives = hitPlayer(inet_ntoa(si_other.sin_addr));
			strcpy(response, "play");
		} else {
			strcpy(response, buf);
		}

		if (lives == 0){
			strcpy(response, "die");
			gameOver(s, recv_len, 0, slen);
		}

		printf("Response message %s \n", response);
		if (sendto(s, response, recv_len, 0, (struct sockaddr*) &si_other, slen)
				== -1) {
			die("sendto()");
		}
	}

	close(s);
	return 0;
}
