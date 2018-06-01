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

//char players[4][25];
char data_received[25];

int lives = 3;
char response[BUFLEN] = "";
char ip_single_player[11] = "";

struct arg_struct {
	int s;
	const void* response;
	size_t recv_len;
	int flags;
	const struct sockaddr* si_other;
	socklen_t slen;
};

struct player {
	char ip[15];
	int lives;
};
struct player players[4];

void die(char *s) {
	perror(s);
	exit(1);
}

/*void setPlayerLives(struct arg_struct* args) {
	printf("New player lives %i \n", lives);
	if (sendto(args->s, args->response, args->recv_len, 0, (struct sockaddr*) &args->si_other, args->slen)
			== -1) {
		die("sendto()");
	}
}*/

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

/*void createPlayerThread(int s, const void *response, size_t recv_len,
		int flags, const struct sockaddr *si_other, socklen_t slen){
	struct arg_struct args;
	args.s = s;
	args.response = response;
	args.recv_len = recv_len;
	args.flags = flags;
	args.si_other = si_other;
	args.slen = slen;
	pthread_t thread_id;
	pthread_create(&thread_id, NULL, setPlayerLives, (void *)&args);
	pthread_join(thread_id, NULL);
}*/

int addPlayer(int player_pos, char ip_adress[]){
	int is_player_added = 0;

	//Add player to array if it doesnt exist
	while (!strcmp("", players[player_pos].ip) && !is_player_added
			&& player_pos < 4) {
		strcpy(players[player_pos].ip, ip_adress);
		players[player_pos].lives = 3;
		printf("Player added: %s\n", players[player_pos].ip);
		is_player_added = 1;
		player_pos++;
		//Create new player thread
		createNewPlayer(3);
	}
	return player_pos;
}

int hitPlayer(char ip[]){
	printf("Player %s got hit\n", ip);
	for (int i = 0; i < 4; i++){
		if (strstr(players[i].ip, ip)){
			players[i].lives--;
			printf("Remaining lives %d\n", players[i].lives);
			return players[i].lives;
		}
	}
	return 0;
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
		printf("Waiting for players...");
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

			player_pos = addPlayer(player_pos, inet_ntoa(si_other.sin_addr));

			strcpy(response, "play");
			//createNewPlayer(3);
			//createPlayerThread(s, response, recv_len, 0, (struct sockaddr*) &si_other, slen);
		} else if (strstr(buf, "hit") != NULL){
			/*printf("Player %s got hit\n", inet_ntoa(si_other.sin_addr));
			lives--;
			printf("%d lives left \n", lives);*/
			lives = hitPlayer(inet_ntoa(si_other.sin_addr));
			if (lives == 0)
				strcpy(response, "die");
		} else if (strstr(ip_single_player, inet_ntoa(si_other.sin_addr)) != NULL
				&& strstr(buf, "play") != NULL){
			strcpy(response, "play");
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
