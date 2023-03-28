#include <pthread.h> 
#include <stdio.h> 
#include <stdlib.h> 

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#include <string.h>

#define FICHIER "Serveur.cpp"

void * fctThread(void * param); 

pthread_t threadHandle; 

int main(void)
{
	int ret, * retThread; 
	
	puts("Thread principal demarre"); 

	ret = pthread_create(&threadHandle, NULL, (void *(*) (void *))fctThread, NULL);
	
	puts("Thread secondaire lance !"); 

	puts("Attente de la fin du thread secondaire"); 
	
	ret = pthread_join(threadHandle, (void **)&retThread); 

	printf("Valeur renvoyee par le thread secondaire = %d\n", *retThread); 

	puts("Fin du thread principal");
}

void * fctThread (void * param) 
{ 
	int * cpt = (int *)malloc(sizeof(int)); 
	*cpt = 0;

	char cible[] = "affiche", buffer[8];
	printf("%s\n", cible);

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

	printf("taille = %d\n", taille);

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

		if ((rc = read(fd,buffer,7)) == -1)
		{
			perror("Erreur de read");
			exit(1);
		}
		buffer[rc] = '\0'; // sinon buffer ne contient pas une chaine de caracteres

		if(strcmp(buffer, cible) == 0) //si buffer egal a cible alors ca envoie 0
		{
			(*cpt)++; //augmente le compteur
			printf("*\n");
		}

		close(fd);
	}
	
	pthread_exit(cpt); //pour renvoyer au thread principal
	
	return 0; 
} 