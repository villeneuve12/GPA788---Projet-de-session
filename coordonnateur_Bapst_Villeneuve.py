#!/usr/bin/python3
# ex_i2c_coord_v2.py
#Ce programme combine les programmes test Leq et test TempHum

import smbus            # pour la communication I2C
import time             # pour sleep
import struct           # pour la conversion octet -> float
import requests    #pour l'envoi des données sur Thingsspeak
from datetime import datetime    # pour l'horodatage des échantillons

# ------------------------------------------------------
# Des constantes pour le fonctionnement du coordonnateur
# ------------------------------------------------------

# Temps d'échantillonnage du coordonnateur. Il doit être
# supérieur au temps d'échantillonage du noeud.
sampling_time = 60      # Ts du coordonnateur, à quel moment on recupere des données
Ts_noeud_TH = 30              # nouvelle Ts pour le noeud Temphum 
nbLi=60             # nombre de li pour le calcul de Leq
# Adresse IC2 des noeuds
i2c_address_TH = 0x44   #noeud TempHum
i2c_address_Leq=0x45   #noeud Leq

#Thingspeak
THINGSPK_URL = 'https://api.thingspeak.com/update/'
THINGSPK_API_KEY = 'IW31QQLRLN97OGVJ'


# Commandes possibles entre le coordonnateur et le noeud
i2c_cmd_set_Ts = 0xA0   # Changer le temps d'échantillonnage pour TempHum
i2c_cmd_set_Stop = 0xA1 # Arrêter l'échantillonnage
i2c_cmd_set_Go = 0xA2   # Démarrer l'échantillonnage
i2c_cmd_available=0xA3 #demande de dispo
# Commandes possible entre le coordonnateur et le noeud
i2c_cmd_set_nbLi = 0xA0   # Changer le nombre de Li

# Adresse des registres sur le noeud TempHum. Elle correspond
# à la carte des registres du noeud.
i2c_node_Ts_LSB = 0         # Temps d'échantillonnage (2 octets)
i2c_node_Ts_LSB = 1
i2c_temp_LSB0 = 2  # Temperature interne du processeur
i2c_temp_LSB1 = 3  # (float sur 4 octets)
i2c_temp_MSB0 = 4
i2c_temp_MSB1 = 5
i2c_hum_LSB0=6
i2c_hum_LSB1=7
i2c_hum_MSB0=8
i2c_hum_MSB1=9

#pour Leq
# Adresse des registres sur le noeud. Elle correspond
# à la carte des registres du noeud.
i2c_node_nbLi_LSB = 0         # nbLi (2 octets)
i2c_node_nbLi_MSB=1
i2c_node_Leq_LSB0 = 2  # Temperature interne du processeur
i2c_node_Leq_LSB1 = 3  # (float sur 4 octets)
i2c_node_Leq_MSB0 = 4
i2c_node_Leq_MSB1 = 5


# Liste de 4 octets qui servira à la conversion
# octets -> float reçus du noeud
T = bytearray([0x00, 0x00, 0x00, 0x00])
H=bytearray([0x00, 0x00, 0x00, 0x00])

#pour Leq
Leq = bytearray([0x00, 0x00, 0x00, 0x00])

# ------------------------------------------------------
# Programme principal
# ------------------------------------------------------

# Bon. Indiquer que le coordonnateur est prêt...
print("Coordonnateur (Pi) en marche avec Ts =", sampling_time, "sec.")
print("ctrl-c pour terminer le programme.")


# Instancier un objet de type SMBus et le lié au port i2c-1
bus = smbus.SMBus(1)

#demander si le noeud TempHum est présent
try:
    bus.write_byte(i2c_address_TH, i2c_cmd_available)
    etat=bus.read_byte(i2c_address_TH)
    if (etat==1):   #si le noeud renvoie vrai
        print("Noeud TempHum disponible")
        status_TH=True
    else:
        print("Noeud TempHum indisponible")
        status_TH=False
except IOError:
    print("Noeud TempHum indisponible")
    status_TH=False

#test du noeud Leq
try:
    bus.write_byte(i2c_address_Leq, i2c_cmd_available)
    etat=bus.read_byte(i2c_address_Leq)

    if (etat==1):
        print("Noeud Leq disponible")
        status_Leq=True
    else:
        print("Noeud Leq indisponible")
        status_Leq=False
except IOError:
    print("Noeud Leq indisponible")
    status_Leq=False


#Arrêtez les noeuds
if status_TH==1:
    print('Arrêter le noeud TempHum.')
    bus.write_byte(i2c_address_TH, i2c_cmd_set_Stop)

# 2) Régler le temps d'échantillonnage du noeud 
#    
    print("Assigner une nouvelle Ts =",  Ts_noeud_TH, "au noeud TempHum.")
    bus.write_i2c_block_data(i2c_address_TH, i2c_cmd_set_Ts, [Ts_noeud_TH])
    time.sleep(0.1)         # attendre avant de continuer l'écriture

# 3) Ok. Demander au noeud de démarrer son échantillonnage avec
#    la nouvelle 
    bus.write_byte(i2c_address_TH, i2c_cmd_set_Go)
    print("Démarrer le noeud TempHum")


#de même avec le deuxième noeud
if status_Leq==1:
    print('Arrêter le noeud Leq.')
    bus.write_byte(i2c_address_Leq, i2c_cmd_set_Stop)

# 2) Régler le temps d'échantillonnage du noeud 
#    
    print("Assigner un nouveau nombre de li =",  nbLi, "au noeud Leq.")
    bus.write_i2c_block_data(i2c_address_Leq, i2c_cmd_set_nbLi, [nbLi])
    time.sleep(0.1)         # attendre avant de continuer l'écriture

# 3) Ok. Demander au noeud de démarrer son échantillonnage avec
#    la nouvelle Ts
    bus.write_byte(i2c_address_Leq, i2c_cmd_set_Go)
    print("Démarrer le noeud Leq")


# 4) Le coordonnateur demande et reçoit des données du noeud
#    jusqu'à ce que l'utilisateur arrête le programme par ctrl-c.
try:
    while True:      
        time.sleep(sampling_time)   # boucle infinie temporisée
        if status_TH:  #si le noeud est disponible
            #température
            bus.write_byte(i2c_address_TH, i2c_temp_MSB1)
            T[3] = bus.read_byte(i2c_address_TH)
            bus.write_byte(i2c_address_TH, i2c_temp_MSB0)
            T[2] = bus.read_byte(i2c_address_TH)
            bus.write_byte(i2c_address_TH, i2c_temp_LSB1)
            T[1] = bus.read_byte(i2c_address_TH)
            bus.write_byte(i2c_address_TH, i2c_temp_LSB0)
            T[0] = bus.read_byte(i2c_address_TH)
                # pour humidité 
            bus.write_byte(i2c_address_TH, i2c_hum_MSB1)
            H[3] = bus.read_byte(i2c_address_TH)
            bus.write_byte(i2c_address_TH, i2c_hum_MSB0)
            H[2] = bus.read_byte(i2c_address_TH)
            bus.write_byte(i2c_address_TH, i2c_hum_LSB1)
            H[1] = bus.read_byte(i2c_address_TH)
            bus.write_byte(i2c_address_TH, i2c_hum_LSB0)
            H[0] = bus.read_byte(i2c_address_TH)

            temperature = struct.unpack('f', T)
            humidity=struct.unpack('f',H)

            print("Température: ", temperature[0])
            print("Humidité: ", humidity[0])
    
        if status_Leq:
            #pour le niveau sonore
            bus.write_byte(i2c_address_Leq, i2c_node_Leq_MSB1)
            Leq[3] = bus.read_byte(i2c_address_Leq)
            bus.write_byte(i2c_address_Leq, i2c_node_Leq_MSB0)
            Leq[2] = bus.read_byte(i2c_address_Leq)
            bus.write_byte(i2c_address_Leq, i2c_node_Leq_LSB1)
            Leq[1] = bus.read_byte(i2c_address_Leq)
            bus.write_byte(i2c_address_Leq, i2c_node_Leq_LSB0)
            Leq[0] = bus.read_byte(i2c_address_Leq)

            niveau_sonore_eq = struct.unpack('f', Leq)

            print("Niveau sonore équivalent", niveau_sonore_eq[0])


        #envoi sur thingspeak si seul le noeud TempHum est dispo
        if status_TH and not status_Leq:
            try:
                print('écrire Temp et Hum dans le canal')         
                resp = requests.get(THINGSPK_URL,
                                    timeout = (10, 10),
                                    params = { "api_key" : THINGSPK_API_KEY,
                                            "field1"  : temperature[0],
                                            "field2"  : humidity[0]})
                                            
                                    
        
                print("ThingSpeak GET response: ", resp.status_code)
                if resp.status_code != 200:
                    print("Erreur de communication détectée!")
                
            except requests.Timeout:
                print("Exception de timeout reçue (connexion ou écriture)")
        
        #envoi sur thingspeak si seul le noeud Leq est dispo
        if status_Leq and not status_TH:
            try:
                print('écrire Leq dans le canal')         
                resp = requests.get(THINGSPK_URL,
                                    timeout = (10, 10),
                                    params = { "api_key" : THINGSPK_API_KEY,
                                            "field3"  : niveau_sonore_eq[0]})
                                            
                                    
        
                print("ThingSpeak GET response: ", resp.status_code)
                if resp.status_code != 200:
                    print("Erreur de communication détectée!")
                
            except requests.Timeout:
                print("Exception de timeout reçue (connexion ou écriture)")

        #envoi sur thingspeak si les 2 noeuds sont dispos
        if status_Leq and status_TH:
            try:
                print('écrire Temp, Hum et Leq dans le canal')         
                resp = requests.get(THINGSPK_URL,
                                    timeout = (10, 10),
                                    params = { "api_key" : THINGSPK_API_KEY,
                                            "field1"  : temperature[0],
                                            "field2"  : humidity[0],
                                            "field3"  : niveau_sonore_eq[0]})
                                            
                                    
        
                print("ThingSpeak GET response: ", resp.status_code)
                if resp.status_code != 200:
                    print("Erreur de communication détectée!")
                
            except requests.Timeout:
                print("Exception de timeout reçue (connexion ou écriture)")
        
       

except KeyboardInterrupt:
    # 5) Ctrl-c reçu alors arrêter l'échantillonnage
    if status_TH:
        bus.write_byte(i2c_address_TH, i2c_cmd_set_Stop) 
    if status_Leq:
        bus.write_byte(i2c_address_Leq, i2c_cmd_set_Stop)
    

