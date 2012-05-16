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
#include <errno.h>
#if defined(__APPLE__)
#  define COMMON_DIGEST_FOR_OPENSSL
#  include <CommonCrypto/CommonDigest.h>
#  define SHA1 CC_SHA1
#else
#  include <openssl/md5.h>
#endif


#define PORT htons(21212)
#define BUFSIZE 2048
#define SENDSIZE 512

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
    char state[5];
    char user[30];
    char licznik[6];
    char mailbox[100];
} klient;

typedef struct klient * pmystruct;

/* Funkcja zwracająca wskaźnik do nowo utworzonej struktury Klienta */
pmystruct getpstruct() {
    pmystruct temp=(pmystruct)malloc(sizeof(pmystruct*)) ;
    return temp;
}

char *str2md5(const char *str, int length) {
    int n;
    MD5_CTX c;
    unsigned char digest[16];
    char *out = (char*)malloc(33);

    MD5_Init(&c);

    while (length > 0) {
        if (length > 512) {
            MD5_Update(&c, str, 512);
        } else {
            MD5_Update(&c, str, length);
        }
        length -= 512;
        str += 512;
    }

    MD5_Final(digest, &c);

    for (n = 0; n < 16; ++n) {
        snprintf(&(out[n*2]), 16*2, "%02x", (unsigned int)digest[n]);
    }

    return out;
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
        printf("Send message error: %s\n", strerror(errno));
    } else {
        if (strcmp(type, "untagged")==0)
            printf("S: C%d %s %s", gn->nr, gn->licznik, message);
        else 
            printf("S: C%d %s %s\n", gn->nr, gn->licznik, message);
    }
}

void Greeting(pmystruct gn) {
    char message[SENDSIZE] = "* OK IMAP4rev1 Service Ready";
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
    char message[SENDSIZE] = "NO [CANNOT] User state inappropriate";
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
    char message[SENDSIZE];
    strcpy(message, "* CAPABILITY IMAP4rev1 AUTH=PLAIN\n");
    SendMessage(gn, message, tag);
    strcpy(tag, "tagged");
    strcpy(message,"OK CAPABILITY completed");
    message[strlen(message)] = '\0';
    SendMessage(gn, message, tag);
}

void Noop(pmystruct gn) {
    char message[SENDSIZE] = "OK NOOP completed";
    message[strlen(message)] = '\0';
    char tag[] = "tagged";
    SendMessage(gn, message, tag);
}

void Logout(pmystruct gn) {
    char message[SENDSIZE] = "* BYE IMAP4rev1 Server logging out\n";
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
    char message[SENDSIZE] = "OK STARTTLS completed";
    message[strlen(message)] = '\0';
    char tag[] = "tagged";
    SendMessage(gn, message, tag);
}

void Login(pmystruct gn, char *user, char *password) {
    char message[SENDSIZE] = "OK LOGIN completed";
    message[strlen(message)] = '\0';
    char tag[] = "tagged";
    char state[] = "non";
    char *output = str2md5(password, strlen(password));
    if (CheckState(gn, state)==true) {
        char file[] = "logins.txt";
        char line[128];
        sprintf(line, "%s %s", user, output);

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
    char message[SENDSIZE] = "OK AUTHENTICATE completed";
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
    char message[SENDSIZE];
    
    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {
        strcpy(gn->state, "sel");
        strcpy(gn->mailbox, mailbox_name);
        sprintf(dir, "%s/%s", gn->user, gn->mailbox);
        
        folder = opendir(dir);

        if(folder==NULL) {
            sprintf(message, "NO [CANNOT] %s", strerror(errno));
            strcpy(tag, "tagged");
            SendMessage(gn, message, tag);
        }
        else {
            while((DirEntry=readdir(folder))!=NULL)
            {
                if(DirEntry->d_name[0] == '.' || DirEntry->d_type != isFile ) continue;
                mail_count++;
                //printf("Found mail: %s\n", DirEntry->d_name);  
            }
            closedir(folder);
            sprintf(message, "* %d EXISTS\n", mail_count);
            strcpy(tag, "untagged");
            SendMessage(gn, message, tag);
            sprintf(message, "* %d RECENT\n", mail_count);
            strcpy(tag, "untagged");
            SendMessage(gn, message, tag);
            strcpy(message, "* FLAGS (\\Answered \\Flagged \\Deleted \\Seen \\Draft)\n");
            strcpy(tag, "untagged");
            SendMessage(gn, message, tag); 
            for (int i=0; i<strlen(mailbox_name);i++) {
                mailbox_name[i]=toupper(mailbox_name[i]);
            }
            if (strcmp(mailbox_name, "INBOX")==0 || strcmp(mailbox_name, "OUTBOX")==0) {
                strcpy(message, "OK [READ-ONLY] SELECT completed");
                message[strlen(message)] = '\0';
                strcpy(tag, "tagged");
                SendMessage(gn, message, tag);
            } else {
                strcpy(message, "OK [READ-WRITE] SELECT completed");
                message[strlen(message)] = '\0';
                strcpy(tag, "tagged");
                SendMessage(gn, message, tag);
            }
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
    char message[SENDSIZE];
    
    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {
        sprintf(dir, "%s/%s", gn->user, mailbox_name);
        
        folder = opendir(dir);

        if(folder==NULL) {
            sprintf(message, "NO [CANNOT] %s", strerror(errno));
            strcpy(tag, "tagged");
            SendMessage(gn, message, tag);
        }
        else {
            while((DirEntry=readdir(folder))!=NULL)
            {
                if(DirEntry->d_name[0] == '.' || DirEntry->d_type != isFile ) continue;
                mail_count++;
                //printf("Found mail: %s\n", DirEntry->d_name);
            }
            closedir(folder);
            sprintf(message, "* %d EXISTS\n", mail_count);
            strcpy(tag, "untagged");
            SendMessage(gn, message, tag);
            sprintf(message, "* %d RECENT\n", mail_count);
            strcpy(tag, "untagged");
            SendMessage(gn, message, tag);
            strcpy(message, "* FLAGS (\\Answered \\Flagged \\Deleted \\Seen \\Draft)\n");
            strcpy(tag, "untagged");
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
    char message[SENDSIZE];

    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {
        char newdir[100];
        sprintf(newdir, "%s/%s", gn->user, mailbox_name);
        for (int i=0; i<strlen(mailbox_name);i++) {
            mailbox_name[i]=toupper(mailbox_name[i]);
        }
        if (strcmp(mailbox_name, "INBOX")==0 || strcmp(mailbox_name, "OUTBOX")==0) {
            strcpy(message, "NO [CANNNOT] Access denied");
            message[strlen(message)] = '\0';
            SendMessage(gn, message, tag);
        } else {
            mode_t process_mask = umask(0);
            int result_code = mkdir(newdir, S_IRWXU | S_IRWXG | S_IRWXO);
            umask(process_mask);
            if (result_code==0) {
                strcpy(message, "OK CREATE completed");
                message[strlen(message)] = '\0';
                SendMessage(gn, message, tag);
            } else {
                sprintf(message, "NO [CANNOT] %s", strerror(errno));
                message[strlen(message)] = '\0';
                SendMessage(gn, message, tag);
            }
        }
    }
}

void Delete(pmystruct gn, char *mailbox_name) {
    char state[] = "aut";
    char state2[] = "sel";
    char tag[] = "tagged";
    char message[SENDSIZE];

    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {
        char com[100];
        sprintf(com, "%s/%s", gn->user, mailbox_name);
        for (int i=0; i<strlen(mailbox_name);i++) {
            mailbox_name[i]=toupper(mailbox_name[i]);
        }
        if (strcmp(mailbox_name, "INBOX")==0 || strcmp(mailbox_name, "OUTBOX" )==0) {
                strcpy(message, "NO [CANNOT] Access denied");
                message[strlen(message)] = '\0';
                SendMessage(gn, message, tag);
        } else {
            if (rmdir(com)==0) {
                strcpy(message, "OK DELETE completed");
                message[strlen(message)] = '\0';
                SendMessage(gn, message, tag);
            } else {
                sprintf(message, "NO [CANNOT] %s", strerror(errno));
                message[strlen(message)] = '\0';
                SendMessage(gn, message, tag);
            }
        }
    }
}

void Rename(pmystruct gn, char *mailbox_name, char *new_mailbox_name) {
    char state[] = "aut";
    char state2[] = "sel";
    char tag[] = "tagged";
    char message[SENDSIZE];

    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {
        if (strcmp(mailbox_name, "INBOX")==0 || strcmp(mailbox_name, "OUTBOX" )==0 ||
            strcmp(new_mailbox_name, "INBOX")==0 || strcmp(new_mailbox_name, "OUTBOX" )==0) {
            strcpy(message, "NO [CANNOT] Access denied");
            message[strlen(message)] = '\0';
            SendMessage(gn, message, tag);
        }
        char com[100];
        char com2[100];
        sprintf(com, "%s/%s", gn->user, mailbox_name);
        sprintf(com2, "%s/%s", gn->user, new_mailbox_name);
        if (rename(com, com2)==0) {
            strcpy(message, "OK RENAME completed");
            message[strlen(message)] = '\0';
            SendMessage(gn, message, tag);
        } else {
            sprintf(message, "NO [CANNOT] %s", strerror(errno));
            message[strlen(message)] = '\0';
            SendMessage(gn, message, tag);
        }
        
    }
}

void Subscribe(pmystruct gn, char *mailbox_name) {
    char state[] = "aut";
    char state2[] = "sel";
    char message[SENDSIZE] = "OK SUBSCRIBE completed";
    message[strlen(message)] = '\0';
    char tag[] = "tagged";
    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {
        SendMessage(gn, message, tag);
    }
}

void Unsubscribe(pmystruct gn, char *mailbox_name) {
    char state[] = "aut";
    char state2[] = "sel";
    char message[SENDSIZE] = "OK UNSUBSCRIBE completed";
    message[strlen(message)] = '\0';
    char tag[] = "tagged";
    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {
        SendMessage(gn, message, tag);
    }
}

void List(pmystruct gn/*reference name
               mailbox name with possible wildcards*/) {
    char state[] = "aut";
    char state2[] = "sel";
    char message[SENDSIZE] = "OK LIST completed";
    message[strlen(message)] = '\0';
    char tag[] = "tagged";
    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {
        SendMessage(gn, message, tag);
    }
}

void Lsub(pmystruct gn/*reference name
               mailbox name with possible wildcards*/) {
    char state[] = "aut";
    char state2[] = "sel";
    char message[SENDSIZE] = "OK LSUB completed";
    message[strlen(message)] = '\0';
    char tag[] = "tagged";
    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {
        SendMessage(gn, message, tag);
    }
}

void Status(pmystruct gn, char *mailbox_name) {
    char state[] = "aut";
    char state2[] = "sel";
    char message[SENDSIZE] = "OK STATUS completed";
    message[strlen(message)] = '\0';
    char tag[] = "tagged";
    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {
        SendMessage(gn, message, tag);
    }
}

void Append(pmystruct gn, char *mailbox_name/*mailbox name
               OPTIONAL flag parenthesized list
               OPTIONAL date/time string
               message literal*/) {
    char state[] = "aut";
    char state2[] = "sel";
    char message[SENDSIZE] = "OK APPEND completed";
    message[strlen(message)] = '\0';
    char tag[] = "tagged";
    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {
        SendMessage(gn, message, tag);
    }
}

//          Client Commands - Selected State

void Check(pmystruct gn) {
    char state[] = "sel";
    char message[SENDSIZE] = "OK CHECK completed";
    message[strlen(message)] = '\0';
    char tag[] = "tagged";
    if (CheckState(gn, state)==false) {
        WrongState(gn);
    } else {
        SendMessage(gn, message, tag);
    }
} 

void Close(pmystruct gn) {
    char state[] = "sel";
    char message[SENDSIZE] = "OK CLOSE completed";
    message[strlen(message)] = '\0';
    char tag[] = "tagged";
    if (CheckState(gn, state)==false) {
        WrongState(gn);
    } else {
        strcpy(gn->state, "aut");
        SendMessage(gn, message, tag);
    }
}

void Expunge(pmystruct gn) {
    char state[] = "sel";
    char message[SENDSIZE] = "OK EXPUNGE completed";
    message[strlen(message)] = '\0';
    char tag[] = "tagged";
    if (CheckState(gn, state)==false) {
        WrongState(gn);
    } else {
        SendMessage(gn, message, tag);
    }
}

void Search(pmystruct gn, char *search_criteria) {
    char state[] = "sel";
    char message[SENDSIZE] = "OK SEARCH completed";
    message[strlen(message)] = '\0';
    char tag[] = "tagged";
    if (CheckState(gn, state)==false) {
        WrongState(gn);
    } else {
        SendMessage(gn, message, tag);
    }
}

void Fetch(pmystruct gn, char *sequence_set, char *macro) {
    DIR *folder;
    FILE* file;
    
    struct dirent *DirEntry;
    unsigned char isFile =0x8;
    char dir[SENDSIZE], *number;
    char plik[120];
    long  sent, read, sent_full=0, dl_file=0;
    int mail_count = 0;
    unsigned char bufor[BUFSIZE];
    char state[] = "sel";

    char message[SENDSIZE] = "OK FETCH completed";
    message[strlen(message)] = '\0';
    char tag[] = "tagged";


    if (CheckState(gn, state)==false) {
        WrongState(gn);
    } else {
        //printf("File opening\n");
        //printf("%s/%s\n", gn->user, gn->mailbox);
        sprintf(dir, "%s/%s", gn->user, gn->mailbox);
        folder = opendir(dir);
        if(folder==NULL) {
            sprintf(message, "NO [CANNOT] %s", strerror(errno));
            strcpy(tag, "tagged");
            SendMessage(gn, message, tag);
        }
        else {
            //printf("WHILE fileinfo\n");
            while((DirEntry=readdir(folder))!=NULL)
            {
                
                if(DirEntry->d_name[0] == '.' || DirEntry->d_type != isFile ) continue;
                mail_count++;
                sprintf(number, "%d", mail_count);
                //printf("searching fileinfo %s for %s\n", number, sequence_set);
                if (strcmp(sequence_set, number)==0) {
                    printf("Found mail\n");
                    sprintf(plik, "%s/%s/%s", gn->user, gn->mailbox, DirEntry->d_name);
                    file = fopen(plik, "rb");
                    struct stat fileinfo;
                    
                    if (stat(plik, &fileinfo) < 0)
                    {
                        printf("Potomny: nie moge pobrac informacji o pliku\n");
                        break;
                    }
                    dl_file = fileinfo.st_size;
                    //printf("wielkosc pliku %s : %ld\n",DirEntry->d_name, dl_file);
                    while (sent_full<dl_file) {
                        //printf("In while\n");
                        read = fread(bufor, 1, BUFSIZE, file);
                        //printf("BUFOR: %s\n", bufor);
                        sent = send(gn->nr, bufor, read, 0);
                        if (read != sent)
                            break;
                        sent_full += sent;
                        //printf("Potomny: wyslano %ld bajtow\n", sent_full);
                    }
                    //printf("After while\n");
                    fclose(file);
                }
                //printf("Found mail: %s\n", DirEntry->d_name);
            }
            closedir(folder);
            SendMessage(gn, message, tag);
        }
    }
}

void Store(pmystruct gn/*sequence set
               message data item name
               value for message data item*/) {
    char state[] = "sel";
    char message[SENDSIZE] = "OK STORE completed";
    message[strlen(message)] = '\0';
    char tag[] = "tagged";
    if (CheckState(gn, state)==false) {
        WrongState(gn);
    } else {
        SendMessage(gn, message, tag);
    }
}

void Copy(pmystruct gn, char *sequence_set, char *mailbox_name) {
    char state[] = "sel";
    char message[SENDSIZE] = "OK COPY completed";
    message[strlen(message)] = '\0';
    char tag[] = "tagged";
    if (CheckState(gn, state)==false) {
        WrongState(gn);
    } else {
        SendMessage(gn, message, tag);
    }
}

void Uid(pmystruct gn, char *command_name, char *command_arguments) {
    char state[] = "sel";
    char message[SENDSIZE] = "OK UID completed";
    message[strlen(message)] = '\0';
    char tag[] = "tagged";
    if (CheckState(gn, state)==false) {
        WrongState(gn);
    } else {
        SendMessage(gn, message, tag);
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
        {Create, "CREATE"}, {Delete, "DELETE"}, {Subscribe, "SUBSCRIBE"}, {Unsubscribe, "UNSUBSCRIBE"},
        {Search, "SEARCH"}, {Status, "STATUS"}};
    
    struct functionThreeMETA threeFunc[] = { {Login, "LOGIN"}, {Rename, "RENAME"}, {Copy, "COPY"}, 
        {Uid, "UID"}, {Fetch, "FETCH"}};

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
        for (int i = 0; i<9; i++) {
            if (strcmp(com,twoFunc[i].funcName)==0) {
                found=true;
                int size = ml-argp[0];
                char arg1[size];
                strcpy(arg1,extractArgument(command, 2));
                twoFunc[i].funcPtr(gn, arg1);
            }
        }
    } else if (argcount==3) {
        for (int i = 0; i<5; i++) {
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
        char message[SENDSIZE] = "Command not found or wrong number of arguments";
        message[strlen(message)] = '\0';
        char tag[] = "tagged";
        SendMessage(gn, message, tag);
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
        printf("S: %s\n", strerror(errno));
        return 1;
    }
    
    if (listen(gn_nasluch, 10) < 0) {
       printf("S: %s\n", strerror(errno));
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
            printf("S: %s\n", strerror(errno));
            continue;
        }

        printf("S: Connection from %s:%u\n", inet_ntoa(adr.sin_addr), ntohs(adr.sin_port));
        file = fopen("log.txt","a+"); 
        fprintf(file,"S: Connection from %s:%u Time: %s", inet_ntoa(adr.sin_addr), ntohs(adr.sin_port), ctime(&rawtime));
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
                    Logout(user);

                } else if (n>0) {
                    printf("C: C%d %s %s", user->nr, user->licznik, bufor);
                    CommandParser(user, bufor);
                    Licznik(user->licznik);
                } else {
                    Logout(user);
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
