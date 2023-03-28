#include <pthread.h> 
#include <stdio.h> 
#include <stdlib.h> 

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#include <string.h>

#define FICHIER "Serveur.cpp"

struct motrechercher
{
	char nomFichier[20];
	char mot[20];
	int nbrTab;
}typedef MOTRECHERCHER;

void * fctThread(void * param); 

pthread_t threadHandle; 
pthread_t threadHandle2;
pthread_t threadHandle3;
pthread_t threadHandle4;

int main(void)
{
	puts("Thread principal demarre");

	int ret, *retThread, *retThread2, *retThread3, *retThread4; 

	MOTRECHERCHER T1, T2, T3, T4;

	T1.nbrTab = 0;
	strcpy(T1.nomFichier, FICHIER);
	printf("Encoder le mot rechercher pour le deuxieme thread: ");
	scanf("%s", T1.mot);

	T2.nbrTab = 1;
	strcpy(T2.nomFichier, FICHIER);
	printf("Encoder le mot rechercher pour le troisieme thread: ");
	scanf("%s", T2.mot); 

	T3.nbrTab = 2;
	strcpy(T3.nomFichier, FICHIER);
	printf("Encoder le mot rechercher pour le quatrieme thread: ");
	scanf("%s", T3.mot); 

	T4.nbrTab = 3;
	strcpy(T4.nomFichier, FICHIER);
	printf("Encoder le mot rechercher pour le quatrieme thread: ");
	scanf("%s", T4.mot); 

	ret = pthread_create(&threadHandle, NULL, (void *(*) (void *))fctThread, (void *)&T1);
	puts("Thread secondaire lance !");

	ret = pthread_create(&threadHandle2, NULL, (void *(*) (void *))fctThread, (void *)&T2);
	puts("Troisieme Thread lance !");

	ret = pthread_create(&threadHandle3, NULL, (void *(*) (void *))fctThread, (void *)&T3);
	puts("Quatrieme Thread lance !");

	ret = pthread_create(&threadHandle4, NULL, (void *(*) (void *))fctThread, (void *)&T4); 
	puts("Cinquieme Thread lance !");

	printf("\n");

	//puts("Attente de la fin du thread secondaire"); 
	
	ret = pthread_join(threadHandle, (void **)&retThread);  
	ret = pthread_join(threadHandle2, (void **)&retThread2); 
	ret = pthread_join(threadHandle3, (void **)&retThread3);
	ret = pthread_join(threadHandle4, (void **)&retThread4);

	printf("\n\n");
	printf("Voici les valeurs des differents threads: \n");
	printf("-------------------------------------------------------------\n");
	printf("Valeur renvoyee par le thread secondaire = %d\n", *retThread);
	printf("Valeur renvoyee par le troisieme thread = %d\n", *retThread2); 
	printf("Valeur renvoyee par le quatrieme thread = %d\n", *retThread3);
	printf("Valeur renvoyee par le cinquieme thread = %d\n", *retThread4);
	printf("-------------------------------------------------------------\n");

	//pour liberer la memoire
	free(retThread);
	free(retThread2);
	free(retThread3);
	free(retThread4);

	puts("Fin du thread principal");
}

void * fctThread (void * param) 
{ 
	int * cpt = (int *)malloc(sizeof(int)); 
	*cpt = 0;

	MOTRECHERCHER *T = (MOTRECHERCHER *)param;

	//printf("nomFichier = %s\n", T->nomFichier);
	//printf("nbrTab = %d\n", T->nbrTab);
	//printf("Mot Rechercher = %s\n", T->mot);

	char cible[20], buffer[20];

	strcpy(cible, T->mot);

	int tailleMot = strlen(cible);

	//printf("Mot rechercher par thread 2: %s\n", cible);

	int fd;

	if ((fd = open(FICHIER, O_RDONLY)) == -1)
	{
		perror("Erreur de open()");
		exit(1);
	}

	int rc, taille, i = 0;

	if ((rc = lseek(fd,0,SEEK_END)) == -1)
	{
		perror("Erreur de lseek");
		exit(1);
	}

	taille = rc;

	//printf("taille = %d\n", taille);

	if ((rc = lseek(fd,0,SEEK_SET)) == -1)
	{
		perror("Erreur de lseek");
		exit(1);
	}

	close(fd);

	for (i = 0; i < taille; i++)
	{
		if ((fd = open(FICHIER, O_RDONLY)) == -1)
		{
			perror("Erreur de open()");
			exit(1);
		}

		if ((rc = lseek(fd,i,SEEK_SET)) == -1)
		{
			perror("Erreur de lseek");
			exit(1);
		}

		if ((rc = read(fd,buffer,tailleMot)) == -1)
		{
			perror("Erreur de read");
			exit(1);
		}
		
		buffer[rc] = '\0'; // sinon buffer ne contient pas une chaine de caracteres

		if(strcmp(buffer, cible) == 0) //si buffer egal a cible alors ca envoie 0
		{
			(*cpt)++; //augmente le compteur

			for(int j = 0; j < T->nbrTab; j++) printf("\t");

			puts("*");
		}

		close(fd);
	}
	
	pthread_exit(cpt); //pour renvoyer au thread principal
	
	return 0; 
} 

