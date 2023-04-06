#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <SDL/SDL.h>
#include "./presentation/presentation.h"

#define VIDE        		0
#define DKJR       		1
#define CROCO       		2
#define CORBEAU     		3
#define CLE 			4

#define AUCUN_EVENEMENT    	0

#define LIBRE_BAS		1
#define LIANE_BAS		2
#define DOUBLE_LIANE_BAS	3
#define LIBRE_HAUT		4
#define LIANE_HAUT		5

void* FctThreadEvenements(void *);
void* FctThreadCle(void *);
void* FctThreadDK(void *);
void* FctThreadDKJr(void *);
void* FctThreadScore(void *);
void* FctThreadEnnemis(void *);
void* FctThreadCorbeau(void *);
void* FctThreadCroco(void *);

void initGrilleJeu();
void setGrilleJeu(int l, int c, int type = VIDE, pthread_t tid = 0);
void afficherGrilleJeu();

void HandlerSIGUSR1(int);
void HandlerSIGUSR2(int);
void HandlerSIGALRM(int);
void HandlerSIGINT(int);
void HandlerSIGQUIT(int);
void HandlerSIGCHLD(int);
void HandlerSIGHUP(int);

void DestructeurVS(void *p);

pthread_t threadCle;
pthread_t threadDK;
pthread_t threadDKJr;
pthread_t threadEvenements;
pthread_t threadScore;
pthread_t threadEnnemis;

pthread_cond_t condDK;
pthread_cond_t condScore;

pthread_mutex_t mutexGrilleJeu;
pthread_mutex_t mutexDK;
pthread_mutex_t mutexEvenement;
pthread_mutex_t mutexScore;
pthread_mutex_t mutexDelai;

pthread_key_t keySpec;

bool MAJDK = false;
int  score = 0;
bool MAJScore = true;
int  delaiEnnemis = 4000;
int  positionDKJr = 1;
int  evenement = AUCUN_EVENEMENT;
int etatDKJr;

//variable globale personnelle
int VeriScore = 0;
int vie = 1;

typedef struct
{
  int type;
  pthread_t tid;
} S_CASE;

S_CASE grilleJeu[4][8];

typedef struct
{
  bool haut;
  int position;
} S_CROCO;

// ------------------------------------------------------------------------

int main(int argc, char* argv[])
{
	//pour initialiser le interface graphique
	ouvrirFenetreGraphique();

	//armenment du signal sigquit
	struct sigaction A;
	A.sa_handler = HandlerSIGQUIT;
	sigemptyset(&A.sa_mask);
	A.sa_flags = 0;

	if (sigaction(SIGQUIT,&A,NULL) == -1)
	{
		perror("Erreur de sigaction");
		exit(1);
	}

	//armement du signal sigalarm
	struct sigaction B;
  B.sa_handler = HandlerSIGALRM;
  sigemptyset(&B.sa_mask);
  B.sa_flags = 0;

  if (sigaction(SIGALRM,&B,NULL) == -1)
  {
    perror("Erreur de sigaction");
    exit(1);
  }

  //armement du signal SIGUSR1
	struct sigaction C;
  C.sa_handler = HandlerSIGUSR1;
  sigemptyset(&C.sa_mask);
  C.sa_flags = 0;

  if (sigaction(SIGUSR1,&C,NULL) == -1)
  {
    perror("Erreur de sigaction");
    exit(1);
  }

  //armenment du signal sigint
	struct sigaction D;
	D.sa_handler = HandlerSIGINT;
	sigemptyset(&D.sa_mask);
	D.sa_flags = 0;

	if (sigaction(SIGINT,&D,NULL) == -1)
	{
		perror("Erreur de sigaction");
		exit(1);
	}

	//armement du signal SIGUSR2
	struct sigaction E;
  E.sa_handler = HandlerSIGUSR2;
  sigemptyset(&E.sa_mask);
  E.sa_flags = 0;

  if (sigaction(SIGUSR2,&E,NULL) == -1)
  {
    perror("Erreur de sigaction");
    exit(1);
  }

  //armement du signal SIGUSR2
	struct sigaction F;
  F.sa_handler = HandlerSIGHUP;
  sigemptyset(&F.sa_mask);
  F.sa_flags = 0;

  if (sigaction(SIGHUP,&F,NULL) == -1)
  {
    perror("Erreur de sigaction");
    exit(1);
  }

  //armement du signal SIGCHLD
	struct sigaction G;
  G.sa_handler = HandlerSIGCHLD;
  sigemptyset(&G.sa_mask);
  G.sa_flags = 0;

  if (sigaction(SIGCHLD,&G,NULL) == -1)
  {
    perror("Erreur de sigaction");
    exit(1);
  }

	//masquage des signaux
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask,SIGQUIT);
	sigaddset(&mask,SIGALRM);
	sigaddset(&mask,SIGUSR1);
	sigaddset(&mask,SIGINT);
	sigaddset(&mask,SIGUSR2);
	sigaddset(&mask,SIGHUP);
	sigaddset(&mask,SIGCHLD);

	//initiliser cond DK
	pthread_cond_init(&condDK, NULL);

	//initialiser cond score
	pthread_cond_init(&condScore, NULL);

	// Mise en place du nouveau masque de signal
	sigprocmask(SIG_SETMASK,&mask,NULL);

	//initialiser mutex evenement
	pthread_mutex_init(&mutexEvenement, NULL);

	//initialiser mutex GrilleJeu
	pthread_mutex_init(&mutexGrilleJeu, NULL);

	//initialiser mutex DK
	pthread_mutex_init(&mutexDK, NULL);

	//initialiser mutex score
	pthread_mutex_init(&mutexScore, NULL);

	//initialiser mutex delai
	pthread_mutex_init(&mutexDelai, NULL);

	//creer notre cle keyspec qui va etre utiliser pour fournir la position du courbeau et croco
	pthread_key_create(&keySpec, DestructeurVS);

	//commencer le thread cle
	pthread_create(&threadCle, NULL, (void *(*) (void *))FctThreadCle, NULL);

	//commencer le thread evenement
	pthread_create(&threadEvenements, NULL, (void *(*) (void *))FctThreadEvenements, NULL);

	//commencer le thread donkey kong
	pthread_create(&threadDK, NULL, (void *(*) (void *))FctThreadDK, NULL);

	//commencer le thread score
	pthread_create(&threadScore, NULL, (void *(*) (void *))FctThreadScore, NULL);

	//commencer le thread ennemis
	pthread_create(&threadEnnemis, NULL, (void *(*) (void *))FctThreadEnnemis, NULL);

	while(vie <= 3)
	{
		//commencer le thread DKJr
		pthread_create(&threadDKJr, NULL, (void *(*) (void *))FctThreadDKJr, NULL);

		pthread_join(threadDKJr, NULL);

		afficherEchec(vie);

		vie++;
	}

	pthread_join(threadEvenements, NULL);
}

// -------------------------------------

void initGrilleJeu()
{
  int i, j;   
  
  pthread_mutex_lock(&mutexGrilleJeu);

  for(i = 0; i < 4; i++)
    for(j = 0; j < 7; j++)
      setGrilleJeu(i, j);

  pthread_mutex_unlock(&mutexGrilleJeu);
}

// -------------------------------------

void setGrilleJeu(int l, int c, int type, pthread_t tid)
{
  grilleJeu[l][c].type = type;
  grilleJeu[l][c].tid = tid;
}

// -------------------------------------

void afficherGrilleJeu()
{   
   for(int j = 0; j < 4; j++)
   {
       for(int k = 0; k < 8; k++)
          printf("%d  ", grilleJeu[j][k].type);
       printf("\n");
   }

   printf("\n");   
}

// -------------------------------------
//LES THREADS

void* FctThreadCle(void *)
{
	int i = 1;

	struct timespec rest, requete = { 0, 700000000 };

	while(1)
	{
		if(i == 5) i = 1;
		
		pthread_mutex_lock(&mutexGrilleJeu);
		effacerCarres(3, 12, 2, 4);
		afficherCle(i);
		pthread_mutex_unlock(&mutexGrilleJeu);

		if(i == 1) grilleJeu[0][1].type = CLE;
		else grilleJeu[0][1].type = VIDE;

		//printf("Thread cle position: %d - Valeur = %d\n", i, grilleJeu[0][1].type);

		i++;

		//sleep(1);
		nanosleep(&requete, &rest);
	}
}

// -------------------------------------

void* FctThreadEvenements(void *)
{
	struct timespec rest, requete = { 0, 100000000 };

	while (1)
	{
		pthread_mutex_lock(&mutexEvenement);

    evenement = lireEvenement();

    kill(getpid(), SIGQUIT);
    
    if(evenement == SDL_QUIT) exit(0);

    pthread_mutex_unlock(&mutexEvenement); 

		//printf("valeur evenement = %d ---- ", evenement);

		nanosleep(&requete, &rest);

		pthread_mutex_lock(&mutexEvenement);
		evenement = AUCUN_EVENEMENT;
		pthread_mutex_unlock(&mutexEvenement); 

		//printf("valeur evenement apres sleep = %d\n", evenement);
	}
}

// -------------------------------------

void* FctThreadDKJr(void *)
{
	printf("Thread DKJR Cree: %d.%u\n", getpid(), pthread_self());

	sigset_t maskDKJR;
	sigfillset(&maskDKJR);
	sigdelset(&maskDKJR,SIGQUIT); //pour permettre le thread dkjr de recoivoir le signal SIGQUIT
	sigdelset(&maskDKJR,SIGINT); //pour permettre le thread dkjr de recoivoir le signal SIGINT
	sigdelset(&maskDKJR,SIGHUP); //pour permettre le thread dkjr de recoivoir le signal SIGHUP
	sigdelset(&maskDKJR,SIGCHLD); //pour permettre le thread dkjr de recoivoir le signal SIGCHLD

	// Mise en place du nouveau masque de signal
	sigprocmask(SIG_SETMASK,&maskDKJR,NULL);

	/*---------------------------------------*/

	bool on = true; 
	struct timespec rest, requete;

	pthread_mutex_lock(&mutexGrilleJeu);

	//pour tuer les courbeau
	if(grilleJeu[2][0].type == CORBEAU) pthread_kill(grilleJeu[2][0].tid, SIGUSR1);
	if(grilleJeu[2][1].type == CORBEAU) pthread_kill(grilleJeu[2][1].tid, SIGUSR1);
	if(grilleJeu[2][2].type == CORBEAU) pthread_kill(grilleJeu[2][2].tid, SIGUSR1);

	//pour tuer les croco
	if(grilleJeu[3][1].type == CROCO) pthread_kill(grilleJeu[3][1].tid, SIGUSR2);
	if(grilleJeu[3][2].type == CROCO) pthread_kill(grilleJeu[3][2].tid, SIGUSR2);
	if(grilleJeu[3][3].type == CROCO) pthread_kill(grilleJeu[3][3].tid, SIGUSR2);
	 
	setGrilleJeu(3, 1, DKJR); 
	afficherDKJr(11, 9, 1); 
	effacerCarres(11, 7, 2, 2);
	etatDKJr = LIBRE_BAS; 
	positionDKJr = 1;
	 
	pthread_mutex_unlock(&mutexGrilleJeu);

	while(on)
	{
		pause();
		
		pthread_mutex_lock(&mutexEvenement);
 		pthread_mutex_lock(&mutexGrilleJeu);

 		switch (etatDKJr)
 		{
 			case LIBRE_BAS:
 				switch (evenement)
				{
					case SDLK_LEFT:

						 	if (positionDKJr > 1)
						 	{
						 		setGrilleJeu(3, positionDKJr);
						 		effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);

						 		positionDKJr--;

						 		setGrilleJeu(3, positionDKJr, DKJR);
						 		afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
						 	}
					 	break;

					case SDLK_RIGHT:
							if (positionDKJr < 7)
						 	{
						 		setGrilleJeu(3, positionDKJr);
						 		effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);

						 		positionDKJr++;

						 		setGrilleJeu(3, positionDKJr, DKJR);
						 		afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
						 	}
					 	break;

					case SDLK_UP:
							if (positionDKJr == 2 || positionDKJr == 3 || positionDKJr == 4 || positionDKJr == 6)
						 	{
						 		if(grilleJeu[2][positionDKJr].type == CORBEAU)
						 		{
						 			printf("REKT BY COURBEAU LLLLLL\n");

						 			setGrilleJeu(3, positionDKJr);
								 	effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);

						 			/*
										pthread_kill est utilise pour envoyer un signal a un thread specific, si on fais un kill normale
										il va selectionner lui meme un ennemis a tuer donc on peut se trouver dans une situation ou
										on est tuer par l'ennemis mais lui il ne mort pas mais l'ennemis qui se trouve devant lui mort
										donc il faut utiliser pthread_kill en place du kill normale
						 			*/
						 			pthread_kill(grilleJeu[2][positionDKJr].tid, SIGUSR1);

									on = false;
						 		}
						 		else
						 		{
						 			//on efface jr sur le sol
							 		setGrilleJeu(3, positionDKJr);
							 		effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);

							 		//on va lui afficher en train de sauter avec l'image 8
							 		setGrilleJeu(2, positionDKJr, DKJR);
							 		afficherDKJr(10, (positionDKJr * 2) + 7, 8);

							 		//mettre un timeur de 1.4 secondes
							 		requete = { 1, 400000000 };

							 		//liberer le mutex grillejeu
							 		pthread_mutex_unlock(&mutexGrilleJeu);

							 		//faire le sleep
							 		nanosleep(&requete, &rest);

							 		//reprendre en main le mutex
							 		pthread_mutex_lock(&mutexGrilleJeu);

							 		//effacer jr en train de sauter
							 		setGrilleJeu(2, positionDKJr);
							 		effacerCarres(10, (positionDKJr * 2) + 7, 2, 2);

							 		if(grilleJeu[3][positionDKJr].type == CROCO)
									{
										printf("L by croco\n");

										pthread_kill(grilleJeu[3][positionDKJr].tid, SIGUSR2);

										on = false;
									}
									else
									{
								 		//lui afficher sur le sol
								 		setGrilleJeu(3, positionDKJr, DKJR);
								 		afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);

								 		if(grilleJeu[3][positionDKJr-1].type == CROCO)
								 		{
								 			//pour augmenter le score
							 				pthread_mutex_lock(&mutexScore);

							 				MAJScore = true;
							 				score += 1;

							 				pthread_mutex_unlock(&mutexScore);

							 				//signaler que le score vient de changer
							 				pthread_cond_signal(&condScore);
								 		}

								 		//afficherGrilleJeu();
									}
						 		}
						 	}
						 	else
						 	{
						 		if(grilleJeu[2][positionDKJr].type == CORBEAU)
						 		{
						 			printf("REKT BY COURBEAU LLLLLL\n");

						 			setGrilleJeu(3, positionDKJr);
								 	effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);

						 			pthread_kill(grilleJeu[2][positionDKJr].tid, SIGUSR1);

									on = false;
						 		}
						 		else
						 		{
						 			if(positionDKJr == 1 || positionDKJr == 5)
								 	{
								 		//on efface jr sur le sol
								 		setGrilleJeu(3, positionDKJr);
								 		effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);

								 		//on va lui afficher avec image 7
								 		setGrilleJeu(2, positionDKJr, DKJR);
								 		afficherDKJr(10, (positionDKJr * 2) + 7, 7);

								 		etatDKJr = LIANE_BAS;
								 	}
								 	else
								 	{
								 		//on efface jr sur le sol
								 		setGrilleJeu(3, positionDKJr);
								 		effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);

								 		//on va lui afficher avec l'image 5
								 		setGrilleJeu(2, positionDKJr, DKJR);
								 		afficherDKJr(10, (positionDKJr * 2) + 7, 5);

								 		etatDKJr = DOUBLE_LIANE_BAS;
								 	}
						 		}
						 	}
					 	break;
				}
				break;
 			case LIANE_BAS:
 					switch(evenement)
 					{
 						case SDLK_DOWN:
 								//effacer jr en grimper
						 		setGrilleJeu(2, positionDKJr);
						 		effacerCarres(10, (positionDKJr * 2) + 7, 2, 2);

						 		//lui afficher sur le sol
						 		setGrilleJeu(3, positionDKJr, DKJR);
						 		afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);

						 		etatDKJr = LIBRE_BAS;
 							break;
 					}
 				break;
 			case DOUBLE_LIANE_BAS:
 					switch(evenement)
 					{
 						case SDLK_UP:
 								//effacer jr en train de grimper
						 		setGrilleJeu(2, positionDKJr);
						 		effacerCarres(10, (positionDKJr * 2) + 7, 2, 2);

						 		//lui afficher en train de grimper plus haut
						 		setGrilleJeu(1, positionDKJr, DKJR);
						 		afficherDKJr(7, (positionDKJr * 2) + 7, 6);

						 		etatDKJr = LIBRE_HAUT;
 							break;
 						case SDLK_DOWN:
 								//effacer jr en train de grimper
						 		setGrilleJeu(2, positionDKJr);
						 		effacerCarres(10, (positionDKJr * 2) + 7, 2, 2);

						 		//lui afficher sur le sol
						 		setGrilleJeu(3, positionDKJr, DKJR);
						 		afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);

						 		etatDKJr = LIBRE_BAS;
 							break;
 					}
 				break;
 			case LIBRE_HAUT:
 					switch(evenement)
 					{
 						case SDLK_UP:
 							if(positionDKJr == 3 || positionDKJr == 4)
							{
								//on efface jr sur le sol
						 		setGrilleJeu(1, positionDKJr);
						 		effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);

						 		//on va lui afficher en train de sauter avec l'image 8
						 		setGrilleJeu(0, positionDKJr, DKJR);
						 		afficherDKJr(6, (positionDKJr * 2) + 7, 8);

						 		//mettre un timeur de 1.4 secondes
						 		requete = { 1, 400000000 };

						 		//liberer le mutex grillejeu
						 		pthread_mutex_unlock(&mutexGrilleJeu);

						 		//faire le sleep
						 		nanosleep(&requete, &rest);

						 		//reprendre en main le mutex
						 		pthread_mutex_lock(&mutexGrilleJeu);

						 		//effacer jr en train de sauter
						 		setGrilleJeu(0, positionDKJr);
						 		effacerCarres(6, (positionDKJr * 2) + 7, 2, 2);

						 		if(grilleJeu[1][positionDKJr].type == CROCO)
								{
									printf("L by croco haut\n");

									pthread_kill(grilleJeu[1][positionDKJr].tid, SIGUSR2);

									on = false;
								}
								else
								{
									//lui afficher sur le sol
							 		setGrilleJeu(1, positionDKJr, DKJR);
							 		afficherDKJr(7, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);

							 		if(grilleJeu[1][positionDKJr+1].type == CROCO)
							 		{
							 			//pour augmenter le score
						 				pthread_mutex_lock(&mutexScore);

						 				MAJScore = true;
						 				score += 1;

						 				pthread_mutex_unlock(&mutexScore);

						 				//signaler que le score vient de changer
						 				pthread_cond_signal(&condScore);
							 		}
								}
							}
							else
							{
								if(evenement == SDLK_UP && positionDKJr == 6)
								{
									//on efface jr sur le sol
							 		setGrilleJeu(1, positionDKJr);
							 		effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);

							 		//on va lui afficher avec image 7
							 		setGrilleJeu(0, positionDKJr, DKJR);
							 		afficherDKJr(6, (positionDKJr * 2) + 7, 7);

									etatDKJr = LIANE_HAUT;
								}
							}
 							
 						break;

 						case SDLK_DOWN:
 							if(positionDKJr == 7)
 							{
 								//effacer jr en train de grimper en haut
						 		setGrilleJeu(1, positionDKJr);
						 		effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);

						 		//lui afficher en train de grimper plus bas
						 		setGrilleJeu(2, positionDKJr, DKJR);
						 		afficherDKJr(10, (positionDKJr * 2) + 7, 5);

						 		etatDKJr = DOUBLE_LIANE_BAS;
 							}
 						break;

 						case SDLK_LEFT:

 							if(positionDKJr > 3)
 							{
 								setGrilleJeu(1, positionDKJr);
						 		effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);

						 		positionDKJr--;

						 		setGrilleJeu(1, positionDKJr, DKJR);
						 		afficherDKJr(7, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
 							}
 							else
 							{
 								setGrilleJeu(1, positionDKJr);
						 		effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);

 								setGrilleJeu(0, positionDKJr, DKJR);
					 			afficherDKJr(7, (positionDKJr * 2) + 7, 9);

					 			requete = { 0, 500000000 };

					 			if(grilleJeu[0][1].type == CLE)
					 			{
					 				printf("CLE CAPTURER\n");

					 				//faire le sleep
				 					nanosleep(&requete, &rest);

				 					//efface jr en train de sauter pour le cle et le cle
						 			setGrilleJeu(0, positionDKJr);
						 			effacerCarres(3, 12, 2, 4);
						 			effacerCarres(5, (2 * 2) + 7, 3, 3);
						 			afficherCage(4);

						 			//afficher jr avec le cle
						 			setGrilleJeu(0, positionDKJr, DKJR);
					 				afficherDKJr(6, (positionDKJr * 2) + 7, 10);

					 				//pour augmenter le score
					 				pthread_mutex_lock(&mutexScore);

					 				MAJScore = true;
					 				score += 10;

					 				pthread_mutex_unlock(&mutexScore);

					 				//signaler que le score vient de changer
					 				pthread_cond_signal(&condScore);

					 				pthread_mutex_lock(&mutexDK);

									MAJDK = true;

									pthread_mutex_unlock(&mutexDK); 

									pthread_cond_signal(&condDK); //pour envoyer le signal

					 				requete = { 0, 200000000 };

					 				nanosleep(&requete, &rest);

					 				//efface jr avec le cle
						 			setGrilleJeu(0, positionDKJr);
						 			effacerCarres(3, (2 * 2) + 7, 3, 2);
						 			afficherCage(4);

						 			//afficher jr heureuse
						 			setGrilleJeu(1, positionDKJr, DKJR);
					 				afficherDKJr(7, (positionDKJr * 2) + 7, 11);

					 				requete = { 0, 500000000 };

					 				nanosleep(&requete, &rest);
					 		
					 				setGrilleJeu(1, positionDKJr);
					 				effacerCarres(6, 10, 2, 3);

					 				//pour tuer les courbeau
									if(grilleJeu[2][0].type == CORBEAU) pthread_kill(grilleJeu[2][0].tid, SIGUSR1);
									if(grilleJeu[2][1].type == CORBEAU) pthread_kill(grilleJeu[2][1].tid, SIGUSR1);
									if(grilleJeu[2][2].type == CORBEAU) pthread_kill(grilleJeu[2][2].tid, SIGUSR1);

									//pour tuer les croco
									if(grilleJeu[3][1].type == CROCO) pthread_kill(grilleJeu[3][1].tid, SIGUSR2);
									if(grilleJeu[3][2].type == CROCO) pthread_kill(grilleJeu[3][2].tid, SIGUSR2);
									if(grilleJeu[3][3].type == CROCO) pthread_kill(grilleJeu[3][3].tid, SIGUSR2);

									//printf("hello\n");

					 				//lui afficher sur le sol
							 		setGrilleJeu(3, 1, DKJR); 
									afficherDKJr(11, 9, 1); 
									etatDKJr = LIBRE_BAS; 
									positionDKJr = 1;

									//printf("hello2\n");
					 			}
					 			else 
					 			{
				 					printf("L\n");

				 					//faire le sleep
				 					pthread_mutex_unlock(&mutexGrilleJeu);
					 				nanosleep(&requete, &rest);
					 				pthread_mutex_lock(&mutexGrilleJeu);

				 					//efface jr en train de sauter pour le cle
						 			setGrilleJeu(0, positionDKJr);
						 			effacerCarres(5, (2 * 2) + 7, 3, 3);
						 			afficherCage(4);

						 			//afficher jr en train de tomber
						 			setGrilleJeu(0, positionDKJr, DKJR);
					 				afficherDKJr(6, (positionDKJr * 2) + 7, 12);

					 				requete = { 0, 200000000 };

					 				pthread_mutex_unlock(&mutexGrilleJeu);
					 				nanosleep(&requete, &rest);
					 				pthread_mutex_lock(&mutexGrilleJeu);

					 				//efface jr entreint de tomber
						 			setGrilleJeu(0, positionDKJr);
						 			effacerCarres(6, (2 * 2) + 7, 2, 2);

						 			//afficher jr dans les buissons
						 			setGrilleJeu(3, 1, DKJR);
					 				afficherDKJr(11, 8, 13);

					 				requete = { 0, 200000000 };

					 				pthread_mutex_unlock(&mutexGrilleJeu);
					 				nanosleep(&requete, &rest);
					 				pthread_mutex_lock(&mutexGrilleJeu);

						 			setGrilleJeu(3, positionDKJr);
						 			
						 			//pour signaler le fin
						 			on = false;
								}
 							}

					 	break;

						case SDLK_RIGHT:
							if (positionDKJr < 7)
						 	{
						 		setGrilleJeu(1, positionDKJr);
						 		effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);

						 		positionDKJr++;

						 		if(positionDKJr == 7)
						 		{
							 		setGrilleJeu(1, positionDKJr, DKJR);
							 		afficherDKJr(7, (positionDKJr * 2) + 7, 6);
						 		}
						 		else
						 		{
						 			setGrilleJeu(1, positionDKJr, DKJR);
						 			afficherDKJr(7, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
						 		}
						 	}
					 	break;
 					}
 				break;
 			case LIANE_HAUT:
				switch(evenement)
				{
					case SDLK_DOWN:
							
							//effacer jr en train de sauter
							setGrilleJeu(0, positionDKJr);
							effacerCarres(6, (positionDKJr * 2) + 7, 2, 2);

					 		//lui afficher sur le sol
					 		setGrilleJeu(1, positionDKJr, DKJR);
						 	afficherDKJr(7, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);

					 		etatDKJr = LIBRE_HAUT;
 						break;
				}
 				break;

 		}

 		//afficherGrilleJeu();

 		pthread_mutex_unlock(&mutexGrilleJeu);
 		pthread_mutex_unlock(&mutexEvenement);
	}

	pthread_exit(NULL);
}

// -------------------------------------

void* FctThreadDK(void *)
{
	printf("Thread Donkey Kong Cree: %d.%u\n", getpid(), pthread_self());

	int i = 1;
	struct timespec rest, requete;

	while(1)
	{
		afficherCage(1);
		afficherCage(2);
		afficherCage(3);
		afficherCage(4);

		i = 1;

		while(i <= 4)
		{
			pthread_cond_wait(&condDK, &mutexDK);

			if(MAJDK == true)
			{
				switch(i)
				{
					case 1:
							printf("1\n");
							effacerCarres(2,7,2,2);
						break;
					case 2:
							printf("2\n");
							effacerCarres(2,9,2,2);
						break;
					case 3:
							printf("3\n");
							effacerCarres(4,7,2,2);
						break;
					case 4:
							printf("4\n");
							//effacer le dernier cage

							requete = { 0, 200000000 };
							nanosleep(&requete, &rest);

							effacerCarres(4,9,2,3);

							afficherRireDK();
							requete = { 0, 700000000 };

							//pour augmenter le score
			 				pthread_mutex_lock(&mutexScore);

			 				MAJScore = true;
			 				score += 10;

			 				pthread_mutex_unlock(&mutexScore);

			 				//signaler que le score vient de changer
			 				pthread_cond_signal(&condScore);

							pthread_mutex_lock(&mutexGrilleJeu);
							nanosleep(&requete, &rest);
						 	pthread_mutex_unlock(&mutexGrilleJeu);

						 	//effacer le rire
						 	effacerCarres(3,8,2,2);

						break;
				}

				i++;

				MAJDK = false;
			}
		}
	}

	pthread_exit(NULL);
}

// -------------------------------------

void* FctThreadScore(void *)
{
	printf("Thread Score Cree: %d.%u\n", getpid(), pthread_self());

	afficherScore(0);

	while(1)
	{
		pthread_cond_wait(&condScore, &mutexScore);

		if(MAJScore == true)
		{
			printf("update score\n");

			afficherScore(score);

			if((score - VeriScore) >= 300)
			{
				VeriScore = score;
				if(vie > 1) vie--;

				int i = 1;

				//effacer les tetes du dkjr
				effacerCarres(7, 27, 1, 3);

				if(delaiEnnemis < 3000)
				{
					pthread_mutex_lock(&mutexDelai);

			 		delaiEnnemis = 3000;
					alarm(15);

			 		pthread_mutex_unlock(&mutexDelai);
				}

				while(i < vie)
				{
					afficherEchec(i);
					i++;
				}
			}

			MAJScore = false;
		}
	}

	pthread_exit(NULL);
}

// -------------------------------------

void* FctThreadEnnemis(void *)
{
	printf("Thread Ennemis Cree: %d.%u\n", getpid(), pthread_self());

	sigset_t maskEnnemis;
	sigfillset(&maskEnnemis);
	sigdelset(&maskEnnemis,SIGALRM); //pour permettre le thread ennemis de recevoir le signal SIGALARM

	// Mise en place du nouveau masque de signal
	sigprocmask(SIG_SETMASK,&maskEnnemis,NULL);

	/*---------------------------------------*/

	alarm(15);
	srand((unsigned) time(NULL));

	int timeRespawnSec = delaiEnnemis/1000;
	int timeRespawnNsec = (delaiEnnemis%1000)*1000000;
	struct timespec rest, requete = { timeRespawnSec, timeRespawnNsec };
	int random;
	pthread_t threadCourbeau, threadCroco;

	while(1)
	{
		nanosleep(&requete, &rest);

		printf("time respawn = %d.%d\n", timeRespawnSec, timeRespawnNsec);

		random = 1 + (rand() % 2);

		if(random == 1)
		{
			//printf("CORBEAU\n");

			//commencer le thread courbeau
			pthread_create(&threadCourbeau, NULL, (void *(*) (void *))FctThreadCorbeau, NULL);
		}
		else
		{
			//printf("CROCO\n");

			//commencer le thread croco
			pthread_create(&threadCroco, NULL, (void *(*) (void *))FctThreadCroco, NULL);
		}

		pthread_mutex_lock(&mutexDelai);

		timeRespawnSec = delaiEnnemis/1000;
		timeRespawnNsec = (delaiEnnemis%1000)*1000000;

		pthread_mutex_unlock(&mutexDelai);

		requete = { timeRespawnSec, timeRespawnNsec };
	}

	pthread_exit(NULL);
}

// -------------------------------------

void* FctThreadCorbeau(void *)
{
	printf("Thread Courbeau Cree: %d.%u\n", getpid(), pthread_self());

	sigset_t maskCourbeau;
	sigfillset(&maskCourbeau);
	sigdelset(&maskCourbeau,SIGUSR1); //pour permettre le thread courbeau de recevoir le signal SIGUSR1

	// Mise en place du nouveau masque de signal
	sigprocmask(SIG_SETMASK,&maskCourbeau,NULL);

	/*---------------------------------------*/

	int positionCourbeau = 0;
	struct timespec rest, requete = { 0, 700000000 };

	//creation du memoire pour notre cle
	int *PosCourb = (int *)malloc(sizeof(int));

	setGrilleJeu(2, positionCourbeau, CORBEAU, pthread_self());
 	afficherCorbeau(8, 2);

 	*PosCourb = positionCourbeau;
 	pthread_setspecific(keySpec, PosCourb);

 	nanosleep(&requete, &rest);

	while(positionCourbeau < 7)
	{
		pthread_mutex_lock(&mutexGrilleJeu);

		if(grilleJeu[2][positionCourbeau + 1].type == DKJR)
		{
			printf("MEGA L BY COURBEAU\n");

			kill(getpid(), SIGINT); //envoyer le signal

			pthread_mutex_unlock(&mutexGrilleJeu);

			//pour faire sortir du boucle
			break;
		}
		else
		{
			setGrilleJeu(2, positionCourbeau);
	 		effacerCarres(9, (positionCourbeau * 2) + 8, 2, 1);

	 		positionCourbeau++;

	 		*PosCourb = positionCourbeau;
	 		pthread_setspecific(keySpec, PosCourb);

	 		setGrilleJeu(2, positionCourbeau, CORBEAU, pthread_self());
	 		afficherCorbeau((positionCourbeau * 2) + 8, ((positionCourbeau - 1) % 2) + 1);

	 		//afficherGrilleJeu();

	 		pthread_mutex_unlock(&mutexGrilleJeu);

	 		nanosleep(&requete, &rest);
		}
	}

	setGrilleJeu(2, positionCourbeau);
 	effacerCarres(9, (positionCourbeau * 2) + 8, 2, 2);

	pthread_exit(NULL);
}

// -------------------------------------

void* FctThreadCroco(void *)
{
	printf("Thread Croco Cree: %d.%u\n", getpid(), pthread_self());

	sigset_t maskCroco;
	sigfillset(&maskCroco);
	sigdelset(&maskCroco,SIGUSR2); //pour permettre le thread courbeau de recevoir le signal SIGUSR1

	// Mise en place du nouveau masque de signal
	sigprocmask(SIG_SETMASK,&maskCroco,NULL);

	/*---------------------------------------*/

	struct timespec rest, requete = { 0, 700000000 };
	S_CROCO croc;

	//creation du memoire pour notre cle
	S_CROCO *PosCroco = (S_CROCO *)malloc(sizeof(S_CROCO));

	croc.haut = true;
	croc.position = 2;

	setGrilleJeu(1, croc.position, CROCO, pthread_self());
	afficherCroco(11, 2);

	*PosCroco = croc;
 	pthread_setspecific(keySpec, PosCroco);

	nanosleep(&requete, &rest);

	while(1)
	{
		if(croc.haut)
		{
			pthread_mutex_lock(&mutexGrilleJeu);

			if(grilleJeu[1][croc.position+1].type == DKJR)
			{
				kill(getpid(), SIGHUP);

				setGrilleJeu(1, croc.position);
				effacerCarres(8, (croc.position * 2) + 7, 1, 1);

				pthread_mutex_unlock(&mutexGrilleJeu);

				pthread_exit(NULL);

			}
			else
			{
				if(croc.position < 7)
				{
					setGrilleJeu(1, croc.position);
					effacerCarres(8, (croc.position * 2) + 7, 1, 1);

					croc.position++;

					*PosCroco = croc;
	 				pthread_setspecific(keySpec, PosCroco);

					setGrilleJeu(1, croc.position, CROCO, pthread_self());
					afficherCroco((croc.position * 2) + 7, ((croc.position - 1) % 2) + 1);

					pthread_mutex_unlock(&mutexGrilleJeu);

					nanosleep(&requete, &rest);
				}
				else
				{
					setGrilleJeu(1, croc.position);
					effacerCarres(8, (croc.position * 2) + 7, 1, 1);

					croc.position++;
					croc.haut = false;

					setGrilleJeu(2, croc.position, CROCO, pthread_self());
					afficherCroco(23, 3);

					pthread_mutex_unlock(&mutexGrilleJeu);

					nanosleep(&requete, &rest);
				}
			}
		}
		else
		{
			if(croc.position > 1)
			{
				pthread_mutex_lock(&mutexGrilleJeu);

				if(grilleJeu[3][croc.position-1].type == DKJR)
				{
					kill(getpid(), SIGCHLD);

					if(croc.position == 8)
					{
						croc.position--;
						setGrilleJeu(2, croc.position);
						effacerCarres(9, 23, 1, 1);
					}
					else
					{
						setGrilleJeu(3, croc.position);
						effacerCarres(12, (croc.position * 2) + 8, 1, 1);
					}

					pthread_mutex_unlock(&mutexGrilleJeu);

					break;

				}
				else
				{
					if(croc.position == 8)
					{
						//printf("hola croco\n");

						setGrilleJeu(2, croc.position);
						effacerCarres(9, 23, 1, 1);

						croc.position--;

						*PosCroco = croc;
	 					pthread_setspecific(keySpec, PosCroco);

						setGrilleJeu(3, croc.position, CROCO, pthread_self());
						afficherCroco((croc.position * 2) + 8, 4);

						pthread_mutex_unlock(&mutexGrilleJeu);

						nanosleep(&requete, &rest);
					}
					else
					{
						setGrilleJeu(3, croc.position);
						effacerCarres(12, (croc.position * 2) + 8, 1, 1);

						croc.position--;

						*PosCroco = croc;
	 					pthread_setspecific(keySpec, PosCroco);

						setGrilleJeu(3, croc.position, CROCO, pthread_self());
						afficherCroco((croc.position * 2) + 8, ((croc.position - 1) % 2) + 4);

						pthread_mutex_unlock(&mutexGrilleJeu);

						nanosleep(&requete, &rest);
					}
				}
				
			}
			else break;
		}
	}

	setGrilleJeu(3, croc.position);
	effacerCarres(12, (croc.position * 2) + 8, 1, 1);

	pthread_exit(NULL);
}

// -------------------------------------
// DESTRUCTEUR

void DestructeurVS(void *p)
{
	printf("LIBERATION DU MEMOIRE POUR LE THREAD %d.%u\n", getpid(), pthread_self());
	free(p);
}

// -------------------------------------
//LES SIGNAUX

void HandlerSIGQUIT(int)
{
	//printf("positionDKJr = %d; etatDKJr = %d\n", positionDKJr, etatDKJr);
	//printf("Recu par: %d.%u\n", getpid(), pthread_self());
}

// -------------------------------------

void HandlerSIGALRM(int)
{
	printf("SIGALRM RECU PAR %d.%u!\n", getpid(), pthread_self());

	pthread_mutex_lock(&mutexDelai);

	delaiEnnemis = delaiEnnemis - 250;

	printf("Delai Ennemis = %d\n", delaiEnnemis);

	if(delaiEnnemis > 2500) alarm(15);

	pthread_mutex_unlock(&mutexDelai);
}

// -------------------------------------

void HandlerSIGUSR1(int)
{
	printf("SIGUSR1 RECU PAR %d.%u!\n", getpid(), pthread_self());

	pthread_mutex_lock(&mutexGrilleJeu);

	int *posC = (int*)pthread_getspecific(keySpec);

	setGrilleJeu(2, *posC);
	effacerCarres(9, (*posC * 2) + 8, 2, 1);

	pthread_mutex_unlock(&mutexGrilleJeu);

	pthread_exit(NULL);
}

// -------------------------------------

void HandlerSIGINT(int)
{
	printf("SIGINT RECU PAR %d.%u!\n", getpid(), pthread_self());

	pthread_mutex_lock(&mutexGrilleJeu);

	setGrilleJeu(2, positionDKJr);
	effacerCarres(10, (positionDKJr * 2) + 7, 2, 2);

	pthread_mutex_unlock(&mutexGrilleJeu);

 	pthread_mutex_unlock(&mutexEvenement);

 	pthread_exit(NULL);
}

// -------------------------------------

void HandlerSIGUSR2(int)
{
	printf("SIGUSR2 RECU PAR %d.%u!\n", getpid(), pthread_self());

	pthread_mutex_lock(&mutexGrilleJeu);

	S_CROCO *posCr = (S_CROCO*)pthread_getspecific(keySpec);

	if(posCr->haut)
	{
		setGrilleJeu(1, posCr->position);
		effacerCarres(8, (posCr->position * 2) + 7, 1, 1);
	}
	else
	{
		setGrilleJeu(3, posCr->position);
		effacerCarres(12, (posCr->position * 2) + 8, 1, 1);
	}

	pthread_mutex_unlock(&mutexGrilleJeu);

	pthread_exit(NULL);
}

// -------------------------------------

void HandlerSIGHUP(int)
{
	printf("SIGHUP RECU PAR %d.%u!\n", getpid(), pthread_self());

	pthread_mutex_lock(&mutexGrilleJeu);

	setGrilleJeu(1, positionDKJr);
	effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);

	pthread_mutex_unlock(&mutexGrilleJeu);
	pthread_mutex_unlock(&mutexEvenement);

	pthread_exit(NULL);
}

// -------------------------------------

void HandlerSIGCHLD(int)
{
	printf("SIGCHLD RECU PAR %d.%u!\n", getpid(), pthread_self());

	pthread_mutex_lock(&mutexGrilleJeu);

	setGrilleJeu(3, positionDKJr);
	effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);

	pthread_mutex_unlock(&mutexGrilleJeu);
	pthread_mutex_unlock(&mutexEvenement);

	pthread_exit(NULL);
}