Le processus init-cleaner remplace le processus init d'une cage. Il est chargé
de lancer une commande initiale (e.g. service de filtrage/parsing) et permet,
par la suite, de reinitialiser sa cage dans un etat sain, identique a l'etat
initial (tue tous les processus et relance la commande de service).  L'ordre de
"reset" de la cage peut etre envoye par un processus avec un GID approprie par
un socket unix.

Afin qu'il ne reste pas de donnees remanentes, cette version necessite que la
cage n'ait qu'un systeme de fichier en lecture-seule. Par la suite, une
fonctionnalite de nettoyage des espaces inscriptibles est envisageable.

usage: ./init-cleaner -s <socket-path> -a <allowed-launcher-gid> -u <spawned-process-uid> -g <spawned-process-gid> -- /path/to/bin args...
