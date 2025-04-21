# Sherlock 13 - OS-User
This project was part of my training at Polytech Sorbonne engineering school. The following is a readme, somewhat of a report, in french.

## Compilation
Ce projet a été utilisé et compilé avec gcc 13.3.0 sur Ubuntu 24.04. Il utilise des librairies de SDL 2. Voici comment les installer (Ubuntu uniquement) :
 - `sudo apt install libsdl2-dev `
 - `sudo apt install libsdl2-image-dev `
 - `sudo apt install libsdl2-ttf-dev `
 
 Si tout s'est installé correctement, vous aurez simplement à exécuter le makefile.
 
 (je viens de me rappeler que vous êtes sous Mac : essayez avec 
 - `brew install sdl2`
 - `brew install sdl2_image`
 - `brew install sdl2_ttf`

et ça devrait fonctionner correctement :))

Vous n'aurez alors plus qu'à exécuter `make` !

## Fonctionnement général
Pour lancer le jeu, vous aurez besoin de 5 instances de votre terminal :
- Un pour le serveur
- 4 pour les clients.

Pour lancer le serveur, exécutez `make run_server`. Il se lancera sur le port spécifié dans le makefile

Pour les autres clients, exécutez `make run_client1`,`make run_client2`, `make run_client3` et `make run_client4` sur les autres instances du shell. Vous pourrez alors vous connecter.

## Comment jouer ?
Attendez que tous les joueurs soient connectés au serveur.
Alors, le jeu se lance. Lorsque c'est votre tour, vous pouvez choisir entre deux enquêtes et une accusation.

Pour enquêter, choisissez un objet en le sélectionnant (il est alors surligné en vert). Vous aurez alors soit '9' soit '0' comme réponse : si quelqu'un ne possède pas l'objet, sa case se remplit d'un zéro. S'il possède l'objet, la case est remplie d'un neuf. Le maximum de chaque objet étant entre 2 (si le coupable possède l'un des objets dont le maximum est 3) et 5, 9 n'est en pratique jamais possible.

Si vous souhaitez enquêter sur un joueur en lui demandant combien de fois celui-ci possède un objet spécifique, sélectionnez en même temps l'objet et le joueur de manière que les deux soient surlignés.

Pour accuser un personnage, sélectionnez le nom du personnage et cochez sa case.

Pour valider votre choix, appuiez sur "go !" deux fois consécutives (un petit bug...)
Votre tour est alors validé.

## Breakdown du code

### Serveur
Le serveur gère 4 clients. On n'utilise qu'une instance du serveur par partie. De plus, aucune des fonctions ne renvoie un pointeur ce qui souligne qu'il fonctionnerait en single-thread.
On observe qu'il fonctionne avec une machine à états finis (avec deux états). Un pour attendre la connexion de tous les joueurs, un pour la gestion du jeu.
Comme il fonctionne en single thread et sans pipe (car pas non plus de fork), il n'y a aucune utilité pour un mutex et il fonctionne sur un seul processus.

On note que le premier état, qui sert à attendre la connexion des joueurs, utilise les aspects déjà vus au module Réseaux du semestre précédent. En effet, on se connecte en TCP, on créera un socket de communication avec l'appel système `socket(AF_INET, SOCK_STREAM, 0);`.  On le lie ensuite au port qu'on souhaite utiliser, qui est précisé lors de l'exécution du serveur : `bind(sockfd, (struct  sockaddr  *) &serv_addr, sizeof(serv_addr))`.  On écoute les messages entrants avec `listen(sockfd,5);` puis on les accepte avec `accept(sockfd, (struct  sockaddr  *) &cli_addr, &clilen);`. Enfin, dans la boucle infinie, on lit systématiquement 255 caractères de ce que les clients nous envoient comme messages avec `read(newsockfd,buffer,255);` En principe, on n'aura pas de problème de dépassement car les messages sont tous très largement <255 caractères. On répond au cas par cas avec `sendMessageToClient()` ou `broadcastMessage()` qui utilisent dans les deux cas l'appel système `write(sockfd,buffer,strlen(buffer));.`

Le deuxième état gère toute la partie. On travaille essentiellement avec la structure de données tcpClients, [que j'ai enrichie d'un dernier élément, à savoir si le joueur est éliminé (dû à une mauvaise accusation) ou non,] et les réponses des joueurs. Globalement, si ce n'est pas le tour d'un joueur, le programme rejette sa requête et attend la requête du joueur dont c'est le tour. On check aussi si le joueur est éliminé (par mauvaise accusation). Si c'est le cas, on passe son tour, puis on attend le coup du prochain joueur. Même si ce n'est pas à son avantage, le joueur peut envoyer une requête qui est déjà satisfaite (par exemple, le joueur 1 peut enquêter pour savoir combien le joueur 1 a de crânes...), le serveur traîte cette requête comme une requête valide. Si le joueur accuse à tort un personnage, son tour est automatiquement passé. Lorsqu'un joueur fait une bonne accusation, la partie se termine.

### Client



## Perspectives d'amélioration
- Les performances sont assez mauvaises. L'interface graphique se met à jour assez lentement et réagit à son rythme lorsqu'on clique sur les éléments qu'on souhaite choisir.
- La terminaison de la partie est à revoir. Les sockets ne sont pas fermés correctement côté serveur et ça occasionne un comportement assez chaotique lors de la fin de partie.
