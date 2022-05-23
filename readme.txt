Naumencu Mihai
336 CA

Tema2 Protocoale de Comunicatii


subscriber.c:
	Implemetarea aplicatiei client a serviciului
	Dupa initializarea socketilor si a diverselor variabile folosite in rezolvare,
	creez conexiunea cu serverul (folosind connect()) si trimit pentru prima data
	catre server ID-ul clientului curent, pentru ca acesta sa-l afiseze.

	In bulca 'while' preiau comenzile de la tastatura, aceastea pot fi:
	- exit:
		Caz in care trimit serverului un pachet cu continutul "exit\n" +
		<ID_CLIENT> (2 stringuri concatenate), pentru ca serverul sa stie
		ce client s-a deconectat, dupa care opresc procesul clientului
		(ies din while dandu-i variabilei 'client_UP' valoarea 'false');
	- subscribe:
		Caz in care parsez inputul primit de la tastatura, avand grija ca
		acesta sa aiba formatul corect (in caz contrar afisez utilizatorului
		un mesaj cu formatul adecvat), folosindu-ma de functia 'strtok' si de
		o structura mesaj (definita in helpers.h). Daca comanda a fost 
		introdusa corect de catre utilizator, trimit pachetul comanda catre
		server;
	- unsubscribe:
		Similar cu subscribe, singura diferenta fiind ca aceasta comanda
		nu va avea sf setat (folosesc aceeasi structura de comanda, doar
		ca in cazul comenzii unsubscribe, serverul nu va tine cont de
		campul 'sf' al structurii);
	sau primesc un mesaj dinspre server (mesajul trimis initial de catre
	clientul UDP si forwardat de catre server), caz in care formatez campul
	'data_type' al structurii primite (restul campurilor: ip, portno, topic
	si content sunt primite direct in formatul de afisare), strucutara numita
	'UDP2TCP' si definita in fisierul helpers.h


server.c:
	Implementarea aplicatiei server a serviciului
	Dupa initializarea variabilelor si listelor folosite in implementare,
	a socketilor clientilor (bind pe UDP si TCP si listen pe TCP), a
	socketului serverului, a file descriptor-ilor aplicatia intra in
	bulca 'while' (care ruleaza atat timp cat starea serverului este
	'UP' - folosind variabila 'server_UP').
	In bucla 'while' trec prin toti socketii setati ('FD_ISSET(i, &tmp_fds'),
	dupa care verific 4 cazuri, si anume:
	- i este 0 (serverul primeste comanda de la stdin):
		In acest caz, doar comanda 'exit' este valabila si acceptata de
		catre server, daca aceasta comanda este primita, aplicatia server
		se inchide, odata cu toti clientii logati;

	- i este egal cu socketul clientului UDP:
		In acest caz, serverul primeste un pachet de la clientul UDP,
		dupa care il formateaza pentru a putea fi trimis care clientul
		TCP, decodand content-ul pachetului (datagrama), in functie de
		tipul acestuia ('INT', 'SHORT_REAL', 'FLOAT' sau 'STRING').
		Dupa formatare se trimit mesajele clientilor online si abonati la
		topic respectiv si se stocheaza mesajele pentru cleintii offline
		care sunt abonati la topic cu optiunea SF selectata (SF == 1);

	- i este egal cu socketul clientului UDP:
		In acest caz, serverul primeste o cerere de conexiune de la un
		client TCP. 
		- 	Daca ID-ul acestui client nu este inregistrat in lista
			de clienti logati (ce pot afi atat online cat si offline), atunci
			el este logat (ca un nou client).
		-	Daca Id-ul este inregistrat pe in lista, atunci se verifica daca
			clientul cu ID-ul respectiv este sau nu online.
			- Daca este online, atunci clientului ce incearca sa se conecteze
			nu i se va permite acest lucru si va fi deconectat instant
			- Daca nu este online, atunci clientul se conecteaza, caz in care
			va primi mesajele la care a fost abonat si pentru care avea optiunea
			de SF activata (daca a fost trimis vreun mesaj pe topicurile
			respective);

	- i este diferit de primele 3 cazuri:
		In acest caz, un client TCP trimite o comanda catre server, acestea pot
		fi de 3 tipuri, si anume:
		- 'exit':
			Caz in care clientul este deconectat de pe server (e trecut in 
			lista clientilor ca fiind offline si socket respectiv este inchis);
		- 'subscribe':
			Caz in care clientul este abonat la topicul cerut (se adauga un nou
			element in lista de abonamente a clientului respectiv sub forma
			<TOPIC, SF>. daca SF == 0, asta inseamna ca optiunea de SF nu 
			a fost selectata pentru abonamentul respectiv);
		- 'unsubscribe':
			Caz in care clientul este dezabonat de la topicul cerut (se scoate 
			un element din lista abonamentelor clientului);


helpers.h:
	Fisiser header, acesta contine include-uri, define-uri, structuri si 
	functii folositoare, de exemplu:
	- 'DIE':
		functie folosite pentru tratarea erorilor
	- 'max()':
		functie de maxim care nu tine cont de tip
	- 'remove_message()':
		functie folosita pentru inlaturarea unui mesaj dn lista de mesaje
		ce urmeaza a fi trimise catre clientii ce folosesc SF
	- structurile folosite in implementarea protocolului:
		'message' (mesaj primit de la clientul UDP), 
		'queued_message' (mesaj ce urmeaza a fi trimis catre cleintii ce 
						folosesc SF atunci cand acestia se logheaza din nou), 
		'UDP2TCP' (pachet formatat de catre server pentru a fi trimis de la
					clientul UDP catre clientul TCP), 
		'subscribe_command' (comanda de subscribe primita de la tastatura de 
					catre clientul TCP),
		'client' (informatiile unui client TCP - id, statut (on/off line), lista
					de abonamente (de forma <TOPIC, SF>))

map.h, map.c:
	fisiere ce implemeteaza un map neordonat in C

Sursa map: https://github.com/rxi/map
Sursa max: https://stackoverflow.com/questions/3437404/min-and-max-in-c












PS: Cand rulez de mana, imi merge si functionalitatea la care imi mai da fail
	checkerul (la testele 'same_id' si 'c2_restart_sf'), posibil sa fie o problema
	de timp, dar mi se pare ciudat, avand in vedere ca testul cu quick_flow merge 
	cum trebuie). Nu stiu cine esti cel/cea care imi corecteaza, dar daca te uiti
	peste tema si iti dai seama de problema iti raman dator cu o bere, ca eu am
	stat vreo 4 ore si nu am reusit :)) (nu vreau punctele, doar sa aflu si eu care
	e problema)





