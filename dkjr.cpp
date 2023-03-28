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

pthread_key_t keySpec;

bool MAJDK = false;
int  score = 0;
bool MAJScore = true;
int  delaiEnnemis = 4000;
int  positionDKJr = 1;
int  evenement = AUCUN_EVENEMENT;
int etatDKJr;

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

	//masquage du signal sigquit
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask,SIGQUIT);

	// Mise en place du nouveau masque de signal
	sigprocmask(SIG_SETMASK,&mask,NULL);

	//initialiser mutex evenement
	pthread_mutex_init(&mutexEvenement, NULL);

	//initialiser mutex GrilleJeu
	pthread_mutex_init(&mutexGrilleJeu, NULL);

	//initialiser mutex DK et cond DK
	pthread_mutex_init(&mutexDK, NULL);
	pthread_cond_init(&condDK, NULL);

	//commencer le thread cle
	pthread_create(&threadCle, NULL, (void *(*) (void *))FctThreadCle, NULL);

	//commencer le thread evenement
	pthread_create(&threadEvenements, NULL, (void *(*) (void *))FctThreadEvenements, NULL);

	//commencer le thread donkey kong
	pthread_create(&threadDK, NULL, (void *(*) (void *))FctThreadDK, NULL);

	int vie = 1;

	while(vie <= 3)
	{
		//commencer le thread DKJr
		pthread_create(&threadDKJr, NULL, (void *(*) (void *))FctThreadDKJr, NULL);

		pthread_join(threadDKJr, NULL);

		afficherEchec(vie);

		vie++;
	}

	pause();
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

		// if(i == 1) 
		// {
		// 	setGrilleJeu(0,1,CLE,threadCle); //grilleJeu[0][1].type = CLE;
		// }
		// else
		// {
		// 	setGrilleJeu(0,1,VIDE,threadCle); //grilleJeu[0][1].type = VIDE;
		// }

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
	printf("DKJR = %d.%u\n", getpid(), pthread_self());

	sigset_t mask;
	sigemptyset(&mask);
	sigdelset(&mask,SIGQUIT); //pour permettre le thread dkjr de recoivoir le signal SIGQUIT

	// Mise en place du nouveau masque de signal
	sigprocmask(SIG_SETMASK,&mask,NULL);

	/*---------------------------------------*/

	bool on = true; 
	struct timespec rest, requete;

	pthread_mutex_lock(&mutexGrilleJeu);
	 
	setGrilleJeu(3, 1, DKJR); 
	afficherDKJr(11, 9, 1); 
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

						 		//effacer jr en train de sauter
						 		setGrilleJeu(2, positionDKJr);
						 		effacerCarres(10, (positionDKJr * 2) + 7, 2, 2);

						 		//lui afficher sur le sol
						 		setGrilleJeu(3, positionDKJr, DKJR);
						 		afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);

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

						 		//effacer jr en train de sauter
						 		setGrilleJeu(0, positionDKJr);
						 		effacerCarres(6, (positionDKJr * 2) + 7, 2, 2);

						 		//lui afficher sur le sol
						 		setGrilleJeu(1, positionDKJr, DKJR);
						 		afficherDKJr(7, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
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

						 	if (positionDKJr >= 3)
						 	{
						 		setGrilleJeu(1, positionDKJr);
						 		effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);

						 		positionDKJr--;

						 		if(positionDKJr > 3)
						 		{
						 			setGrilleJeu(1, positionDKJr, DKJR);
						 			afficherDKJr(7, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
						 		}
						 		else
						 		{
						 			setGrilleJeu(0, positionDKJr, DKJR);
						 			afficherDKJr(7, (positionDKJr * 2) + 7, 9);

						 			requete = { 0, 500000000 };

						 			if(grilleJeu[0][1].type == CLE)
						 			{
						 				printf("CLE CAPTURER\n");

						 				//faire le sleep
					 					nanosleep(&requete, &rest);

					 					//efface jr en train de sauter pour le cle
							 			setGrilleJeu(0, positionDKJr);
							 			effacerCarres(5, (2 * 2) + 7, 3, 3);
							 			afficherCage(4);

							 			//afficher jr avec le cle
							 			setGrilleJeu(0, positionDKJr, DKJR);
						 				afficherDKJr(6, (positionDKJr * 2) + 7, 10);

						 				pthread_mutex_lock(&mutexDK);

										MAJDK = true;

										pthread_mutex_unlock(&mutexDK); 

										pthread_cond_signal(&condDK); //pour envoyer le signal

						 				requete = { 0, 400000000 };

						 				nanosleep(&requete, &rest);

						 				//efface jr avec le cle
							 			setGrilleJeu(0, positionDKJr);
							 			effacerCarres(3, (2 * 2) + 7, 3, 2);
							 			afficherCage(4);

							 			//afficher jr heureuse
							 			setGrilleJeu(1, positionDKJr, DKJR);
						 				afficherDKJr(7, (positionDKJr * 2) + 7, 11);

						 				requete = { 0, 400000000 };

						 				nanosleep(&requete, &rest);
						 		
						 				setGrilleJeu(1, positionDKJr);
						 				effacerCarres(6, 10, 2, 3);
						 				

						 				//lui afficher sur le sol
								 		setGrilleJeu(3, 1, DKJR); 
										afficherDKJr(11, 9, 1); 
										etatDKJr = LIBRE_BAS; 
										positionDKJr = 1;
						 			}
						 			else 
						 			{
					 					printf("L\n");

					 					//faire le sleep
					 					nanosleep(&requete, &rest);

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

						 				//efface jr dans les buissons
							 			setGrilleJeu(3, positionDKJr);
							 			effacerCarres(11, 7, 2, 2);

							 			//pour signaler le fin
							 			on = false;
									}
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
							effacerCarres(4,9,2,3);

							afficherRireDK();
							requete = { 0, 700000000 };

							pthread_mutex_lock(&mutexGrilleJeu);
							effacerCarres(4,9,2,3);
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
//LES SIGNAUX

void HandlerSIGQUIT(int)
{
	//printf("positionDKJr = %d; etatDKJr = %d\n", positionDKJr, etatDKJr);
}