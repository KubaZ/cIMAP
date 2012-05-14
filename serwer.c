#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fstream>



#define PORT htons(21212)
#define BUFSIZE 2048

/*Definicje wskaźników do funkcji z różną ilością argumentów*/
typedef void (*FunctionWithOneParameter) (struct klient *gn);
typedef void (*FunctionWithTwoParameter) (struct klient *gn, char *arg1);
typedef void (*FunctionWithThreeParameter) (struct klient *gn, char *arg1, char *arg2);

/*Struktury zawierające wskaźniki do funkcji z różną ilością argumentów i ich nazwy*/
typedef struct functionOneMETA {
    FunctionWithOneParameter funcPtr;
    char funcName[20];
} functionOneMETA;

typedef struct functionTwoMETA {
    FunctionWithTwoParameter funcPtr;
    char funcName[20];
} functionTwoMETA;

typedef struct functionThreeMETA {
    FunctionWithThreeParameter funcPtr;
    char funcName[20];
} functionThreeMETA;

/*
Struktura Klienta zawierająca jego nr deskryptora, stan, 
licznik poleceń oraz aktualnie wybrany folder.
*/
struct klient {
    int nr;
    char state[4];
    char user[20];
    char licznik[4];
    char mailbox[20];
} klient;

typedef struct klient * pmystruct;

/* Funkcja zwracająca wskaźnik do nowo utworzonej struktury Klienta */
pmystruct getpstruct() {
    pmystruct temp=(pmystruct)malloc(sizeof(pmystruct*)) ;
    return temp;
}

void Greeting( pmystruct gn) {

    char message[] = "* OK IMAP4rev1 Service Ready\n";
    if (send(gn->nr, message, strlen(message), 0) != strlen(message))
    {
        printf("Greeting error\n");
    } else {
        printf("S: %s %s", gn->licznik, message);
    }
}

/* Funkcja sprawdzająca aktualny stan klienta */
bool checkState(pmystruct gn, char *state) {
    if (strcmp(gn->state, state) == 0 ) {
        return true;
    }
    return false;
}


void wrongState(pmystruct gn) {
    char message[] = "wrong user state";
    if (send(gn->nr, message, strlen(message), 0) != strlen(message))
    {
        printf("wrong state indicate error\n");
    }
}

//          Client Commands - Any State

void Capability(pmystruct gn) {

    char message[] = "* CAPABILITY IMAP4rev1 AUTH=PLAIN\n";
    if (send(gn->nr, message, strlen(message), 0) != strlen(message))
    {
        printf("CAPABILITY untaged error\n");
    } else {
        strcpy(message, "OK CAPABILITY completed\n");
        if (send(gn->nr, message, strlen(message), 0) != strlen(message))
        {
            printf("CAPABILITY taged error\n");
        }
        printf("S: %s %s", gn->licznik, message);
    }
}
void Noop(pmystruct gn) {

    char message[] = "OK NOOP completed\n";
    if (send(gn->nr, message, strlen(message), 0) != strlen(message))
    {
        printf("NOOP error\n");
    } else {
        printf("S: %s %s", gn->licznik, message);
    }
}
void Logout(pmystruct gn) {

    char message[] = "* BYE IMAP4rev1 Server logging out\n";
    if (send(gn->nr, message, strlen(message), 0) != strlen(message))
    {
        printf("Logout error\n"); 
    } else {
        strcpy(gn->state, "log");
        strcpy(message, "OK LOGOUT Completed\n");
        if (send(gn->nr, message, strlen(message), 0) != strlen(message))
        {
            printf("Logout error\n"); 
        } else {
            close(gn->nr);
            printf("S: %s %s", gn->licznik, message);
        }
    }
}

//          Client Commands - Not Authenticated State

void Starttls(pmystruct gn) {

    char message[] = "OK STARTTLS completed \n";
    if (send(gn->nr, message, strlen(message), 0) != strlen(message))
    {
        printf("Starttls error\n");
    } else {
        printf("S: %s %s", gn->licznik, message);
    }
}

void Login(pmystruct gn, char *user, char *password) {

    char message[] = "OK LOGIN completed \n";
    strcpy(gn->state,"aut");
    //strcpy(gn->user, user);
    if (send(gn->nr, message, strlen(message), 0) != strlen(message))
    {
        printf("Login error\n");
    } else {
        printf("S: %s %s", gn->licznik, message);
    }

}
void Authenticate(pmystruct gn, char *mechanism) {
    char message[] = "OK AUTHENTICATE completed \n";
    if (send(gn->nr, message, strlen(message), 0) != strlen(message))
    {
        printf("Authenticate error\n");
    } else {
        printf("S: %s %s", gn->licznik, message);
    }   
}

//          Client Commands - Authenticated State

void Select(pmystruct gn, char *mailbox_name) {
    char state[] = "aut";
    char message[] = "OK [READ-WRITE] SELECT completed\n";
    
    if (checkState(gn, state)==false) {
        wrongState(gn);
    } else {
        strcpy(gn->state, "sel");
        if (send(gn->nr, message, strlen(message), 0) != strlen(message))
        {
            printf("SELECT error\n");
        } else {
            printf("S: %s %s", gn->licznik, message);
        } 
    }
    
} 
void Examine(pmystruct gn, char *mailbox_name) {
    char state[] = "sel";
    if (checkState(gn, state)==false) {
        wrongState(gn);
    } else {

    }
}
void Create(pmystruct gn, char *mailbox_name) {
    char state[] = "sel";
    if (checkState(gn, state)==false) {
        wrongState(gn);
    } else {

    }
}
void Delete(pmystruct gn, char *mailbox_name) {
    char state[] = "sel";
    if (checkState(gn, state)==false) {
        wrongState(gn);
    } else {

    }
}
void Rename(pmystruct gn, char *mailbox_name, char *new_mailbox_name) {
    char state[] = "sel";
    if (checkState(gn, state)==false) {
        wrongState(gn);
    } else {

    }
}
void Subscribe(pmystruct gn, char *mailbox_name) {
    char state[] = "sel";
    if (!checkState(gn, state)) {
        wrongState(gn);
    } else {

    }
}
void Unsubscribe(pmystruct gn, char *mailbox_name) {
    char state[] = "sel";
    if (checkState(gn, state)==false) {
        wrongState(gn);
    } else {

    }
}
void List(pmystruct gn/*reference name
               mailbox name with possible wildcards*/) {
    char state[] = "sel";
    if (checkState(gn, state)==false) {
        wrongState(gn);
    } else {

    }
}
void Lsub(pmystruct gn/*reference name
               mailbox name with possible wildcards*/) {
    char state[] = "sel";
    if (checkState(gn, state)==false) {
        wrongState(gn);
    } else {

    }
}
void Status(pmystruct gn, char *mailbox_name/*mailbox name
               status data item names*/) {
    char state[] = "sel";
    if (checkState(gn, state)==false) {
        wrongState(gn);
    } else {

    }
} 
void Append(pmystruct gn, char *mailbox_name/*mailbox name
               OPTIONAL flag parenthesized list
               OPTIONAL date/time string
               message literal*/) {
    char state[] = "sel";
    if (checkState(gn, state)==false) {
        wrongState(gn);
    } else {

    }
}

//          Client Commands - Selected State

void Check(pmystruct gn) {

} 
void Close(pmystruct gn) {

}
void Expunge(pmystruct gn) {

}
void Search(pmystruct gn/*OPTIONAL [CHARSET] specification
               searching criteria (one or more)*/) {

}
void Fetch(pmystruct gn/*sequence set
               message data item names or macro*/) {

}
void Store(pmystruct gn/*sequence set
               message data item name
               value for message data item*/) {

}
void Copy(pmystruct gn/*sequence set
               mailbox name*/) {

}
void Uid(pmystruct gn/*command name
               command arguments*/) {

}

/*char *extractArgument(char *text, int b, int e) {
    char *arg;
    //printf("%s\n", text);
    int i=0;
    for (b;b<e;b++) {
        arg[i]=text[b];
        i++;
    }
    arg[i]='\0';
    arg = strtok(text, " \n\0");
    //printf("%s\n", arg);
    return arg;
}*/

/*
Command parser
first: extract command name;
second: check if command exists;
third: extract args from command and exec;
*/
void CommandParser( pmystruct gn, char *command) {

    int tmp=0, argcount=1, max=0, current=0;
    int argp[6];
    bool found=false;
    char *arg1, *arg2;
    int ml=strlen(command);

    struct functionOneMETA oneFunc[] = 
        {{Capability, "CAPABILITY"}, {Noop, "NOOP"} , {Logout, "LOGOUT"} , {Starttls, "STARTTLS"} , 
        {Check, "CHECK"} , {Close, "CLOSE"} , {Expunge, "EXPUNGE"}};
    
    struct functionTwoMETA twoFunc[] = 
        {{Authenticate, "AUTHENTICATE"} , {Select, "SELECT"}, {Examine, "EXAMINE"}, 
        {Create, "CREATE"}, {Delete, "DELETE"}, {Subscribe, "SUBSCRIBE"}, {Unsubscribe, "UNSUBSCRIBE"}};
    
    struct functionThreeMETA threeFunc[] = { {Login, "LOGIN"}, {Rename, "RENAME"}};

    for (int i=0;i<=ml;i++) {
        if (command[i] == 32) {
            argcount++;
            argp[current]=i;
            current++;
        }
    }

    char *com = strtok(command, " \n\0");
    for (int i=0; i<strlen(com);i++) {
        com[i]=toupper(com[i]);
    }
    
    if (argcount==1) {
        for (int i = 0; i<sizeof(oneFunc); i++) {
            if (strcmp(com,oneFunc[i].funcName)==0) {
                found=true;
                oneFunc[i].funcPtr(gn);
            }
        }
    } else if (argcount==2) {
        for (int i = 0; i<sizeof(twoFunc); i++) {
            if (strcmp(com,twoFunc[i].funcName)==0) {
                found=true;
                twoFunc[i].funcPtr(gn, arg1);
            }
        }
    } else if (argcount==3) {
        for (int i = 0; i<sizeof(oneFunc); i++) {
            if (strcmp(com,threeFunc[i].funcName)==0) {
                found=true;
                threeFunc[i].funcPtr(gn, arg1, arg2);
            }
        }
    } 
    if (!found) {
        char message[] = "Command not found or wrong number of arguments\n";
        printf("S: %s", message);
        if (send(gn->nr, message, strlen(message), 0) != strlen(message))
        {
            printf("command debug error\n");
            close(gn->nr);
        }
    }
}

void Licznik(char *licznik) {
    if (licznik[3]!='9')
        licznik[3]++;
    else if (licznik[2]!=9) {
        licznik[2]++;
        licznik[3]='0';
    } else if (licznik[1]!=9) {
        licznik[1]++;
        licznik[2]='0';
        licznik[3]='0';
    }
}

int recvtimeout(int s, char *buf, int len, int timeout)
{
    fd_set fds;
    int n;
    struct timeval tv;

    // set up the file descriptor set
    FD_ZERO(&fds);
    FD_SET(s, &fds);

    // set up the struct timeval for the timeout
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    // wait until timeout or data received
    n = select(s+1, &fds, NULL, NULL, &tv);
    if (n == 0) return -2; // timeout!
    if (n == -1) return -1; // error

    // data must be here, so do a normal recv()
    return recv(s, buf, len, 0);
}

void Timeout(pmystruct gn) {
    
    char message[] = "* Connection Timeout\n* BYE IMAP4rev1 Server logging out\n";  
    if (send(gn->nr, message, strlen(message), 0) != strlen(message))
    {
        printf("Timeout message error\n");
    } else {
        printf("S: Connection with C%d timed out\n", gn->nr);
        strcpy(gn->state, "log");
        strcpy(message, "OK LOGOUT Completed\n");
        if (send(gn->nr, message, strlen(message), 0) != strlen(message))
        {
            printf("Logout error\n"); 
        } else {
            close(gn->nr);
            printf("S: %s %s", gn->licznik, message);
        }
    }
}

int main(void)
{
    int gn_nasluch;
    struct sockaddr_in adr;
    socklen_t dladr = sizeof(struct sockaddr_in);
    char bufor[BUFSIZE];
    FILE *file; 
    
    gn_nasluch = socket(AF_INET, SOCK_STREAM, 0);
    adr.sin_family = AF_INET;
    adr.sin_port = PORT;
    adr.sin_addr.s_addr = INADDR_ANY;
    memset(adr.sin_zero, 0, sizeof(adr.sin_zero));
    
    
    if (bind(gn_nasluch, (struct sockaddr*) &adr, dladr) < 0)
    {
        printf("S: bind nie powiodl sie\n");
        return 1;
    }
    
    if (listen(gn_nasluch, 10) < 0) {
       printf("S: Listen nie powiodl sie.\n");
       return 1; 
    }
    
    while(1)
    {
        pmystruct user= getpstruct();
        int n;
        dladr = sizeof(struct sockaddr_in);
        user->nr = accept(gn_nasluch, (struct sockaddr*) &adr, &dladr);
        time_t rawtime;
        time ( &rawtime );

        if (user->nr < 0) {
            printf("S: accept zwrocil blad\n");
            continue;
        }

        printf("S: polaczenie od %s:%u\n", inet_ntoa(adr.sin_addr), ntohs(adr.sin_port));
        file = fopen("log.txt","a+"); 
        fprintf(file,"S: polaczenie od %s:%u Czas: %s", inet_ntoa(adr.sin_addr), ntohs(adr.sin_port), ctime(&rawtime));
        fclose(file);

        if (fork() == 0)
        {
            strcpy(user->licznik,"a000");
            strcpy(user->state,"non");
            Greeting(user);
            while(1){
                memset(bufor, 0, BUFSIZE);
                n=recvtimeout(user->nr, bufor, BUFSIZE, 15);
                if (n == -1) {
                    perror("recvtimeout");
                    break;
                }
                else if (n == -2) {
                    Timeout(user);

                } else {
                    printf("C%d: %s %s", user->nr, user->licznik, bufor);
                    CommandParser(user, bufor);
                    Licznik(user->licznik);
                }
                if (strcmp(user->state, "log")==0) {
                    free(user);
                    break;
                }
            }
        }
        else
        {
            continue;
        }
    }  
    return 0;
}
