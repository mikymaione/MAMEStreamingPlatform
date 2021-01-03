#pragma once

#include <exception>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <unistd.h>

#define TCP_MAX_CLIENT 10000

class TCPServer
{
public:
    const char *TCP_Ascolta(const int IDConnessioneInAscolto, void (*f)());
    const char *TCP_Crea(const int portno, int *IDConnessioneInAscolto_);

private:
    int IDConnessioneInAscolto_ = -1;
    bool InEsecuzione = true;

    void Chiudi(void);
    char *Errore(const char *testo, int errno);
};