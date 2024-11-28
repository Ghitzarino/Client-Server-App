        323CC - Ghita Alexandru - Tema 2 - PCOM

        --Aplicatie client-server TCP si UDP--


    Din punctul meu de vedere, tema s-a dovedit a fi dificila ca si dificultate,
dar am avut de invatat foarte multe din ea. A durat aproximativ 25-30 de ore pe
parcursul unei saptamani pentru a implementa toate cerintele.

Functionalitatea aplicatiei:

    Aplicatia reprezinta un sistem de subscribe si unsubscribe a unor
utilizatori la anumite topicuri pe care primesc mesaje, realizat prin
intermediul unui server, care foloseste conexiuni TCP pentru server-client si
UDP pentru server-mesaje.

    Implementarea pune in evidenta protocolul aplicatie creat peste TCP pentru a
transmite corect si eficient mesajele si pentru a abona si dezabona clientii.

Conexiunea initiala:

    Cand un client se conecteaza la server, acesta trimite un pachet de tip 
START catre server. Acest pachet contine un ID unic pentru care se realizeaza si 
memoreaza conexiunea noua.

Subscribe, unsubscribe la un topic:

    Un client poate sa se aboneze la un anumit topic, trimitand un pachet de tip 
SUBSCRIBE catre server. Acest pachet contine ID-ul clientului si numele
topicului la care doreste sa se aboneze.
    Serverul primeste acest pachet si retine intr-o structura de date toate
topicurile la care s-a abonat fiecare client, pentru a facilita trimiterea 
mesajelor catre abonati.
    Prin aceeasi metoda se realizeaza si UNSUBSCRIBE, actualizand lista de
topicuri a clientului.

Trimiterea si primirea de mesaje:

    Atunci cand un client primeste un mesaj pe un topic la care este abonat,
serverul trimite mesajul catre client. Mesajul este structurat intr-un pachet de
tip UDP_TO_CLIENT_PACKET, care contine informatii despre topic, tipul de mesaj,
mesajul, ip-ul si portul celui care trimite mesajul.

Observatii:

-Intregul cod este gestionat in mod corespunzator pentru a evita pierderea de date
sau alte probleme care pot aparea in timpul comunicarii client-server.

-Topicurile functioneaza inclusiv cu wildcarduri.

-Mesajele se trimit doar la clientii care sunt abonati la respectivul topic.

-Algorimtul lui Nagle a fost dezactivat in cod.

-Nu pot exista 2 clienti cu acelasi ID, iar daca se incearca conectarea cu un ID
deja folosit, atunci clientul se inchide automat. De asemenea, toti clientii sunt
inchisi la inchiderea serverului.

-Se respecta conversiile de byte order.

-Se afiseaza doar mesajele cerute atat in server, cat si in client.

-Numarul de clienti care se pot conecta la server este limitat, alocarea de memorie
fiind statica la pool-ul de file descriptori si la lista de topicuri.
