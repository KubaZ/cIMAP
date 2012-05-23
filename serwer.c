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
#include <fcntl.h>
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
const unsigned char isFile =0x8;

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
struct klient{
    int nr;
    char state[4];
    char user[30];
    char licznik[5];
    char mailbox[128];
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
    char *out = new char[33];

    MD5_Init(&c);

    MD5_Update(&c, str, length);

    MD5_Final(digest, &c);
    strcpy(out, (const char*)digest);
    //sprintf(out, "%x", digest);
    /*for (n = 0; n < 16; ++n) {
        snprintf(&(out[n*2]), 16*2, "%02x", (unsigned int)digest[n]);
    }
    out[33]='\0';*/
    //printf("crypted: %s\n", out);
    return out;
}

char *extractArgument(char *text, int argc, const char delimiter[]) {
    char *arg;
    char copy[strlen(text)];
    strcpy(copy, text);
    int i=1;
    arg = strtok(copy, delimiter);
    if (argc>=2) {
        while (i<argc) {
            arg = strtok(NULL, delimiter);
            i++;
        }
        return arg;
    } else {
        return arg;
    }
}

int SendMessage(pmystruct gn, char message[], const char type[]) {
    int i;
    if (strcmp(type, "debug")==0)
        printf("S: C%d %s %s", gn->nr, gn->licznik, message);
    if ((i=send(gn->nr, message, strlen(message), 0)) != strlen(message))
    {
        printf("Send message error: %s\n", strerror(errno));
        return i;
    } 
    return i;
}

void Greeting(pmystruct gn) {
    char message[SENDSIZE] = "* OK IMAP4rev1 Service Ready\n";
    
    SendMessage(gn, message, "debug");
}

/* Funkcja sprawdzająca aktualny stan klienta */
bool CheckState(pmystruct gn, char *state) {
    if (strcmp(gn->state, state) == 0 ) {
        return true;
    }
    return false;
}


void WrongState(pmystruct gn) {
    char message[SENDSIZE] = "NO [CANNOT] User state inappropriate\n";
    
    SendMessage(gn, message, "debug");
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
    
    char message[SENDSIZE];
    strcpy(message, "* CAPABILITY IMAP4rev1 AUTH=PLAIN\n");
    SendMessage(gn, message, "debug");
    strcpy(message,"OK CAPABILITY completed\n");
    SendMessage(gn, message, "debug");
}

void Noop(pmystruct gn) {
    char message[SENDSIZE] = "OK NOOP completed\n";
    SendMessage(gn, message, "debug");
}

void Logout(pmystruct gn) {
    char message[SENDSIZE] = "* BYE IMAP4rev1 Server logging out\n";
    SendMessage(gn, message, "debug");
    strcpy(gn->state, "log");
    strcpy(message, "OK LOGOUT Completed\n");
    SendMessage(gn, message, "debug");
}

//          Client Commands - Not Authenticated State

void Starttls(pmystruct gn) {
    char message[SENDSIZE] = "OK STARTTLS completed";
    SendMessage(gn, message, "debug");
}

void Login(pmystruct gn, char *user, char *password) {
    char message[100];
    strcpy(message, "OK LOGIN completed\n");
    char state[] = "non";
    //char output[33];
    //strcpy(output, str2md5(password, strlen(password)));
    if (CheckState(gn, state)==true) {
        char file[] = "logins.txt";
        char line[128];
        sprintf(line, "%s %s", user, password);
        if (Search_User(file, line)==0){
            strcpy(gn->user, user);
            gn->user[strlen(gn->user)] = '\0';
            strcpy(gn->state, "aut");
            SendMessage(gn, message, "debug");
        } else {
            strcpy(message, "NO [AUTHENTICATIONFAILED] Authentication failed\n");       
            SendMessage(gn, message, "debug");
        }
    }
    else {
        WrongState(gn);
    }
    
}
void Authenticate(pmystruct gn, char *mechanism) {
    char message[SENDSIZE] = "OK AUTHENTICATE completed\n";
    SendMessage(gn, message, "debug");
}

//          Client Commands - Authenticated State

void Select(pmystruct gn, char *mailbox_name) {
    DIR *folder;
    struct dirent *DirEntry;
    char *dir = new char[128];
    int mail_count = 0;
    char state[] = "aut";
    char state2[] = "sel";
    char *flags;
    int R=0,D=0,A=0,F=0,S=0,T=0,first_unseen=0;
    
    char message[SENDSIZE];
    
    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {
        if (strcasecmp(mailbox_name, "INBOX")==0){
            sprintf(dir, "%s/%s", gn->user, mailbox_name);
        } else {
            sprintf(dir, "%s/inbox/%s", gn->user, mailbox_name);
        }
        folder = opendir(dir);
        if(folder==NULL) {
            sprintf(message, "NO [CANNOT] %s\n", strerror(errno));
            SendMessage(gn, message, "debug");
        }
        else {
            strcpy(gn->state, "sel");
            strcpy(gn->mailbox, dir);
            while((DirEntry=readdir(folder))!=NULL)
            {
                if(DirEntry->d_name[0] == '.' || DirEntry->d_type != isFile ) continue;
                mail_count++;
                flags=strpbrk(DirEntry->d_name, "|");
                printf("%s\n", flags);
                if (strstr(flags, "info")!=NULL) {
                    R++;
                    if (R==1)
                        first_unseen=mail_count;
                }
                if (strstr(flags, "D")!=NULL) {
                    D++;
                }
                if (strstr(flags, "R")!=NULL) {
                    A++;
                }
                if (strstr(flags, "F")!=NULL) {
                    F++;
                }
                if (strstr(flags, "T")!=NULL) {
                    T++;
                }
                if (strstr(flags, "S")!=NULL) {
                    S++;
                }
                //printf("Found mail: %s\n", DirEntry->d_name);  
            }
            closedir(folder);
            sprintf(message, "* %d EXISTS\n", mail_count);
            SendMessage(gn, message, "debug");
            sprintf(message, "* %d RECENT\n", R);
            SendMessage(gn, message, "debug");
            if (first_unseen!=0) {
                sprintf(message, "* OK [UNSEEN %d] Message %d is first unseen\n", first_unseen, first_unseen);
                SendMessage(gn, message, "debug");
            }
            strcpy(message, "* FLAGS (");
            if (A>0)
                strcat(message, "\\Answered ");
            if (F>0)
                strcat(message, "\\Flagged ");
            if (T>0)
                strcat(message, "\\Deleted ");
            if (S>0)
                strcat(message, "\\Seen ");
            if (D>0)
                strcat(message, "\\Draft ");
            strcat(message, ")\n");
            SendMessage(gn, message, "debug"); 
            for (int i=0; i<strlen(mailbox_name);i++) {
                mailbox_name[i]=toupper(mailbox_name[i]);
            }
            
            if (strcmp(mailbox_name, "INBOX")==0 || strcmp(mailbox_name, "OUTBOX")==0) {
                strcpy(message, "OK [READ-ONLY] SELECT completed\n");
                
                SendMessage(gn, message, "debug");
            } else {
                strcpy(message, "OK [READ-WRITE] SELECT completed\n");
                
                SendMessage(gn, message, "debug");
            }
        }
    }
} 

void Examine(pmystruct gn, char *mailbox_name) {
    DIR *folder;
    struct dirent *DirEntry;
    char *dir = new char[128];
    int mail_count = 0;
    char state[] = "aut";
    char state2[] = "sel";
    char message[SENDSIZE];
    
    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {
        sprintf(dir, "%s/%s", gn->mailbox, mailbox_name);
        folder = opendir(dir);
        if(folder==NULL) {
            sprintf(message, "NO [CANNOT] %s\n", strerror(errno));
            SendMessage(gn, message, "debug");
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
            SendMessage(gn, message, "debug");
            sprintf(message, "* %d RECENT\n", mail_count);
            SendMessage(gn, message, "debug");
            strcpy(message, "* FLAGS (\\Answered \\Flagged \\Deleted \\Seen \\Draft)\n");
            SendMessage(gn, message, "debug");
            strcpy(message, "OK [READ-ONLY] EXAMINE completed\n");
            SendMessage(gn, message, "debug");
        }
    }
}

void Create(pmystruct gn, char *mailbox_name) {
    char state[] = "aut";
    char state2[] = "sel";
    
    char message[SENDSIZE];
    int result_code=0;
    char newdir[256], subdir[128], dir[256];

    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {
        if (strcasecmp(mailbox_name, "INBOX")==0) {
            strcpy(message, "NO [CANNNOT] Access denied\n");
            SendMessage(gn, message, "debug");
        } else {
            for (int i=0; i<strlen(mailbox_name); i++) {
                if(i==strlen(mailbox_name)-1 || mailbox_name[i]=='.') {
                    for (int a = 0; a<=i; a++) {
                        if (a==i && mailbox_name[a] == '.')
                            break;
                        subdir[a] = mailbox_name[a];
                    }
                    struct stat dirinfo;
                    sprintf(newdir,"%s/inbox/.%s", gn->user, subdir);
                    if (stat(newdir, &dirinfo)<0) {
                        mode_t process_mask = umask(0);
                        result_code = mkdir(newdir, S_IRWXU | S_IRWXG | S_IRWXO);
                        if (result_code<0) {
                            sprintf(message, "NO [CANNOT] %s\n", strerror(errno));
                            SendMessage(gn, message, "debug");
                            break;
                        }
                        /*sprintf(dir,"%s/new", newdir);
                        mkdir(dir, S_IRWXU | S_IRWXG | S_IRWXO);
                        sprintf(dir,"%s/tmp", newdir);
                        mkdir(dir, S_IRWXU | S_IRWXG | S_IRWXO);
                        sprintf(dir,"%s/cur", newdir);
                        mkdir(dir, S_IRWXU | S_IRWXG | S_IRWXO);
                        umask(process_mask);*/
                    }
                    
                }
            }
            if (result_code==0) {
                strcpy(message, "OK CREATE completed\n");
                SendMessage(gn, message, "debug");
            }
        }
    }
}

void Delete(pmystruct gn, char *mailbox_name) {
    char state[] = "aut";
    char state2[] = "sel";
    
    char message[SENDSIZE];
    DIR *folder;
    struct dirent *DirEntry;
    struct stat dirinfo;
    char filepath[256];
    bool subfolder=false;

    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {
        char com[100];
        sprintf(com, "%s/inbox/%s", gn->user, mailbox_name);
        if (strcasecmp(mailbox_name, "INBOX")==0) {
                strcpy(message, "NO [CANNOT] can't delete mailbox with that name\n");
                SendMessage(gn, message, "debug");
        } else {
            if (stat(com, &dirinfo)<0) {
                sprintf(message, "NO [CANNOT] %s\n", strerror(errno));
                SendMessage(gn, message, "debug");
            } else {
                folder = opendir(com);
                while((DirEntry=readdir(folder))!=NULL) {
                    if(strcmp(DirEntry->d_name, ".") == 0 || strcmp(DirEntry->d_name, "..") == 0) continue;
                    if(DirEntry->d_type != isFile) {
                        sprintf(message, "NO [CANNOT] maibox has subfolders\n");
                        SendMessage(gn, message, "debug");
                        subfolder=true;
                        break;
                    }
                }
                if(!subfolder) {
                    rewinddir(folder);
                    while((DirEntry=readdir(folder))!=NULL) {
                        if(DirEntry->d_name[0] == '.') continue;
                        sprintf(filepath, "%s/%s", com, DirEntry->d_name);
                        remove(filepath);
                    }
                    if (rmdir(com)==0) {
                        strcpy(message, "OK DELETE completed\n");
                        SendMessage(gn, message, "debug");
                    } else {
                        sprintf(message, "NO [CANNOT] %s\n", strerror(errno));
                        SendMessage(gn, message, "debug");
                    }
                }
            }
        }
    }
}

void Rename(pmystruct gn, char *mailbox_name, char *new_mailbox_name) {
    char state[] = "aut";
    char state2[] = "sel";
    
    struct stat dirinfo;
    char message[SENDSIZE];

    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {
        if (strcasecmp(mailbox_name, "INBOX")==0 || strcasecmp(new_mailbox_name, "INBOX")==0) {
            strcpy(message, "NO [CANNOT] Access denied\n");
            SendMessage(gn, message, "debug");
        }
        char com[100];
        char com2[100];
        sprintf(com, "%s/%s", gn->mailbox, mailbox_name);
        sprintf(com2, "%s/%s", gn->mailbox, new_mailbox_name);
            if (rename(com, com2)==0) {
                strcpy(message, "OK RENAME completed\n");
                
                SendMessage(gn, message, "debug");
            } else {
                sprintf(message, "NO [CANNOT] %s\n", strerror(errno));
                
                SendMessage(gn, message, "debug");
            }
    }
}

void Subscribe(pmystruct gn, char *mailbox_name) {
    char state[] = "aut";
    char state2[] = "sel";
    char message[SENDSIZE] = "OK SUBSCRIBE completed\n";
    
    
    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {
        SendMessage(gn, message, "debug");
    }
}

void Unsubscribe(pmystruct gn, char *mailbox_name) {
    char state[] = "aut";
    char state2[] = "sel";
    char message[SENDSIZE] = "OK UNSUBSCRIBE completed\n";
    
    
    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {
        SendMessage(gn, message, "debug");
    }
}

void List(pmystruct gn, char *reference_name, char *mailbox_name) {
    DIR *folder;
    struct dirent *DirEntry;
    char *dir = new char[128];
    int mail_count = 0;
    char state[] = "aut";
    char state2[] = "sel";
    char message[SENDSIZE] = "OK LIST completed\n";
    
    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {
        if (strstr(reference_name, "inbox")!=NULL) {
            sprintf(dir, "%s/%s", gn->user, reference_name);
        } else {
            sprintf(dir, "%s/%s", gn->mailbox, reference_name);
        }
        folder = opendir(dir);
        if(folder==NULL) {
            sprintf(message, "NO [CANNOT] %s\n", strerror(errno));
            SendMessage(gn, message, "debug");
        }
        else {
            while((DirEntry=readdir(folder))!=NULL)
            {
                if(strcmp(DirEntry->d_name, ".") == 0 || strcmp(DirEntry->d_name, "..") == 0 || DirEntry->d_type == isFile ) continue;
                if ((strstr(DirEntry->d_name, mailbox_name)) != NULL || strcmp(mailbox_name, "*")==0) {
                    if(strstr(DirEntry->d_name, ".") != NULL) {
                        sprintf(message, "* LIST () \".\" %s\n", DirEntry->d_name);
                        SendMessage(gn, message, "debug");
                    }
                    else {
                        sprintf(message, "* LIST () \"/\" %s\n", DirEntry->d_name);
                        SendMessage(gn, message, "debug");
                    }
                }
            }
            closedir(folder);
            strcpy(message, "OK LIST completed\n");
            SendMessage(gn, message, "debug");
        }
    }
}

void Lsub(pmystruct gn, char *reference_name, char *mailbox_name) {
    char state[] = "aut";
    char state2[] = "sel";
    char message[SENDSIZE] = "OK LSUB completed\n";
    
    
    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {
        SendMessage(gn, message, "debug");
    }
}

void Status(pmystruct gn, char *mailbox_name) {
    char state[] = "aut";
    char state2[] = "sel";
    char message[SENDSIZE] = "OK STATUS completed\n";
    
    
    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {
        SendMessage(gn, message, "debug");
    }
}

void Append(pmystruct gn, char *mailbox_name/*OPTIONAL flag parenthesized list
               OPTIONAL date/time string
               message literal*/) {
    char state[] = "aut";
    char state2[] = "sel";
    char message[SENDSIZE] = "OK APPEND completed\n";
    
    
    if (CheckState(gn, state)==false && CheckState(gn, state2)==false) {
        WrongState(gn);
    } else {
        SendMessage(gn, message, "debug");
    }
}

//          Client Commands - Selected State

void Check(pmystruct gn) {
    char state[] = "sel";
    char message[SENDSIZE] = "OK CHECK completed\n";
    
    
    if (CheckState(gn, state)==false) {
        WrongState(gn);
    } else {
        SendMessage(gn, message, "debug");
    }
} 

void Close(pmystruct gn) {
    char state[] = "sel";
    char message[SENDSIZE] = "OK CLOSE completed\n";
    
    if (CheckState(gn, state)==false) {
        WrongState(gn);
    } else {
        strcpy(gn->state, "aut");
        strcpy(gn->mailbox, "");
        SendMessage(gn, message, "debug");
    }
}

void Expunge(pmystruct gn) {
    char state[] = "sel";
    char message[SENDSIZE] = "OK EXPUNGE completed\n";
    
    
    if (CheckState(gn, state)==false) {
        WrongState(gn);
    } else {
        SendMessage(gn, message, "debug");
    }
}

void Search(pmystruct gn, char *search_criteria) {
    char state[] = "sel";
    char message[SENDSIZE] = "OK SEARCH completed\n";
    
    
    if (CheckState(gn, state)==false) {
        WrongState(gn);
    } else {
        SendMessage(gn, message, "debug");
    }
}

void Fetch(pmystruct gn, char *sequence_set, char *macro) {
    DIR *folder;
    FILE* file;
    struct dirent *DirEntry;
    
    char *number = new char[5];
    char plik[128];
    long sent, read, sent_full=0, dl_file=0;
    int mail_count = 0, delimiter, from;
    char bufor[BUFSIZE];
    char state[] = "sel";
    char message[SENDSIZE];
    char *checked;

    char firstNum[6];
    char secondNum[6];
    if (strchr(sequence_set, ':')==NULL) {
        strcpy(firstNum, extractArgument(sequence_set, 1, ":"));
    } else {
        strcpy(firstNum, extractArgument(sequence_set, 1, ":"));
        strcpy(secondNum, extractArgument(sequence_set, 2, ":"));
    }
    from = atoi(firstNum);
    if (strcmp(secondNum, "*")!=0){
        delimiter = atoi(secondNum);
    }

    if (CheckState(gn, state)==false) {
        WrongState(gn);
    } else {
        folder = opendir(gn->mailbox);
        sprintf(number, "%d", mail_count);
        while((DirEntry=readdir(folder))!=NULL)
        { 
            if(DirEntry->d_name[0] == '.' || DirEntry->d_type != isFile ) continue;
            mail_count++;
            if ((from<=mail_count && delimiter>=mail_count) || (from<=mail_count && strcmp(secondNum, "*")==0)) {
                sprintf(plik, "%s/%s", gn->mailbox, DirEntry->d_name);
                file = fopen(plik, "rb");
                struct stat fileinfo;  
                if (stat(plik, &fileinfo) < 0)
                {
                    sprintf(message, "NO [CANNOT] %s\n", strerror(errno));
                    
                    SendMessage(gn, message, "debug");
                    break;
                }
                dl_file = fileinfo.st_size;
                
                sprintf(message, "* FETCH %d ...\n", mail_count);
                SendMessage(gn, message, "debug");
                while (sent_full<dl_file) {
                    read = fread(bufor, 1, BUFSIZE, file);
                    sent = SendMessage(gn, bufor, "not");
                    if (read != sent)
                        break;
                    sent_full += sent;
                }
                strcpy(bufor, "");
                sent_full = 0;
                sent = 0;
                fclose(file);
            }
        }
        closedir(folder);
        
        strcpy(message, "OK FETCH completed\n");
        
        SendMessage(gn, message, "debug");
    }
}

void Store(pmystruct gn, char *sequence_set
               /*message data item name
               value for message data item*/) {
    char state[] = "sel";
    char message[SENDSIZE] = "OK STORE completed\n";
    
    char firstNum[6];
    char secondNum[6];
    if (strchr(sequence_set, ':')==NULL) {
        strcpy(firstNum, extractArgument(sequence_set, 1, ":"));
    } else {
        strcpy(firstNum, extractArgument(sequence_set, 1, ":"));
        strcpy(secondNum, extractArgument(sequence_set, 2, ":"));
    }

    if (CheckState(gn, state)==false) {
        WrongState(gn);
    } else {
        SendMessage(gn, message, "debug");
    }
}

void Copy(pmystruct gn, char *sequence_set, char *mailbox_name) {
    DIR *folder;
    FILE *file1, *file2;
    struct dirent *DirEntry;
    struct stat object;
    int input, output, mail_count=0, from, delimiter;
    char filename[256];

    char state[] = "sel";
    char message[SENDSIZE] = "OK COPY completed\n";
    
    
    char ch;

    char firstNum[6];
    char secondNum[6];
    if (strchr(sequence_set, ':')==NULL) {
        strcpy(firstNum, extractArgument(sequence_set, 1, ":"));
    } else {
        strcpy(firstNum, extractArgument(sequence_set, 1, ":"));
        strcpy(secondNum, extractArgument(sequence_set, 2, ":"));
    }
    from = atoi(firstNum);
    if (strcmp(secondNum, "*")!=0){
        delimiter = atoi(secondNum);
    }

    sprintf(filename, "%s/inbox/%s", gn->user, mailbox_name);
    folder = opendir(filename);
    if (CheckState(gn, state)==false) {
        WrongState(gn);
    } else {
        if(folder==NULL) {
            sprintf(message, "NO [CANNOT] %s\n", strerror(errno));
            
            SendMessage(gn, message, "debug");
        } else {
            folder = opendir(gn->mailbox);
            while((DirEntry=readdir(folder))!=NULL) { 
                if(DirEntry->d_name[0] == '.' || DirEntry->d_type != isFile ) continue;
                mail_count++;
                if ((from<=mail_count && delimiter>=mail_count) || (from<=mail_count && strcmp(secondNum, "*")==0)) {
                    sprintf(filename, "%s/%s",gn->mailbox, DirEntry->d_name);
                    file1 = fopen(filename, "rb");
                    sprintf(filename, "%s/inbox/%s/%s", gn->user, mailbox_name, DirEntry->d_name);
                    file2 = fopen(filename, "wb");
                    while(!feof(file1))
                    {
                        ch = getc(file1);
                        putc(ch,file2);
                    }
                    putc('\0',file2);
                    fclose(file1);
                    fclose(file2);
                }
            }
            SendMessage(gn, message, "debug");
        }
    }
}

void Uid(pmystruct gn, char *command_name, char *command_arguments) {
    char state[] = "sel";
    char message[SENDSIZE] = "OK UID completed\n";

    if (CheckState(gn, state)==false) {
        WrongState(gn);
    } else {
        SendMessage(gn, message, "debug");
    }
}

/*
Command parser
first: extract command name;
second: check if command exists;
third: extract args from command and exec;
*/
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
    }else if (licznik[0]!='z'){
        licznik[0]++;
        licznik[1]='0';
        licznik[2]='0';
        licznik[3]='0';
    }
}
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
        {Uid, "UID"}, {Fetch, "FETCH"}, {List, "LIST"}};

    for (int i=0;i<=ml;i++) {
        if (command[i] == ' ' ) {
            argcount++;
            argp[current]=i;
            current++;
        }
    }

    char com[20];
    strcpy(com, extractArgument(command, 1, " \n\r\f"));
    if (argcount==1) {
        for (int i = 0; i<7; i++) {
            if (strcasecmp(com,oneFunc[i].funcName)==0) {
                found=true;
                oneFunc[i].funcPtr(gn);
            }
        }
    } else if (argcount==2) {
        for (int i = 0; i<9; i++) {
            if (strcasecmp(com,twoFunc[i].funcName)==0) {
                found=true;
                int size = ml-argp[0];
                char arg1[size];
                strcpy(arg1,extractArgument(command, 2, " \n\r\f"));
                twoFunc[i].funcPtr(gn, arg1);
            }
        }
    } else if (argcount==3) {
        for (int i = 0; i<6; i++) {
            if (strcasecmp(com,threeFunc[i].funcName)==0) {
                found=true;
                int size = ml-argp[0];
                char arg1[size];
                size = ml-argp[1];
                char arg2[size];
                strcpy(arg1,extractArgument(command, 2, " \n\r\f"));
                strcpy(arg2,extractArgument(command, 3, " \n\r\f"));
                threeFunc[i].funcPtr(gn, arg1, arg2);

            }
        }
    } 
    if (!found) {
        char message[SENDSIZE] = "BAD [CANNOT] command unknown or arguments invalid\n";
        
        
        SendMessage(gn, message, "debug");
    }
    Licznik(gn->licznik);
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
