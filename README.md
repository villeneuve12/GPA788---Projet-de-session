# GPA788---Projet-de-session
Répertoire de tous les codes utilisés dans le cadre du cours de GPA788.
Équipe composée de Régis Villeneuve et Mathieu Bapst.
ÉTS Hiver 2019

Le projet de session consiste à créer un système d’acquisition et traitement de données d’un procédé industriel. Le procédé industriel est représenté par les signaux captés par les modules capteurs. Ces derniers sont reliés à une passerelle (gateway) par le port I2C. Dans ce projet de session, un module capteur est appelé un nœud et comprend nécessaire une carte Arduino. La passerelle qui fait le pont entre les services infonuagiques et les signaux captés est une carte Raspberry Pi (noté Pi dans ce document).

Le Pi est situé tout près des nœuds Arduino et joue le rôle de coordonnateur dans la communication I2C. C’est également le Pi qui  est responsable de transmettre les données reçues aux services ThingSpeak pour l’entreposage des données.
Les services infonuagiques ThingSpeak sont délocalisés du lieu de production des signaux. Elles permettent l’affichage rapide des données sous forme de graphiques (tableaux de bord). Puisqu’ils sont des services WEB, l’accès aux données peut se faire à distance. Ce point est important puisqu’il n’est plus nécessaire de réaliser l’acquisition des signaux et leur traitement dans les mêmes lieux. Le protocole de communication pour cette partie du projet est le style REST. Les données entreposées sont lues et traitées par des réseaux LSTM (Long Short-Term Memory network). Les LSTM sont des réseaux récurrents conçus pour traiter des données temporelles. 
