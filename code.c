#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h> 

// CONSTANTES
#define N 4        
#define M1 3       
#define M2 4      
#define N1 4      
#define N2 5      



typedef struct {
    long mtype;         
    int num_malade; 
    int num_organe;
} StructureDemande; 

typedef struct {
     long mtype;        
     int type_malade; 
     int num_malade;
     int num_organe;
} StructureReponse;

typedef struct {
    int type_malade;
    int num_malade;
    int num_organe;
} ElementTampon;

typedef struct {
    int cpt;      
    int tete;      
    int queue;     
    ElementTampon buffer[N]; 
} MemoirePartagee;




int semid, Qcr, Qncr, Qimp, shmid;
int tube[2];
MemoirePartagee *Torgane; 


int Agenerer() { 
    return rand() % 2; 
}

void P(int sem_num) {
    struct sembuf op = {sem_num, -1, 0};
    semop(semid, &op, 1);
}

void V(int sem_num) {
    struct sembuf op = {sem_num, 1, 0};
    semop(semid, &op, 1);
}

// --- INITIALISATION 

void Creer_et_initialiser_semaphores(int *nv, int *mutex) {
    key_t key = ftok(".", 'S');
    semid = semget(key, 2, IPC_CREAT | 0666);
    semctl(semid, 0, SETVAL, N); 
    semctl(semid, 1, SETVAL, 1); 
    *nv = 0; *mutex = 1;
}

void Creer_files_messages(int *pQcr, int *pQncr, int *pQimp) {
    key_t k1 = ftok(".", 'A'), k2 = ftok(".", 'B'), k3 = ftok(".", 'C');

    Qcr = msgget(k1, IPC_CREAT | IPC_EXCL | 0666);
    if (Qcr == -1 && errno == EEXIST) {
        Qcr = msgget(k1, 0); msgctl(Qcr, IPC_RMID, NULL);
        Qcr = msgget(k1, IPC_CREAT | 0666);
    }
    Qncr = msgget(k2, IPC_CREAT | IPC_EXCL | 0666);
    if (Qncr == -1 && errno == EEXIST) {
        Qncr = msgget(k2, 0); msgctl(Qncr, IPC_RMID, NULL);
        Qncr = msgget(k2, IPC_CREAT | 0666);
    }
    Qimp = msgget(k3, IPC_CREAT | IPC_EXCL | 0666);
    if (Qimp == -1 && errno == EEXIST) {
        Qimp = msgget(k3, 0); msgctl(Qimp, IPC_RMID, NULL);
        Qimp = msgget(k3, IPC_CREAT | 0666);
    }
    *pQcr = Qcr; *pQncr = Qncr; *pQimp = Qimp;
}

void Creer_et_attacher_tampon(MemoirePartagee **pTorgane) {
    key_t key = ftok(".", 'M');
    shmid = shmget(key, sizeof(MemoirePartagee), IPC_CREAT | 0666); 
    Torgane = (MemoirePartagee *)shmat(shmid, NULL, 0);
    Torgane->cpt = 0; Torgane->tete = 0; Torgane->queue = 0;
    *pTorgane = Torgane;
}

void Creer_tube(int p_tube[2]) {
    if (pipe(tube) == -1) exit(1);
    p_tube[0] = tube[0]; p_tube[1] = tube[1];
}

// --- DESTRUCTION ---
void Detruire_semaphores(int nv, int mutex) { semctl(semid, 0, IPC_RMID); }
void Detruire_files_messages(int a, int b, int c) { 
    msgctl(Qcr, IPC_RMID, NULL); msgctl(Qncr, IPC_RMID, NULL); msgctl(Qimp, IPC_RMID, NULL); 
}
void Detruire_tampon(MemoirePartagee *t) { shmdt(t); shmctl(shmid, IPC_RMID, NULL); }
void Detruire_tube(int t[2]) { close(tube[0]); close(tube[1]); }

// --- PROCESSUS ---

void MaladeCr() {
    StructureDemande mess1; mess1.mtype = 1;
    StructureReponse mess2;
    int i = 0; 
    int nb_rep = 0, j;
    
    
    int liste[M1];
    for(int k=0; k<M1; k++) liste[k] = 0;
    
    size_t taille_demande = sizeof(StructureDemande) - sizeof(long);
    size_t taille_reponse = sizeof(StructureReponse) - sizeof(long);

    srand(getpid());
    while(nb_rep < M1) {
        if (i < M1 && Agenerer()) {
            j = (rand() % N1) + 1;
            mess1.num_malade = i; 
            mess1.num_organe = j;

            msgsnd(Qcr, &mess1, taille_demande, 0);
            

            liste[i] = j;
            
            printf("\033[32m[PID %d] (Malade CRITIQUE %d) : Demande envoyee -> Organe %d\033[0m\n", getpid(), i, j);
            i++; 
        }

        if (msgrcv(Qimp, &mess2, taille_reponse, 0, IPC_NOWAIT) != -1) {

            if (mess2.type_malade == 1 && liste[mess2.num_malade] == mess2.num_organe) {
                liste[mess2.num_malade] = 0; 
                nb_rep++;
                printf("\033[32m[PID %d] (Malade CRITIQUE %d) : SUCCES ! Recu Organe %d (Total: %d/%d)\033[0m\n", getpid(), mess2.num_malade, mess2.num_organe, nb_rep, M1);
            } else if (mess2.type_malade != 1) {
                msgsnd(Qimp, &mess2, taille_reponse, 0);
            }
        }
        usleep(30000);
    }
    exit(0);
}

void MaladeNCr() {
    StructureDemande mess1; mess1.mtype = 1;
    StructureReponse mess2;
    int i = 0;
    int nb_rep = 0, j;
    
    int liste[M2];
    for(int k=0; k<M2; k++) liste[k] = 0;
    
    size_t taille_demande = sizeof(StructureDemande) - sizeof(long);
    size_t taille_reponse = sizeof(StructureReponse) - sizeof(long);

    srand(getpid() + 1);
    while(nb_rep < M2) {
        if (i < M2 && Agenerer()) {
            j = (rand() % N2) + 1;
            mess1.num_malade = i;
            mess1.num_organe = j;

            msgsnd(Qncr, &mess1, taille_demande, 0);
            liste[i] = j;
            
            printf("\033[36m[PID %d] (Malade NON-CRIT %d) : Demande envoyee -> Organe %d\033[0m\n", getpid(), i, j);
            i++;
        }
        
        if (msgrcv(Qimp, &mess2, taille_reponse, 0, IPC_NOWAIT) != -1) {
            if (mess2.type_malade == 2 && liste[mess2.num_malade] == mess2.num_organe) {
                liste[mess2.num_malade] = 0;
                nb_rep++;
                printf("\033[36m[PID %d] (Malade NON-CRIT %d) : SUCCES ! Recu Organe %d (Total: %d/%d)\033[0m\n", getpid(), mess2.num_malade, mess2.num_organe, nb_rep, M2);
            } else if (mess2.type_malade != 2) { 
                msgsnd(Qimp, &mess2, taille_reponse, 0); 
            }
        }
        usleep(30000);
    }
    exit(0);
}

void Chirurgien() {
    StructureDemande mess1;
    StructureReponse mess2;
    ElementTampon tube_buf;
    
    int i1 = 0, i2 = 0;
    int nb_rep = 0;
    
    size_t taille_demande = sizeof(StructureDemande) - sizeof(long);
    size_t taille_reponse = sizeof(StructureReponse) - sizeof(long);
    size_t taille_tube = sizeof(ElementTampon);

    while(nb_rep < M1 + M2) {
        if (i1 < M1 && msgrcv(Qcr, &mess1, taille_demande, 0, IPC_NOWAIT) != -1) {
            i1++;
            printf("\033[33m[PID %d] (CHIRURGIEN) : Reçoit demande CRITIQUE (M%d, O%d) -> Transfert \033[0m\n", getpid(), mess1.num_malade, mess1.num_organe);
            
            tube_buf.type_malade = 1;
            tube_buf.num_malade = mess1.num_malade;
            tube_buf.num_organe = mess1.num_organe;
            write(tube[1], &tube_buf, taille_tube);
        }

        if (i2 < M2 && msgrcv(Qncr, &mess1, taille_demande, 0, IPC_NOWAIT) != -1) {
            i2++;
            printf("\033[33m[PID %d] (CHIRURGIEN) : Reçoit demande NON-CRIT (M%d, O%d) -> Transfert\033[0m\n", getpid(), mess1.num_malade, mess1.num_organe);

            tube_buf.type_malade = 2;
            tube_buf.num_malade = mess1.num_malade;
            tube_buf.num_organe = mess1.num_organe;
            write(tube[1], &tube_buf, taille_tube);
        }

        P(1); // P(mutex)
        if (Torgane->cpt > 0) {
            
            V(1);
            
            //PRELEVER
            mess2.mtype = 1;
            mess2.type_malade = Torgane->buffer[Torgane->tete].type_malade;
            mess2.num_malade  = Torgane->buffer[Torgane->tete].num_malade;
            mess2.num_organe  = Torgane->buffer[Torgane->tete].num_organe;
            
            Torgane->tete = (Torgane->tete + 1) % N;


            P(1); 
            Torgane->cpt--;
            V(1); // V(mutex)
            
            V(0); // V(nvide)

            printf("\033[33m[PID %d] (CHIRURGIEN) : OPERATION REUSSIE  Organe %d  pour le Malade %d (Type %d)\033[0m\n", getpid(), mess2.num_organe, mess2.num_malade, mess2.type_malade);

            msgsnd(Qimp, &mess2, taille_reponse, 0);
            nb_rep++;
        }
        else {
            V(1); 
        }
        usleep(5000);
    }
    printf("CHIRURGIEN FINI\n");
    exit(0);
}




void Donneur() {
    
    int Tcr[M1]; 
    int Tncr[M2];
    
    for(int k=0; k<M1; k++) Tcr[k] = 0;
    for(int k=0; k<M2; k++) Tncr[k] = 0;
    
    ElementTampon mess2;
    int i = 0, nb_rep = 0;
    size_t taille_tube = sizeof(ElementTampon);

    srand(getpid() + 2);
    
    while(nb_rep < M1 + M2) {
        if (i < M1 + M2) {
            ssize_t lu = read(tube[0], &mess2, taille_tube);
            if (lu > 0) {
                printf("\033[35m[PID %d] (DONNEUR)    : Ordre reçu -> Fabriquer Organe %d pour Malade %d\033[0m\n", getpid(), mess2.num_organe, mess2.num_malade);

                if (mess2.type_malade == 1) {
                    Tcr[mess2.num_malade] = mess2.num_organe; 
                } else {
                    Tncr[mess2.num_malade] = mess2.num_organe;
                }
                i++;
            }
        }
        
        int Tmalade = (rand() % 2) + 1; 
        int Nmalade, Norgane;
        
        if (Tmalade == 1) { // Cas Critique
            Nmalade = rand() % M1; 
            Norgane = (rand() % N1) + 1;


            if (Tcr[Nmalade] == Norgane) {
                P(0); 
                
                Torgane->buffer[Torgane->queue].type_malade = 1;
                Torgane->buffer[Torgane->queue].num_malade = Nmalade;
                Torgane->buffer[Torgane->queue].num_organe = Norgane;
                Torgane->queue = (Torgane->queue + 1) % N;
                
                P(1); Torgane->cpt++; V(1);
                
                printf("\033[35m[PID %d] (DONNEUR)    : Organe FABRIQUE et DEPOSE au frigo (Pour Critique M%d)\033[0m\n", getpid(), Nmalade);

                nb_rep++;
                Tcr[Nmalade] = 0; 
            }
        }
        else { 
            Nmalade = rand() % M2; 
            Norgane = (rand() % N2) + 1;

            if (Tncr[Nmalade] == Norgane) {
                P(0); 
                
                Torgane->buffer[Torgane->queue].type_malade = 2;
                Torgane->buffer[Torgane->queue].num_malade = Nmalade;
                Torgane->buffer[Torgane->queue].num_organe = Norgane;
                Torgane->queue = (Torgane->queue + 1) % N;
                
                P(1); Torgane->cpt++; V(1);
                
                printf("\033[35m[PID %d] (DONNEUR)    : Organe FABRIQUE et DEPOSE au frigo (Pour Non-Crit M%d)\033[0m\n", getpid(), Nmalade);

                nb_rep++;
                Tncr[Nmalade] = 0;
            }
        }
        usleep(2000);
    }
    printf("DONNEUR FINI\n");
    exit(0);
}

int main() {
    int nv, mutex;
    int Qcr_l, Qncr_l, Qimp_l; 
    int tube_l[2];
    MemoirePartagee *ptr_shm;
    int id;

    Creer_et_initialiser_semaphores(&nv, &mutex);
    Creer_files_messages(&Qcr_l, &Qncr_l, &Qimp_l);
    Creer_et_attacher_tampon(&ptr_shm);
    Creer_tube(tube_l);
    
    if (fcntl(tube[0], F_SETFL, O_NONBLOCK) == -1) { perror("fcntl"); exit(1); }
    
    printf("--- DEMARRAGE SIMULATION (Processus Père PID: %d) --- \n", getpid());
    
    id = fork(); if (id == 0) MaladeCr();
    id = fork(); if (id == 0) MaladeNCr();
    id = fork(); if (id == 0) Chirurgien();
    id = fork(); if (id == 0) Donneur();

    for (int i=0; i<4; i++) wait(0); 

    Detruire_semaphores(nv, mutex);
    Detruire_files_messages(Qcr_l, Qncr_l, Qimp_l);
    Detruire_tampon(ptr_shm);
    Detruire_tube(tube_l);
    
    printf("--- FIN SIMULATION --- \n");
    return 0;
}
