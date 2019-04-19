# Lire les données Temp, Hum et Leq de Thingspeak
#puis entrainer un réseau LSTM pour prédire les valeurs futures des 3 signaux
import requests, sys
from conversion_signaux import signaux_apprentissage_supervise as ss
from math import sqrt, ceil
import numpy as np
from matplotlib import pyplot
from pandas import read_csv
from pandas import DataFrame
from pandas import concat
from sklearn.preprocessing import MinMaxScaler
from sklearn.preprocessing import LabelEncoder
from sklearn.metrics import mean_absolute_error
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense
from tensorflow.keras.layers import LSTM
from tensorflow.python.keras.callbacks import EarlyStopping


# ThingSpeak
THINGSPK_CHANNEL_ID = '717154'
THINGSPK_URL = 'https://api.thingspeak.com/channels/' + THINGSPK_CHANNEL_ID + '/feeds.json'

THINGSPK_READ_API_KEY = 'HBKPYI7YUFEDQVRH'

nb_echantillons=5300

nb_etats_passes=1
nb_futurs_prediction=1

#nombre de cellules mémoires
nb_cellules=50

#proportion de données utilisée pour l'apprentissage. exemple: 0,4 --> on utilise
# 40% des données pour l'apprentissage et 60% pour la validation
proportion_app_val=0.8

nb_epochs=500  #nb d'epochs
batch_size=20   #taille de lot


#récupérer les données sur Thingspeak
try:
    resp = requests.get(THINGSPK_URL,
                        timeout = (10, 10),
                        params = { "api_key" : THINGSPK_READ_API_KEY, "results"  : nb_echantillons})
    print('La requête est:', resp.url)
    print("ThingSpeak GET response: ", resp.status_code)
    if resp.status_code != 200:
        print("Erreur de communication détectée!")
        sys.exit(1)
    # Décoder la réponse de JSON à Python
    try:
        r = resp.json()
        datas=list()
        for d in r['feeds']:
            
            datas.append(d['field1'])
            datas.append(d['field2'])
            datas.append(d['field3'])
            
          
    except requests.ValueError:
        print('Exception ValueError reçue (décodage JSON)')
        sys.exit(2)
except requests.Timeout:
    print("Exception de timeout reçue (connexion ou écriture)")
    sys.exit(3)

#création du tableau de valeurs
data=np.array(datas)
data=np.reshape(data,(nb_echantillons,3))
lignes=[j for j in range(nb_echantillons-50,nb_echantillons)]  #on supprime les dernières
#données qui ne sont pas significatives
data=np.delete(data,lignes,axis=0) 
df=DataFrame(data)
df.dropna(axis=0, inplace=True)    #on supprime les éventuelles lignes qui contiennent des nan
#dues à des erreurs lors de l'échantillonnage
data=df.values
data=data.astype('float32')  #ok
temp=data[:,0]
max_temp=np.max(temp)   #on recuille le min et le max de la température afin de pouvoir effectuer une 
#remise à l'échelle lors de la présentation des résultats
min_temp=np.min(temp)
print("max temp",max_temp)
print("min temp",min_temp)

#normalisation des valeurs
scaler = MinMaxScaler(feature_range = (0, 1))
data_normalisees = scaler.fit_transform(data)
#decalage methode fenetre glissante avec la fonction conversion_signaux
data_converties = ss(data_normalisees, nb_etats_passes, nb_futurs_prediction, enlever_nan=True)
#enlever les valeurs des colonnes de sortie si l'on ne souhaite pas predire tous les signaux
data_converties.drop(data_converties.columns[-2:], axis = 1, inplace = True)
#créer un ensemble d'apprentissage et un ensemble de validation
apprentissage=data_converties.values[:ceil(proportion_app_val*nb_echantillons), :]
validation=data_converties.values[ceil(proportion_app_val*nb_echantillons):, :]
print("taille apprentissage",np.size(apprentissage))
print("taille val",np.size(validation))


#pour chaque ensemble, separation entree/sortie 
#et séparation apprentissage/validation

app_X=apprentissage[:, :-1]
app_Y=apprentissage[:, -1:]
val_X=validation[:, :-1]
val_Y=validation[:, -1:]


#redimensionnement des entrées
app_X=app_X.reshape((app_X.shape[0],1,app_X.shape[1]))
val_X=val_X.reshape((val_X.shape[0],1,val_X.shape[1]))

#création et compilation du réseau
reseau=Sequential()
reseau.add(LSTM(nb_cellules,return_sequences=True, input_shape=(app_X.shape[1], app_X.shape[2])))
reseau.add(LSTM(nb_cellules,return_sequences=True))
reseau.add(LSTM(nb_cellules,return_sequences=True))
reseau.add(LSTM(nb_cellules))
reseau.add(Dense(1))
reseau.compile(loss='mae', optimizer='adam')

#entrainer le reseau
fonction_es = EarlyStopping(monitor='val_loss', mode = 'min', verbose=1, patience = 10) #fonction d'early stopping
history=reseau.fit(app_X, app_Y, epochs = nb_epochs, batch_size = batch_size,\
                    validation_data = (val_X, val_Y), verbose = 1, shuffle = False,callbacks=[fonction_es])



#courbes et résultats

mae=0
temp_predit = reseau.predict(val_X, verbose = 0)
temp_predit_echelle=temp_predit*(max_temp-min_temp)+min_temp  #remise à l'échelle pour obtenir des températures
val_Y_echelle=val_Y*(max_temp-min_temp)+min_temp
mae += mean_absolute_error(val_Y, temp_predit)
print("MAE : ",mae)
#affichage de la prédiction et de la température réelle
pyplot.figure(1)
pyplot.plot(temp_predit_echelle,label='prediction')
pyplot.plot(val_Y_echelle,label='reelle')
pyplot.legend()
#affichage de l'erreur en °C
pyplot.figure(2)
erreur=abs(val_Y_echelle-temp_predit_echelle)
pyplot.plot(erreur,label='erreur')
pyplot.legend()
pyplot.show()