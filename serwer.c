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
#include <dirent.h>
#include <openssl/md5.h>

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
    char user[30];
    char licznik[5];
    char mailbox[60];
} klient;

typedef struct klient * pmystruct;

/* Funkcja zwracająca wskaźnik do nowo utworzonej struktury Klienta */
pmystruct getpstruct() {
    pmystruct temp=(pmystruct)malloc(sizeof(pmystruct*)) ;
    return temp;
}

char *extractArgument(char *text, int argc) {
    char *arg;
    char copy[strlen(text)];
    strcpy(copy, text);
    int i=1;
    arg = strtok(copy, " \n\0");
    if (argc>=2) {
        while (i<argc) {
            arg = strtok(NULL, " \n\0");
            i++;
        }
        return arg;
    } else {
        return arg;
    }
}

void SendMessage(pmystruct gn, char message[], char type[]) {
    if (send(gn->nr, message, strlen(message), 0) != strlen(message))
    {
        printf("Send message error\n");
    } else {
        if (strcmp(type, "untagged")==0)
            printf("S: C%d %s %s", gn->nr, gn->licznik, message);
        else 
            printf("S: C%d %s %s\n", gn->nr, gn->licznik, message);
    }
}

void Greeting(pmystruct gn) {
    char message[] = "* OK IMAP4rev1 Service Ready";
    message[strlen(message)] = '\0';
    char tag[] = "tagged";
    SendMessage(gn, message, tag);
}

/* Funkcja sprawdzająca aktualny stan klienta */
bool CheckState(pmystruct gn, char *state) {
    if (strcmp(gn->state, state) == 0 ) {
        return true;
    }
    return false;
}


void WrongState(pmystruct gn) {
    char message[] = "wrong user state";
    message[strlen(message)] = '\0';
    char tag[] = "tagged";
    SendMessage(gn, message, tag);
}

int Search_User(char *fname, char *str) {
    FILE *fp;
    int line_num = 1;
    int find_result = 0;
    char temp[512];

    if((fp = fopen(fname, "r")) == NULL) {
      return(-1);
    }

    while(fgets(temp, 512, fp) != NULL) {
        if((strstr(temp, str)) != NULL && strlen(temp)-1==strlen(str)) {
            if(fp) {
                fclose(fp);
            }
            return 0;
        }
    }
    //Close the file if still open.
    if(fp) {
        fclose(fp);
    }
    return 1;
}

//          Client Commands - Any State

void Capability(pmystruct gn) {
    char tag[] = "untagged";
    char message[] = "* CAPABILITY IMAP4rev1 AUTH=PLAIN\n";
    SendMessage(gn, message, tag);
    strcpy(tag, "tagged");
    strcpy(message,"OK CAPABILITY completed");
    message[strlen(message)] = '\0';
    SendMessage(gn, message, tag);
}
void Noop(pmystruct gn) {
    char message[] = "OK NOOP completed";
    message[strlen(message)] = '\0';
    char tag[] = "tagged";
    SendMessage(gn, message, tag);
}
void Logout(pmystruct gn) {
    char message[] = "* BYE IMAP4rev1 Server logging out\n";
    char tag[] = "untagged";
    SendMessage(gn, message, tag);
    strcpy(gn->state, "log");
    strcpy(message, "OK LOGOUT Completed");
    message[strlen(message)] = '\0';
    strcpy(tag, "tagged");
    SendMessage(gn, message, tag);
}

//          Client Commands - Not Authenticated State

void Starttls(pmystruct gn) {
    char message[] = "OK STARTTLS completed";
    message[strlen(message)] = '\0';
    char tag[] = "tagged";
    SendMessage(gn, message, tag);
}

void Login(pmystruct gn, char *user, char *password) {
    char message[] = "OK LOGIN completed";
    message[strlen(message)] = '\0';
    char tag[] = "tagged";
    char state[] = "non";
    if (CheckState(gn, state)==true) {
        char file[] = "logins.txt";
        char line[60];
        sprintf(line, "%s %s", user, password);

        if (Search_User(file, line)==0){
            strcpy(gn->user, user);
            strcpy(gn->state, "aut");
            SendMessage(gn, message, tag);
        } else {
            strcpy(message, "NO [AUTHENTICATIONFAILED] Authentication failed");
            message[strlen(message)] = '\0';
            SendMessage(gn, message, tag);
        }
    }
    else {
        WrongState(gn);
    }
    
}
void Authenticate(pmystruct gn, char *mechanism) {
    char message[] = "OK AUTHENTICATE completed";
    message[strlen(message)] = '\0';
    char tag[] = "tagged";
    SendMessage(gn, message, tag);
}

//          Client Commands - Authenticated State

void Select(pmystruct gn, char *mailbox_name) {
    DIR *folder;
    struct dirent *DirEntry;
    char *dir;
    unsigned char isFile =0x8;
    int mail_count = 0;
    char state[] = "aut";
    char state2[] = "sel";
    char tag[] = "untagged";
    
    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {
        strcpy(gn->state, "sel");
        strcpy(gn->mailbox, mailbox_name);
        sprintf(dir, "%s/%s", gn->user, mailbox_name);
        
        folder = opendir(dir);

        if(folder==NULL)
          printf("Blad odczytu %s katalogu\n", dir);
        else {
            while((DirEntry=readdir(folder))!=NULL)
            {
                if(DirEntry->d_name[0] == '.' || DirEntry->d_type != isFile ) continue;
                mail_count++;
                //printf("Found mail: %s\n", DirEntry->d_name);  
            }
            closedir(folder);
            char message[50];
            sprintf(message, "* %d EXISTS\n", mail_count);
            SendMessage(gn, message, tag);
            strcpy(message, "OK [READ-WRITE] SELECT completed");
            message[strlen(message)] = '\0';
            strcpy(tag, "tagged");
            SendMessage(gn, message, tag);
        }
    }
} 
void Examine(pmystruct gn, char *mailbox_name) {
    DIR *folder;
    struct dirent *DirEntry;
    char *dir;
    unsigned char isFile =0x8;
    int mail_count = 0;
    char state[] = "aut";
    char state2[] = "sel";
    char tag[] = "untagged";
    
    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {
        sprintf(dir, "%s/%s", gn->user, mailbox_name);
        
        folder = opendir(dir);

        if(folder==NULL)
          printf("Blad odczytu %s katalogu\n", dir);
        else {
            while((DirEntry=readdir(folder))!=NULL)
            {
                if(DirEntry->d_name[0] == '.' || DirEntry->d_type != isFile ) continue;
                mail_count++;
                //printf("Found mail: %s\n", DirEntry->d_name);
            }
            closedir(folder);
            char message[50];
            sprintf(message, "* %d EXISTS\n", mail_count);
            SendMessage(gn, message, tag);
            strcpy(message, "OK [READ-ONLY] EXAMINE completed");
            message[strlen(message)] = '\0';
            strcpy(tag, "tagged");
            SendMessage(gn, message, tag);
        }
    }
}
void Create(pmystruct gn, char *mailbox_name) {
    char state[] = "aut";
    char state2[] = "sel";
    char tag[] = "tagged";

    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {
        char newdir[100];
        sprintf(newdir, "%s/%s", gn->user, mailbox_name);
        mode_t process_mask = umask(0);
        int result_code = mkdir(newdir, S_IRWXU | S_IRWXG | S_IRWXO);
        umask(process_mask);
        if (result_code==0) {
            char message[] = "OK CREATE completed";
            message[strlen(message)] = '\0';
            SendMessage(gn, message, tag);
        }  
        else {
            char message[] = "NO [CANNNOT] error";
            message[strlen(message)] = '\0';
            SendMessage(gn, message, tag);
        }
    }
}
void Delete(pmystruct gn, char *mailbox_name) {
    char state[] = "aut";
    char state2[] = "sel";
    char tag[] = "tagged";

    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {
        char com[100];
        sprintf(com, "rm -r %s/%s", gn->user, mailbox_name);
        for (int i=0; i<strlen(mailbox_name);i++) {
            mailbox_name[i]=toupper(mailbox_name[i]);
        }
        if (strcmp(mailbox_name, "INBOX")==0 || strcmp(mailbox_name, "OUTBOX")==0) {
                char message[] = "NO [CANNOT] Acces denied";
                message[strlen(message)] = '\0';
                SendMessage(gn, message, tag);
        } else {
            if (system(com)==0) {
                char message[] = "OK DELETE completed";
                message[strlen(message)] = '\0';
                SendMessage(gn, message, tag);
            } else {
                char message[] = "NO [CANNOT] no such directory";
                message[strlen(message)] = '\0';
                SendMessage(gn, message, tag);
            }
        }
    }
}
void Rename(pmystruct gn, char *mailbox_name, char *new_mailbox_name) {
    char state[] = "aut";
    char state2[] = "sel";
    char message[] = "OK RENAME completed";
    message[strlen(message)] = '\0';

    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {

    }
}
void Subscribe(pmystruct gn, char *mailbox_name) {
    char state[] = "aut";
    char state2[] = "sel";
    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {

    }
}
void Unsubscribe(pmystruct gn, char *mailbox_name) {
    char state[] = "aut";
    char state2[] = "sel";
    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {

    }
}
void List(pmystruct gn/*reference name
               mailbox name with possible wildcards*/) {
    char state[] = "aut";
    char state2[] = "sel";
    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {

    }
}
void Lsub(pmystruct gn/*reference name
               mailbox name with possible wildcards*/) {
    char state[] = "aut";
    char state2[] = "sel";
    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {

    }
}
void Status(pmystruct gn, char *mailbox_name) {
    char state[] = "aut";
    char state2[] = "sel";
    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {

    }
} 
void Append(pmystruct gn, char *mailbox_name/*mailbox name
               OPTIONAL flag parenthesized list
               OPTIONAL date/time string
               message literal*/) {
    char state[] = "aut";
    char state2[] = "sel";
    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {

    }
}

//          Client Commands - Selected State

void Check(pmystruct gn) {
    char state[] = "sel";
    if (CheckState(gn, state)==false) {
        WrongState(gn);
    } else {

    }
} 
void Close(pmystruct gn) {
    char state[] = "sel";
    if (CheckState(gn, state)==false) {
        WrongState(gn);
    } else {

    }
}
void Expunge(pmystruct gn) {
    char state[] = "sel";
    if (CheckState(gn, state)==false) {
        WrongState(gn);
    } else {

    }
}
void Search(pmystruct gn/*OPTIONAL [CHARSET] specification
               searching criteria (one or more)*/) {
    char state[] = "sel";
    if (CheckState(gn, state)==false) {
        WrongState(gn);
    } else {

    }
}
void Fetch(pmystruct gn/*sequence set
               message data item names or macro*/) {
    char state[] = "sel";
    if (CheckState(gn, state)==false) {
        WrongState(gn);
    } else {

    }
}
void Store(pmystruct gn/*sequence set
               message data item name
               value for message data item*/) {
    char state[] = "sel";
    if (CheckState(gn, state)==false) {
        WrongState(gn);
    } else {

    }
}
void Copy(pmystruct gn, char *sequence_set, char *mailbox_name) {
    char state[] = "sel";
    if (CheckState(gn, state)==false) {
        WrongState(gn);
    } else {

    }
}
void Uid(pmystruct gn, char *command_name, char *command_arguments) {
    char state[] = "sel";
    if (CheckState(gn, state)==false) {
        WrongState(gn);
    } else {

    }
}

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
    int ml = strlen(command);

    struct functionOneMETA oneFunc[] = 
        {{Capability, "CAPABILITY"}, {Noop, "NOOP"} , {Logout, "LOGOUT"} , {Starttls, "STARTTLS"} , 
        {Check, "CHECK"} , {Close, "CLOSE"} , {Expunge, "EXPUNGE"}};
    
    struct functionTwoMETA twoFunc[] = 
        {{Authenticate, "AUTHENTICATE"} , {Select, "SELECT"}, {Examine, "EXAMINE"}, 
        {Create, "CREATE"}, {Delete, "DELETE"}, {Subscribe, "SUBSCRIBE"}, {Unsubscribe, "UNSUBSCRIBE"}};
    
    struct functionThreeMETA threeFunc[] = { {Login, "LOGIN"}, {Rename, "RENAME"}, {Copy, "COPY"}, 
        {Uid, "UID"}};

    for (int i=0;i<=ml;i++) {
        if (command[i] == 32 ) {
            argcount++;
            argp[current]=i;
            current++;
        }
    }

    char com[20];
    strcpy(com, extractArgument(command, 1));
    for (int i=0; i<strlen(com);i++) {
        com[i]=toupper(com[i]);
    }
    
    if (argcount==1) {
        for (int i = 0; i<7; i++) {
            if (strcmp(com,oneFunc[i].funcName)==0) {
                found=true;
                oneFunc[i].funcPtr(gn);
            }
        }
    } else if (argcount==2) {
        for (int i = 0; i<7; i++) {
            if (strcmp(com,twoFunc[i].funcName)==0) {
                found=true;
                int size = ml-argp[0];
                char arg1[size];
                strcpy(arg1,extractArgument(command, 2));
                twoFunc[i].funcPtr(gn, arg1);
            }
        }
    } else if (argcount==3) {
        for (int i = 0; i<3; i++) {
            if (strcmp(com,threeFunc[i].funcName)==0) {
                found=true;
                int size = ml-argp[0];
                char arg1[size];
                size = ml-argp[1];
                char arg2[size];
                strcpy(arg1,extractArgument(command, 2));
                strcpy(arg2,extractArgument(command, 3));
                threeFunc[i].funcPtr(gn, arg1, arg2);
            }
        }
    } 
    if (!found) {
        char message[] = "Command not found or wrong number of arguments";
        message[strlen(message)] = '\0';
        printf("S: C%d %s %s\n", gn->nr, gn->licznik, message);
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
    } else if (licznik[0]!='z'){
        licznik[0]++;
        licznik[1]='0';
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
            strcpy(user->licznik,"a001");
            strcpy(user->state,"non");
            Greeting(user);
            while(1){
                memset(bufor, 0, BUFSIZE);
                n=recvtimeout(user->nr, bufor, BUFSIZE, 1800);
                if (n == -1) {
                    perror("recvtimeout");
                    break;
                }
                else if (n == -2) {
                    Timeout(user);

                } else {
                    printf("C: C%d %s %s", user->nr, user->licznik, bufor);
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
