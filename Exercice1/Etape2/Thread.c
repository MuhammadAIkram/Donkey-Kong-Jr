#include <pthread.h> 
#include <stdio.h> 
#include <stdlib.h> 

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#include <string.h>

#define FICHIER "Serveur.cpp"

void * fctThread(void * param); 
void * fctThread2(void * param); 
void * fctThread3(void * param); 
void * fctThread4(void * param); 

pthread_t threadHandle; 
pthread_t threadHandle2;
pthread_t threadHandle3;
pthread_t threadHandle4;

int main(void)
{
	int ret, *retThread, *retThread2, *retThread3, *retThread4; 
	
	puts("Thread principal demarre"); 

	ret = pthread_create(&threadHandle, NULL, (void *(*) (void *))fctThread, NULL);
	puts("Thread secondaire lance !");

	ret = pthread_create(&threadHandle2, NULL, (void *(*) (void *))fctThread2, NULL);
	puts("Troisieme Thread lance !");

	ret = pthread_create(&threadHandle3, NULL, (void *(*) (void *))fctThread3, NULL);
	puts("Quatrieme Thread lance !");

	ret = pthread_create(&threadHandle4, NULL, (void *(*) (void *))fctThread4, NULL); 
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

	char cible[] = "affiche", buffer[8];
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

		if ((rc = read(fd,buffer,7)) == -1)
		{
			perror("Erreur de read");
			exit(1);
		}
		buffer[rc] = '\0'; // sinon buffer ne contient pas une chaine de caracteres

		if(strcmp(buffer, cible) == 0) //si buffer egal a cible alors ca envoie 0
		{
			(*cpt)++; //augmente le compteur
			puts("*");
		}

		close(fd);
	}
	
	pthread_exit(cpt); //pour renvoyer au thread principal
	
	return 0; 
} 


void * fctThread2 (void * param) 
{ 
	int * cpt = (int *)malloc(sizeof(int)); 
	*cpt = 0;

	char cible[] = "fdPipe", buffer[7];
	//printf("Mot rechercher par thread 3: %s\n", cible);

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

		if ((rc = read(fd,buffer,6)) == -1)
		{
			perror("Erreur de read");
			exit(1);
		}
		buffer[rc] = '\0'; // sinon buffer ne contient pas une chaine de caracteres

		if(strcmp(buffer, cible) == 0) //si buffer egal a cible alors ca envoie 0
		{
			(*cpt)++; //augmente le compteur
			puts("\t*");
		}

		close(fd);
	}
	
	pthread_exit(cpt); //pour renvoyer au thread principal
	
	return 0; 
} 

void * fctThread3(void * param)
{
	int * cpt = (int *)malloc(sizeof(int)); 
	*cpt = 0;

	char cible[] = "tab", buffer[4];
	//printf("Mot rechercher par thread 4: %s\n", cible);

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

		if ((rc = read(fd,buffer,3)) == -1)
		{
			perror("Erreur de read");
			exit(1);
		}
		buffer[rc] = '\0'; // sinon buffer ne contient pas une chaine de caracteres

		if(strcmp(buffer, cible) == 0) //si buffer egal a cible alors ca envoie 0
		{
			(*cpt)++; //augmente le compteur
			puts("\t\t*");
		}

		close(fd);
	}
	
	pthread_exit(cpt); //pour renvoyer au thread principal
	
	return 0; 
}

void * fctThread4(void * param)
{
	int * cpt = (int *)malloc(sizeof(int)); 
	*cpt = 0;

	char cible[] = "handlerSIGINT", buffer[14];
	//printf("Mot rechercher par thread 5: %s\n", cible);

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

		if ((rc = read(fd,buffer,13)) == -1)
		{
			perror("Erreur de read");
			exit(1);
		}
		buffer[rc] = '\0'; // sinon buffer ne contient pas une chaine de caracteres

		if(strcmp(buffer, cible) == 0) //si buffer egal a cible alors ca envoie 0
		{
			(*cpt)++; //augmente le compteur
			puts("\t\t\t*");
		}

		close(fd);
	}
	
	pthread_exit(cpt); //pour renvoyer au thread principal
	
	return 0; 
}