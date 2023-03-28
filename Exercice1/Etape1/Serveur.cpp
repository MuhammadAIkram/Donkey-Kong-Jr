#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include "protocole.h" // contient la cle et la structure d'un message

#include "FichierClient.h"
#include "MySemaphores.h"
#include <setjmp.h>

int idQ,idShm,idSem;
int fdPipe[2];
TAB_CONNEXIONS *tab;
int idPub, idCad, ret, IdAcBD;
sigjmp_buf contexte;

void afficheTab();

void handlerSIGINT(int);
void HandlerSIGCHLD(int);

int main()
{
  // Armement des signaux
  // TO DO
  struct sigaction A;
  A.sa_handler = handlerSIGINT;
  A.sa_flags = 0;
  sigemptyset(&A.sa_mask);
  sigaction(SIGINT,&A,NULL);

  struct sigaction B;
  B.sa_handler = HandlerSIGCHLD;
  sigemptyset(&B.sa_mask);
  B.sa_flags = 0;
  sigaction(SIGCHLD,&B,NULL);

  // Creation des ressources
  // Creation de la file de message
  fprintf(stderr,"(SERVEUR %d) Creation de la file de messages\n",getpid());
  if ((idQ = msgget(CLE,IPC_CREAT | IPC_EXCL | 0600)) == -1)  // CLE definie dans protocole.h
  {
    perror("(SERVEUR) Erreur de msgget");
    exit(1);
  }

  // TO BE CONTINUED
  //creation du semaphore
  fprintf(stderr,"(SERVEUR %d) Creation du semaphore\n",getpid());

  if ((idSem = semget(CLE,1,IPC_CREAT | IPC_EXCL | 0600)) == -1)
  {
    perror("Erreur de semget");
    exit(1);
  }

  printf("idSem = %d\n",idSem);

  //initialise le semaphore
  sem_signal(0);

  //creation du fichier
  //fprintf(stderr,"(SERVEUR %d) CREATION DU FICHIER SI CELA N'EST PAS DEJA ETAIT FAIT\n",getpid());

  int fd = open(FICHIER_CLIENTS, O_WRONLY|O_CREAT|O_APPEND, 0664);

  if(fd == -1)
  {
    perror("(SERVEUR) Erreur de open");
    msgctl(idQ,IPC_RMID,NULL);
    exit(1);
  }

  if (close(fd)) {
    perror("Erreur de close()");
  }

  // Creation du pipe
  // TO DO

  fprintf(stderr,"(SERVEUR %d) Creation du pipe\n",getpid());

  if (pipe(fdPipe))
  { 
    perror("Erreur de pipe"); 
    exit(1); 
  }

  printf("(SERVEUR %d) fdPipe[0] = %d fPipe[1] : %d\n",getpid(),fdPipe[0],fdPipe[1]);


  // Initialisation du tableau de connexions
  tab = (TAB_CONNEXIONS*) malloc(sizeof(TAB_CONNEXIONS)); 

  for (int i=0 ; i<6 ; i++)
  {
    tab->connexions[i].pidFenetre = 0;
    strcpy(tab->connexions[i].nom,"");
    tab->connexions[i].pidCaddie = 0;
  }
  tab->pidServeur = getpid();
  tab->pidPublicite = 0;

  // Creation du processus Publicite (étape 2)
  // TO DO
  fprintf(stderr,"(SERVEUR %d) Creation du mémoire partagée\n",getpid());

  if ((idShm = shmget(CLE,51,IPC_CREAT | IPC_EXCL | 0600)) == -1)
  {
    perror("Erreur de shmget");
    exit(1);
  }

  fprintf(stderr,"(SERVEUR %d) Creation du processus publicite\n",getpid());

  idPub = fork();

  if (idPub == -1)
  {
    perror("Erreur de fork(1)");
    exit(1);
  }

  if(!idPub) 
  {
    execl("./Publicite", "Publicite", NULL);
  }

  fprintf(stderr,"(SERVEUR %d) recupere pid du publicite: %d\n",getpid(), idPub);

  tab->pidPublicite = idPub;

  // Creation du processus AccesBD (étape 4)
  // TO DO

  fprintf(stderr,"(SERVEUR %d) Creation du processus AccesBD\n",getpid());

  IdAcBD = fork();

  if (IdAcBD == -1)
  {
    perror("Erreur de fork(1)");
    exit(1);
  }

  if(!IdAcBD) 
  {
    char pipelecteur[2];
    sprintf(pipelecteur, "%d", fdPipe[0]);

    close(fdPipe[1]);
    execl("./AccesBD", "AccesBD", pipelecteur, NULL);
  }

  fprintf(stderr,"(SERVEUR %d) recupere pid du AccesBD: %d\n",getpid(), IdAcBD);

  tab->pidAccesBD = IdAcBD;

  afficheTab();

  MESSAGE m;
  MESSAGE reponse;
  int i;

  // Mise en place d’un point de retour du saut
  if ((ret = sigsetjmp(contexte,1)) != 0)
  {
    printf("\nRetour du saut %d...\n",ret);
  }

  while(1)
  {
  	if(m.requete != UPDATE_PUB) fprintf(stderr,"(SERVEUR %d) Attente d'une requete...\n",getpid());
    
    if (msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),1,0) == -1)
    {
      perror("(SERVEUR) Erreur de msgrcv");
      msgctl(idQ,IPC_RMID,NULL);
      exit(1);
    }

    switch(m.requete)
    {
      case CONNECT :  // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete CONNECT reçue de %d\n",getpid(),m.expediteur);

                      for (i=0 ; i<6; i++)
                      {
                        if(tab->connexions[i].pidFenetre == 0)
                        {
                          tab->connexions[i].pidFenetre = m.expediteur;

                          i = 6;
                        }
                      }

                      break;

      case DECONNECT : // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete DECONNECT reçue de %d\n",getpid(),m.expediteur);

                      for (int i=0 ; i<6; i++)
                      {
                        if(tab->connexions[i].pidFenetre == m.expediteur)
                        { 
                          tab->connexions[i].pidFenetre = 0;
                          
                          i = 6;
                        }
                      }

                      break;
      case LOGIN :    // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete LOGIN reçue de %d : --%d--%s--%s--\n",getpid(),m.expediteur,m.data1,m.data2,m.data3);

                      reponse = m;

                      if(sem_nowait(0) == -1)
                      {
                        reponse.type = reponse.expediteur;
                        reponse.requete = BUSY;

                        if (msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                        {
                          perror("Erreur de msgsnd");
                          exit(1);
                        }

                        kill(reponse.expediteur, SIGUSR1);
                        fprintf(stderr,"(SERVEUR %d) MESSAGE ENVOYER A %d ",getpid(),reponse.expediteur);
                      }
                      else
                      {
                        reponse.type = reponse.expediteur;
                        reponse.requete = LOGIN;

                        int pos, veri;

                        i = 0;

                        while(i < 6 && tab->connexions[i].pidFenetre != reponse.expediteur) i++;

                        if(reponse.data1 == 0)
                        {
                          pos = estPresent(reponse.data2);

                          if(pos == -1)
                          {
                            strcpy(reponse.data4, " ---- ERREUR: Fichier n'existe pas ! ---- ");
                            reponse.data1 = 1;
                          }
                          else
                          {
                            if(pos == 0) 
                            {
                              strcpy(reponse.data4," ---- Client inconnu… ---- ");
                              reponse.data1 = 1;
                            }
                            else
                            {
                              veri = verifieMotDePasse(pos, reponse.data3);

                              if(veri == -1)
                              {
                                strcpy(reponse.data4," ---- ERREUR !!! ---- ");
                                reponse.data1 = 1;
                              }
                              else
                              {
                                if(veri == 0) 
                                {
                                  strcpy(reponse.data4," ---- Mot de passe incorrect… ---- ");
                                  reponse.data1 = 1;
                                }
                                else 
                                {
                                  strcpy(reponse.data4," ----  Re-bonjour cher client ! ---- ");
                                  reponse.data1 = 0;
                                  strcpy(tab->connexions[i].nom, reponse.data2);
                                }
                              }
                            }
                          }
                        }
                        else
                        {
                          pos = estPresent(reponse.data2);

                          if(pos == -1 || pos == 0)
                          {
                            ajouteClient(reponse.data2, reponse.data3);
                            strcpy(reponse.data4," ---- Nouveau client créé : bienvenue ! ---- ");
                            reponse.data1 = 0;
                            strcpy(tab->connexions[i].nom, reponse.data2);
                          }
                          else 
                          {
                            strcpy(reponse.data4," ---- Client déjà existant ! ---- ");
                            reponse.data1 = 1;
                          }
                        }

                        if (msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                        {
                          perror("Erreur de msgsnd");
                          exit(1);
                        }

                        kill(reponse.expediteur, SIGUSR1);
                        fprintf(stderr,"(SERVEUR %d) MESSAGE ENVOYER A %d ",getpid(),reponse.expediteur);

                        sem_signal(0);

                        if(reponse.data1 == 0)
                        {
                          fprintf(stderr,"(SERVEUR %d) Creation du processus Caddie\n", getpid());

                          idCad = fork();

                          if (idCad == -1)
                          {
                            perror("Erreur de fork(1)");
                            exit(1);
                          }

                          if(!idCad) 
                          {
                            char pipeEcriture[2];
                            sprintf(pipeEcriture, "%d", fdPipe[1]);

                            close(fdPipe[0]);
                            execl("./Caddie", "Caddie", pipeEcriture, NULL);
                          }

                          fprintf(stderr,"(SERVEUR %d) recupere pid du caddie: %d\n",getpid(), idCad);

                          tab->connexions[i].pidCaddie = idCad;

                          //envoie un message login

                          reponse.type = idCad;
                          reponse.expediteur = getpid();
                          reponse.requete = LOGIN;

                          if (msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                          {
                              perror("Erreur de msgsnd");
                              exit(1);
                          }
                        }
                      }

                      break; 

      case LOGOUT :   // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete LOGOUT reçue de %d\n",getpid(),m.expediteur);

                      reponse = m;

                      if(sem_nowait(0) == -1)
                      {
                        reponse.type = reponse.expediteur;
                        reponse.requete = BUSY;

                        if (msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                        {
                          perror("Erreur de msgsnd");
                          exit(1);
                        }

                        kill(reponse.expediteur, SIGUSR1);
                        fprintf(stderr,"(SERVEUR %d) MESSAGE ENVOYER A %d ",getpid(),reponse.expediteur);
                      }
                      else
                      {
                        i = 0;

                        while(i < 6 && tab->connexions[i].pidFenetre != m.expediteur) i++;

                        if(i != 6)
                        {
                          if(tab->connexions[i].pidCaddie != 0)
                          {
                            //vide le panier
                            reponse.type = tab->connexions[i].pidCaddie;
                            reponse.expediteur = getpid();
                            reponse.requete = CANCEL_ALL;

                            if (msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                            {
                                perror("Erreur de msgsnd");
                                exit(1);
                            }

                            reponse.type = tab->connexions[i].pidCaddie;
                            reponse.expediteur = getpid();
                            reponse.requete = LOGOUT;

                            if (msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                            {
                              perror("Erreur de msgsnd");
                              exit(1);
                            }
                          }
                        }

                        sem_signal(0);
                      }

                      break;

      case UPDATE_PUB :  // TO DO
                      i = 0;

                      while(i < 6)
                      {
                        if(tab->connexions[i].pidFenetre != 0) kill(tab->connexions[i].pidFenetre, SIGUSR2);
                        i++;
                      }

                      break;

      case CONSULT :  // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete CONSULT reçue de %d\n",getpid(),m.expediteur);

                      if(sem_nowait(0) == -1)
                      {
                        reponse = m;
                      
                        reponse.type = reponse.expediteur;
                        reponse.requete = BUSY;

                        if (msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                        {
                          perror("Erreur de msgsnd");
                          exit(1);
                        }

                        kill(reponse.expediteur, SIGUSR1);
                        fprintf(stderr,"(SERVEUR %d) MESSAGE ENVOYER A %d ",getpid(),reponse.expediteur);
                      }
                      else
                      {
                        i = 0;

                        while(i < 6 && tab->connexions[i].pidFenetre != m.expediteur) i++;

                        reponse.type = tab->connexions[i].pidCaddie;
                        reponse.expediteur = m.expediteur;
                        reponse.data1 = m.data1;
                        reponse.requete = CONSULT;

                        if (msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                        {
                            perror("Erreur de msgsnd");
                            exit(1);
                        }

                        sem_signal(0);
                      }

                      break;

      case ACHAT :    // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete ACHAT reçue de %d\n",getpid(),m.expediteur);

                      if(sem_nowait(0) == -1)
                      {
                        reponse = m;
                      
                        reponse.type = reponse.expediteur;
                        reponse.requete = BUSY;

                        if (msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                        {
                          perror("Erreur de msgsnd");
                          exit(1);
                        }

                        kill(reponse.expediteur, SIGUSR1);
                        fprintf(stderr,"(SERVEUR %d) MESSAGE ENVOYER A %d ",getpid(),reponse.expediteur);
                      }
                      else
                      {
                        i = 0;

                        while(i < 6 && tab->connexions[i].pidFenetre != m.expediteur) i++;

                        reponse.type = tab->connexions[i].pidCaddie;
                        reponse.expediteur = m.expediteur;
                        reponse.data1 = m.data1;
                        strcpy(reponse.data2, m.data2);
                        reponse.requete = ACHAT;

                        if (msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                        {
                            perror("Erreur de msgsnd");
                            exit(1);
                        }

                        sem_signal(0);
                      }

                      break;

      case CADDIE :   // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete CADDIE reçue de %d\n",getpid(),m.expediteur);

                      if(sem_nowait(0) == -1)
                      {
                        reponse = m;
                      
                        reponse.type = reponse.expediteur;
                        reponse.requete = BUSY;

                        if (msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                        {
                          perror("Erreur de msgsnd");
                          exit(1);
                        }

                        kill(reponse.expediteur, SIGUSR1);
                        fprintf(stderr,"(SERVEUR %d) MESSAGE ENVOYER A %d ",getpid(),reponse.expediteur);
                      }
                      else
                      {
                        i = 0;

                        while(i < 6 && tab->connexions[i].pidFenetre != m.expediteur) i++;

                        reponse.type = tab->connexions[i].pidCaddie;
                        reponse.expediteur = m.expediteur;
                        reponse.requete = CADDIE;

                        if (msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                        {
                            perror("Erreur de msgsnd");
                            exit(1);
                        }

                        sem_signal(0);
                      }

                      break;

      case CANCEL :   // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete CANCEL reçue de %d\n",getpid(),m.expediteur);

                      if(sem_nowait(0) == -1)
                      {
                        reponse = m;
                      
                        reponse.type = reponse.expediteur;
                        reponse.requete = BUSY;

                        if (msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                        {
                          perror("Erreur de msgsnd");
                          exit(1);
                        }

                        kill(reponse.expediteur, SIGUSR1);
                        fprintf(stderr,"(SERVEUR %d) MESSAGE ENVOYER A %d ",getpid(),reponse.expediteur);
                      }
                      else
                      {
                        i = 0;

                        while(i < 6 && tab->connexions[i].pidFenetre != m.expediteur) i++;

                        reponse.type = tab->connexions[i].pidCaddie;
                        reponse.expediteur = m.expediteur;
                        reponse.requete = CANCEL;
                        reponse.data1 = m.data1;

                        if (msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                        {
                            perror("Erreur de msgsnd");
                            exit(1);
                        }

                        sem_signal(0);
                      }

                      break;

      case CANCEL_ALL : // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete CANCEL_ALL reçue de %d\n",getpid(),m.expediteur);

                      if(sem_nowait(0) == -1)
                      {
                        reponse = m;
                      
                        reponse.type = reponse.expediteur;
                        reponse.requete = BUSY;

                        if (msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                        {
                          perror("Erreur de msgsnd");
                          exit(1);
                        }

                        kill(reponse.expediteur, SIGUSR1);
                        fprintf(stderr,"(SERVEUR %d) MESSAGE ENVOYER A %d ",getpid(),reponse.expediteur);
                      }
                      else
                      {
                        i = 0;

                        while(i < 6 && tab->connexions[i].pidFenetre != m.expediteur) i++;

                        reponse.type = tab->connexions[i].pidCaddie;
                        reponse.expediteur = m.expediteur;
                        reponse.requete = CANCEL_ALL;

                        if (msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                        {
                            perror("Erreur de msgsnd");
                            exit(1);
                        }

                        sem_signal(0);
                      }

                      break;

      case PAYER : // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete PAYER reçue de %d\n",getpid(),m.expediteur);

                      if(sem_nowait(0) == -1)
                      {
                        reponse = m;
                      
                        reponse.type = reponse.expediteur;
                        reponse.requete = BUSY;

                        if (msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                        {
                          perror("Erreur de msgsnd");
                          exit(1);
                        }

                        kill(reponse.expediteur, SIGUSR1);
                        fprintf(stderr,"(SERVEUR %d) MESSAGE ENVOYER A %d ",getpid(),reponse.expediteur);
                      }
                      else
                      {
                        i = 0;

                        while(i < 6 && tab->connexions[i].pidFenetre != m.expediteur) i++;

                        reponse.type = tab->connexions[i].pidCaddie;
                        reponse.expediteur = m.expediteur;
                        reponse.requete = PAYER;

                        if (msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                        {
                            perror("Erreur de msgsnd");
                            exit(1);
                        }

                        sem_signal(0);
                      }

                      break;

      case NEW_PUB :  // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete NEW_PUB reçue de %d\n",getpid(),m.expediteur);

                      reponse = m;
                      reponse.type = idPub;

                      if (msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                      {
                          perror("Erreur de msgsnd");
                          exit(1);
                      }

                      kill(idPub, SIGUSR1);

                      break;
    }
    if(m.requete != UPDATE_PUB) afficheTab();
  }
}

void afficheTab()
{
  fprintf(stderr,"Pid Serveur   : %d\n",tab->pidServeur);
  fprintf(stderr,"Pid Publicite : %d\n",tab->pidPublicite);
  fprintf(stderr,"Pid AccesBD   : %d\n",tab->pidAccesBD);
  for (int i=0 ; i<6 ; i++)
    fprintf(stderr,"%6d -%20s- %6d\n",tab->connexions[i].pidFenetre,
                                                      tab->connexions[i].nom,
                                                      tab->connexions[i].pidCaddie);
  fprintf(stderr,"\n");
}

void handlerSIGINT(int)
{
  kill(idPub, SIGKILL); //tue le processus publicite

  int i = 0;

  while(i < 0)
  {
    if(tab->connexions[i].pidCaddie != 0)  kill(tab->connexions[i].pidCaddie, SIGKILL); // tue tout les processus caddie si il ne sont pas deja terminer
  }

  close(fdPipe[0]);
  close(fdPipe[1]);

  // Suppression de l’ensemble de semaphores
  if (semctl(idSem,0,IPC_RMID) == -1)
  {
    perror("Erreur de semctl (3)");
    exit(1);
  }

  if(msgctl(idQ,IPC_RMID,NULL) == -1)
  {
    perror("Erreur de msgctl");
    exit(1);
  }

  if(shmctl(idShm,IPC_RMID,NULL) == -1)
  {
    perror("Erreur de shmctl");
    exit(1);
  }

  exit(0);
}

void HandlerSIGCHLD(int)
{
  int id, status, i;
  id = wait(&status);

  fprintf(stderr, "\n(SERVEUR) Suppression du fils %d de la table des processus\n", id);
  
  if (WIFEXITED(status))
  {
    //printf("\nexit = %d \n", WEXITSTATUS(status));

    i = 0;

    while(i < 6 && tab->connexions[i].pidCaddie != id) i++;
    
    if(i != 6)
    {
      tab->connexions[i].pidCaddie = 0;
      strcpy(tab->connexions[i].nom,"");
      afficheTab();
    }
  }

  siglongjmp(contexte, id);
}

