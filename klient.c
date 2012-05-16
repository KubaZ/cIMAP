#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>

#define IP(H) *((unsigned long*) (H)->h_addr_list[0])

int main(void)
{
    int gn,n;
    struct sockaddr_in adr;
    int port = 21212;
    struct hostent *h;
    char nazwa[512] = "localhost";
    char bufor[1024];
    char sciezka[512];
    char *fullmessage;
    long dl_pliku, odebrano, odebrano_razem, wiadomosc;
    socklen_t dl = sizeof(struct sockaddr_in);
    
    
    h = gethostbyname(nazwa);
    if (h == NULL)
    {
        printf("Nieznany host\n");
        return 1;
    }

    adr.sin_family = AF_INET;
    adr.sin_port = htons(port);
    adr.sin_addr.s_addr = IP(h);
    
    printf("Lacze sie z %s:%d\n",
        inet_ntoa(adr.sin_addr),
        port);
    
    gn = socket(PF_INET, SOCK_STREAM, 0);
    if (connect(gn, (struct sockaddr*) &adr, sizeof(adr))<0)
    {
        printf("Nawiazanie polaczenia nie powiodlo sie\n");
        close(gn);
        return 1;
    }
    printf("Polaczenie nawiazane\n");

    while(1) {
        while(1) {
            memset(bufor, 0, 1024);
            n=recv(gn, bufor, 1024, 0);
            if (bufor[n-1]=='\n') {
                printf("S: %s", bufor);
                continue;
            } else {
                printf("S: %s\n", bufor);
                break;
            }
            if (n==0) {
                return;
            }
        }
        //printf("S: %s\n", bufor);
        printf("Podaj wiadomosc: ");
        fflush(stdin);      
        fgets(sciezka, 1024, stdin);
        if (send(gn, sciezka, strlen(sciezka), 0) != strlen(sciezka))
        {
            printf("%s\n", strerror(errno));
            close(gn);
            return 1;
        }
    }
    

    /*printf("Podaj sciezke do pliku: \n");
    memset(sciezka, 0, 512);
    scanf("%s",sciezka);    
    printf("Wysylam sciezke\n");
    if (send(gn, sciezka, strlen(sciezka), 0) != strlen(sciezka))
    {
        printf("Blad przy wysylaniu sciezki\n");
        close(gn);
        return 1;
    }
    printf("Sciezka wyslana. Odczytuje dlugosc pliku.\n");
    if (recv(gn, &dl_pliku, sizeof(long), 0) != sizeof(long))
    {
        printf("Blad przy odbieraniu dlugosci\n");
        printf("Moze plik nie istnieje?\n");
        close(gn);
        return 1;
    }
    dl_pliku = ntohl(dl_pliku);
    printf("Plik ma dlugosc %d\n", dl_pliku);*/
    /*
    printf("----- ZAWARTOSC PLIKU -----\n");
    odebrano_razem = 0;
    while (odebrano_razem < dl_pliku)
    {
        memset(bufor, 0, 1025);
        odebrano = recv(gn, bufor, 1024, 0);
        if (odebrano < 0)
            break;
        odebrano_razem += odebrano;
        fputs(bufor, stdout);
    }
    close(gn);
    if (odebrano_razem != dl_pliku)
        printf("*** BLAD W ODBIORZE PLIKU ***\n");
    else
        printf("*** PLIK ODEBRANY POPRAWNIE ***\n");*/
    return 0;
}