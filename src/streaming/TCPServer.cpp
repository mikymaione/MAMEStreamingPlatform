#include "TCPServer.hpp"

char *TCPServer::Errore(const char *testo, int errno)
{
    auto r = (char *)calloc(300, sizeof(char));

    sprintf(r, "%s %i\n", testo, errno);

    return r;
}

void TCPServer::Chiudi(void)
{
    close(IDConnessioneInAscolto_);
    InEsecuzione = false;

    printf("\nTCPServer: Bye!\n");
}

const char *TCPServer::TCP_Crea(const int portno, int *IDConnessioneInAscolto)
{
    *IDConnessioneInAscolto = socket(AF_INET, SOCK_STREAM, 0);
    if (*IDConnessioneInAscolto < 0)
        return TCPServer::Errore("TCPServer: Errore apertura socket", *IDConnessioneInAscolto);

    struct sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    auto bi = bind(*IDConnessioneInAscolto, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (bi < 0)
        return TCPServer::Errore("TCPServer: Errore creazione porta", bi);

    return "TCPServer: Creato!";
}

const char *TCPServer::TCP_Ascolta(const int IDConnessioneInAscolto, void (*f)())
{
    pid_t miopid = 0;
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    IDConnessioneInAscolto_ = IDConnessioneInAscolto;

    listen(IDConnessioneInAscolto, TCP_MAX_CLIENT);

    while (InEsecuzione)
    {
        auto IDNuovaConnessione = accept(IDConnessioneInAscolto, (struct sockaddr *)&cli_addr, &clilen);

        if (IDNuovaConnessione < 0)
            return TCPServer::Errore("TCPServer: Errore apertura socket con client", IDNuovaConnessione);

        if ((miopid = fork()) == 0) //processo nasce
        {
            close(IDConnessioneInAscolto);

            f(); // gioca!

            //processo muore
        }
    }

    Chiudi();

    return "\nTCPServer: Bye!";
}