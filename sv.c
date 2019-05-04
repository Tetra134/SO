#include "estruturas.c"

int client_to_server=0;
int server_to_client=0;

void sv()
{
    int p, r, q, status;
    int codigo, nCmds, quant;
    int totalLido, codValido = 1;
    char **strings;
    char buf[1024];
    char str[50];
    int vendas_fd = open("vendas.txt", O_WRONLY | O_APPEND);
    int stocks_fd = open("stocks.txt", O_RDWR);
    int artigos_fd = open("artigos.txt", O_RDONLY);

    
    char *myfifo = "client_to_server_fifo";
    char *myfifo2 = "server_to_client_fifo";

    mkfifo(myfifo, 0666);
    mkfifo(myfifo2, 0666);

    printf("----------------------\n");
    printf("------ SERVIDOR ------\n");
    printf("----------------------\n");

    client_to_server = open(myfifo, O_RDONLY);
    server_to_client = open(myfifo2, O_WRONLY);

    while (1)
    {
        while ((totalLido = read(client_to_server, buf, sizeof(buf))) > 0)
        {
            // buf[totalLido] = '\0';
            printf("Mensagem recebida do cliente: %s\n", buf);

            strings = malloc(sizeof(char *) * 2);
            strings = splitString(buf);

            for (nCmds = 0; strings[nCmds] != NULL; nCmds++)
            {
            };
            codigo = atoi(strings[0]);

            if (codigo >= idAtualArtigos)
            {
                if ((p = fork()) == 0)
                {
                    write(server_to_client, "Esse artigo não se encontra registado\n", strlen("Esse artigo não se encontra registado\n"));
                    _exit(1);
                }
                else
                {
                    wait(&status);
                    codValido = 0;
                }
            }

            if (nCmds == 1 && codValido == 1)
            {
                if ((p = fork()) == 0)
                {
                    char buffilho[6];
                    avancar_offset_stock(codigo, stocks_fd);
                    read(stocks_fd, buffilho, 6);
                    int stock = atoi(buffilho);
                    memset(&buffilho[0], 0, sizeof(buffilho));
                    avancar_offset_artigos(codigo, artigos_fd);
                    lseek(artigos_fd, 14, SEEK_CUR);
                    read(artigos_fd, buffilho, 6);
                    int preco = atoi(buffilho);
                    memset(&buffilho[0], 0, sizeof(buffilho));

                    sprintf(str, "Em stock: %d Preco: %d\n", stock, preco);
                    write(server_to_client, str, strlen(str));
                    _exit(1);
                }
                else
                {
                    wait(&status);
                }
            }

            if (nCmds == 2 && codValido == 1)
            {
                quant = atoi(strings[1]);
                if (quant < 0)
                {
                    //efetuar venda
                    if ((p = fork()) == 0)
                    {
                        char buffilho[6];
                        avancar_offset_stock(codigo, stocks_fd);
                        read(stocks_fd, buffilho, 6);
                        int stock_antigo = atoi(buffilho);
                        int stock_novo = stock_antigo + quant;

                        if (stock_novo < 0) {
                            write(server_to_client,"Quantidade indisponivel.\n",strlen("Quantidade indisponivel.\n"));
                            _exit(-1);
                        }
                        

                        avancar_offset_stock(codigo, stocks_fd);
                        write(stocks_fd,NumToString(stock_novo),6);
                        memset(&buffilho[0], 0, sizeof(buffilho));

                        avancar_offset_artigos(codigo, artigos_fd);
                        lseek(artigos_fd, 14, SEEK_CUR);
                        read(artigos_fd, buffilho, 6);
                        int preco = atoi(buffilho);
                        memset(&buffilho[0], 0, sizeof(buffilho));

                        sprintf(str, "%d %d %d\n", codigo, abs(quant), (abs(quant) * preco));
                        write(vendas_fd, str, strlen(str));

                        sprintf(str, "Stock atual: %d\n", stock_novo);
                        write(server_to_client, str, strlen(str));

                        idAtualVendas++;

                        _exit(2);
                    }
                    else
                    {
                        wait(&status);
                    }
                }
                else
                {
                    //entrada em stock
                    if ((p = fork()) == 0)
                    {
                        char buffilho[6];
                        avancar_offset_stock(codigo, stocks_fd);
                        read(stocks_fd, buffilho, 6);
                        int stock_antigo = atoi(buffilho);
                        int stock_novo = stock_antigo + quant;

                        sprintf(str, "Stock atual: %d\n", stock_novo);
                        write(server_to_client, str, strlen(str));

                        avancar_offset_stock(codigo, stocks_fd);
                        write(stocks_fd,NumToString(stock_novo),6);
                        memset(&buffilho[0], 0, sizeof(buffilho));
                        _exit(3);
                    }
                    else
                    {
                        wait(&status);
                    }
                }
            }
            codValido = 1;
            memset(&buf[0], 0, sizeof(buf));
        }
    }

    
}

void handler(int i){
    if(i==SIGINT){
        write(1,"\nServidor desconectado!\n",strlen("\nServidor desconectado!\n"));
        close(client_to_server);
        close(server_to_client);
        unlink("client_to_server_fifo");
        unlink("server_to_client_fifo");

        kill(getpid(),SIGKILL);
    }
}

int main(int argc, char *argv[])
{
    if(signal(SIGINT,handler)==SIG_ERR){
        perror("SIGINT failed");
    }

    addInfo();
    atualizarVarGlobais();
    //insereArtigos();
    sv();
}