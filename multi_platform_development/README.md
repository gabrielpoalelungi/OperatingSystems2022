331CC - Poalelungi Gabriel - tema 2 SO - 3 aprilie 2022

Structura SO_FILE contine urmatoarele campuri:
    - Un buffer unde sunt stocati bytes cititi cu read();
    - Un buffer unde sunt stocati bytes care urmeaza sa fie scrisi cu write();
    - Cursorul pentru fiecare dintre buffere;
    - Ultima operatie efectuata (citire = 0, scriere = 1);
    - File descriptorul;
    - Cursorul fisierului (offset);
    - Flag-uri de eroare si EOF;
    - PID-ul procesului de care apartine;

1) SO_FILE *so_fopen(const char *pathname, const char *mode)
    - aloca memorie pentru noul fisier;
    - creaza file descriptorul in functie de mode;

2) int so_fclose(SO_FILE *stream);
    - daca sunt date in write_buffer, le da flush;
    - inchide fd-ul;
    - dezaloca memoria;
    - returneaza 0 in caz de succes, sau SO_EOF daca so_fflush sau close
    au esuat.

3) int so_fgetc(SO_FILE *stream);
    - Daca bufferul e gol, incearca sa citeasca 4096 de bytes;
    - Daca nu, citeste un byte din buffer si returneaza-l extins la int;
    - Daca se ajunge la limita bufferului (adica cati bytes a citit read()),
    reseteaza read_bufferul si returneaza SO_EOF;

4) size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream);
    - Pentru fiecare element ce trebuie citit, apelez so_fgetc() de "size" ori;
    - Fiecare byte este pus in vectorul "ptr";
    - Daca so_fgetc() propaga SO_EOF, so_fread() returneaza cate elemente a
      a apucat sa citeasca.

5) int so_fputc(int c, SO_FILE *stream);
    - Daca write_bufferul este plin, flush;
    - Pune caracterul c in write_buffer;
    - Muta cursorul o pozitie in fata;

6) size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream);
    - Pentru fiecare element ce trebuie scris, apeleaza so_fputc() de "size" ori;
    - Fiecare byte este luat din vectorul "ptr";

7) int so_fflush(SO_FILE *stream);
    - Goleste write_bufferul;
    - In caz de eroare, returneaza SO_EOF;
    - Muta cursorul fisierului;
    - Toate aceste instructiuni se executa doar daca s-a efectuat un write
    inainte;

8) int so_fseek(SO_FILE *stream, long offset, int whence);
    - Inainte sa mute cursorul, verifica daca a fost facut write ultima oara.
      Daca da, so_fflush();
    - Se muta cursorul cu lseek;
    - In caz de eroare, se returneaza SO_EOF;
    - In cazul in care cursorul este mutat mai mult decat mai are capacitate read_bufferul,
      se reseteaza read_bufferul;

9) SO_FILE *so_popen(const char *command, const char *type);
    - Se creeaza un pipe undirectional cu functia pipe();
    - Daca type-ul este read, atunci parintele inchide capatul
      de write al pipe-ului, iar copilul isi inlocuieste STDOUT-ul
      cu capatul de write al pipe-ului si inchide capatul de read;
    - Vice-versa pentru type = write;
    - Copilul executa comanda;
    - Parintele creeaza noul SO_FILE si il returneaza;
    - Copilul returneaza NULL;
    - In caz de orice eroare, returneaza NULL;

10) int so_pclose(SO_FILE *stream);
    - Flush la write_buffer;
    - Inchide fd;
    - Dezaloca memoria;
    - Asteapta procesul copil;
    - In caz de eroare, returneaza -1;
    - In caz de succes, returneaza 0;

Probleme aparute:
    - Am avut ceva probleme pe partea de write, deoarece nu verific
      cate caractere au fost scrise de write (adica, daca scrie mai putin
      de 4096 de caractere, eu tot resetez bufferul)
    - I have no idea how to solve that and succeed.