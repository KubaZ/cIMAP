#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <fstream>

#define PORT htons(21212)
#define BUFSIZE 1024

void Greeting(int gn) {
    char message[] = "* OK IMAP4rev1 Service Ready\n";
    if (send(gn, message, strlen(message), 0) != strlen(message))
    {
        printf("Greeting error\n");
        close(gn);
    }
}

//          Client Commands - Any State

void Capability(int gn) {
    char message[] = "OK CAPABILITY completed \n";
    if (send(gn, message, strlen(message), 0) != strlen(message))
    {
        printf("CAPABILITY error\n");
    }
}
void Noop(int gn) {
    char message[] = "OK NOOP completed\n";
    if (send(gn, message, strlen(message), 0) != strlen(message))
    {
        printf("NOOP error\n");
    }
}
void Logout(int gn) {
    char message[] = "* BYE IMAP4rev1 Server logging out\n";
    if (send(gn, message, strlen(message), 0) != strlen(message))
    {
        printf("Logout error\n");
    }
}

//          Client Commands - Not Authenticated State

void Starttls(int gn) {
    char message[] = "OK STARTTLS completed \n";
    if (send(gn, message, strlen(message), 0) != strlen(message))
    {
        printf("Starttls error\n");
    }
}
void Login(int gn, char *user, char *password) {

    char message[] = "OK LOGIN completed \n";
    if (send(gn, message, strlen(message), 0) != strlen(message))
    {
        printf("Login error\n");
    }  

}
void Authenticate(int gn/*authentication mechanism name*/) {
    char message[] = "OK AUTHENTICATE completed \n";
    if (send(gn, message, strlen(message), 0) != strlen(message))
    {
        printf("Authenticate error\n");
    }   
}

//          Client Commands - Authenticated State

void Select(int gn, char *mailbox_name/*mailbox name*/) {

} 
void Examine(int gn/*mailbox name*/) {

}
void Create(int gn/*mailbox name*/) {

}
void Delete(int gn/*mailbox name*/) {

}
void Rename(int gn/*existing mailbox name
               new mailbox name*/) {

}
void Subscribe(int gn/*mailbox name*/) {

}
void Unsubscribe(int gn/*mailbox name*/) {

}
void List(int gn/*reference name
               mailbox name with possible wildcards*/) {

}
void Lsub(int gn/*reference name
               mailbox name with possible wildcards*/) {

}
void Status(int gn/*mailbox name
               status data item names*/) {

} 
void Append(int gn/*mailbox name
               OPTIONAL flag parenthesized list
               OPTIONAL date/time string
               message literal*/) {

}

//          Client Commands - Selected State

void Check(int gn) {

} 
void Close(int gn) {

}
void Expunge(int gn) {

}
void Search(int gn/*OPTIONAL [CHARSET] specification
               searching criteria (one or more)*/) {

}
void Fetch(int gn/*sequence set
               message data item names or macro*/) {

}
void Store(int gn/*sequence set
               message data item name
               value for message data item*/) {

}
void Copy(int gn/*sequence set
               mailbox name*/) {

}
void Uid(int gn/*command name
               command arguments*/) {

}

//          Command parser

void CommandParser(int gn, char *command) {

    int tmp=0, argcount=1, max=0, current=0;
    char *com;
    for (int i=0;i<=strlen(command);i++) {
        if (command[i] == 32) {
            if (max<current) {
                max = current;
            } 
            current = 0;
            argcount++;
        } else if (command[i] == 0) {
            if (max<current) {
                max = current;
            }
        } else 
            current++;
    }
    //printf("Args %d  Max %d\n", argcount, max);
    char temp[10][64];
    //printf("%s\n", "petla 1 skonczona" );
    current=0;
    for (int i=0;i<strlen(command);i++) {
        if (command[i]!=32 || command[i]!=0){
            temp[tmp][current] = command[i];
            //printf("%c\n", temp[tmp][current]);
            current++;
        } else {
            temp[tmp][current] = '\0';
            tmp++;
            current=0;
        }
    }
    //printf("%s\n", "petla 2 skonczona" );
    printf("%s\n", temp[0] );
    com = temp[0];

    if (strcmp(com,"logout")==0){
        Logout(gn);
    } else if (strcmp(com,"capability")==0) {
        Capability(gn);
    } else if (strcmp(com,"noop")==0) {
        Noop(gn);
    } else if (strcmp(com,"login")==0) {
        //Login(gn, name, pass);
    } else if (strcmp(com,"authenticate")==0) {
        Authenticate(gn);
    } else if (strcmp(com,"starttls")==0) {
        Starttls(gn);
    } else if (strcmp(com,"select")==0) {
        //Select(gn);
    } else if (strcmp(com,"examine")==0) {
        Examine(gn);
    } else if (strcmp(com,"create")==0) {
        Create(gn);
    } else if (strcmp(com,"delete")==0) {
        Delete(gn);
    } else if (strcmp(com,"rename")==0) {
        Rename(gn);
    } else if (strcmp(com,"subscribe")==0) {
        Subscribe(gn);
    } else if (strcmp(com,"unsubscribe")==0) {
        Unsubscribe(gn);
    } else if (strcmp(com,"list")==0) {
        List(gn);
    } else if (strcmp(com,"lsub")==0) {
        Lsub(gn);
    } else if (strcmp(com,"status")==0) {
        Status(gn);
    } else if (strcmp(com,"append")==0) {
        Append(gn);
    } else if (strcmp(com,"check")==0) {
        Check(gn);
    } else if (strcmp(com,"close")==0) {
        Close(gn);
    } else if (strcmp(com,"expunge")==0) {
        Expunge(gn);
    } else if (strcmp(com,"search")==0) {
        Search(gn);
    } else if (strcmp(com,"fetch")==0) {
        Fetch(gn);
    } else if (strcmp(com,"store")==0) {
        Store(gn);
    } else if (strcmp(com,"copy")==0) {
        Copy(gn);
    } else if (strcmp(com,"uid")==0) {
        Uid(gn);
    } else {
        strcpy(command,"command unknown or arguments invalid\n");
        send(gn, command, strlen(command), 0);
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

void ObsluzPolaczenie(int gn)
{
    char sciezka[512];
    long dl_pliku, wyslano, wyslano_razem, przeczytano;
    struct stat fileinfo;
    FILE* plik;
    char bufor[BUFSIZE];
    
    memset(sciezka, 0, 512);
    if (recv(gn, sciezka, 512, 0) <= 0)
    {
        printf("Potomny: blad przy odczycie sciezki\n");
        return;
    }
    
    printf("Potomny: klient chce plik %s\n", sciezka);
    
    if (stat(sciezka, &fileinfo) < 0)
    {
        printf("Potomny: nie moge pobrac informacji o pliku\n");
        return;
    }
    
    if (fileinfo.st_size == 0)
    {
        printf("Potomny: rozmiar pliku 0\n");
        return;
    }
    
    printf("Potomny: dlugosc pliku: %jd\n", (intmax_t)fileinfo.st_size);
    
    dl_pliku = htonl((long) fileinfo.st_size);
    
    if (send(gn, &dl_pliku, sizeof(long), 0) != sizeof(long))
    {
        printf("Potomny: blad przy wysylaniu wielkosci pliku\n");
        return;
    }
    
    dl_pliku = fileinfo.st_size;
    wyslano_razem = 0;
    plik = fopen(sciezka, "rb");
    if (plik == NULL)
    {
        printf("Potomny: blad przy otwarciu pliku\n");
        return;
    }
    
    while (wyslano_razem < dl_pliku)
    {
        przeczytano = fread(bufor, 1, BUFSIZE, plik);
        wyslano = send(gn, bufor, przeczytano, 0);
        if (przeczytano != wyslano)
            break;
        wyslano_razem += wyslano;
        printf("Potomny: wyslano %ld bajtow\n", wyslano_razem);
    }
    
    if (wyslano_razem == dl_pliku)
        printf("Potomny: plik wyslany poprawnie\n");
    else
        printf("Potomny: blad przy wysylaniu pliku\n");
    fclose(plik);
    return;    
}


int main(void)
{
    int gn_nasluch, gn_klienta;
    struct sockaddr_in adr;
    socklen_t dladr = sizeof(struct sockaddr_in);
    char bufor[BUFSIZE];
    long wiadomosc;
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
        dladr = sizeof(struct sockaddr_in);
        gn_klienta = accept(gn_nasluch, (struct sockaddr*) &adr, &dladr);
        time_t rawtime;
        time ( &rawtime );

        if (gn_klienta < 0) {
            printf("S: accept zwrocil blad\n");
            continue;
        }

        printf("S: polaczenie od %s:%u\n", inet_ntoa(adr.sin_addr), ntohs(adr.sin_port));
        file = fopen("log.txt","a+"); 
        fprintf(file,"S: polaczenie od %s:%u Czas: %s", inet_ntoa(adr.sin_addr), ntohs(adr.sin_port), ctime(&rawtime));
        fclose(file);
        printf("S: tworze proces potomny\n");

        if (fork() == 0)
        {
            /* proces potomny */
            char licznik[] = "a000";
            printf("S: zaczynam obsluge\n");
            Greeting(gn_klienta);
            while(1) {
                memset(bufor, 0, 1024);
                recv(gn_klienta, bufor, 1024, 0);
                printf("C%d: %s %s", gn_klienta, licznik, bufor);
                Licznik(licznik);
                CommandParser(gn_klienta, bufor);
            }
            printf("koncze proces potomny");
            exit(0);
        }
        else
        {
            /* proces macierzysty */
            printf("S: wracam do nasluchu\n");
            continue;
        }
    }  
    return 0;
}
