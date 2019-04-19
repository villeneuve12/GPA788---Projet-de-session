/*
 * TempHum.cpp
 * Définition de la classe TempHum.
 * La classe TempHum englobe un objet dht afin d'échantillonner 
 * périodiquement la température et l'humidité relative. De plus,
 * TempHum est capable de comptabiliser la valeur min, max, moyenne
 * de la température et de l'humidité relative.
 * 
 * GPA788 - ETS
 * T. Wong
 * 07-2018
 * 
 * modifié par M.Bapst
 */
#include "TempHum.h"

// Déclarer les membres statiques
int TempHum::dPin;
volatile float TempHum::temp;
volatile float TempHum::humidity;
float TempHum::minT;
float TempHum::maxT;
volatile float TempHum::averageT;
float TempHum::minH;
float TempHum::maxH;
volatile float TempHum::averageH;
volatile float TempHum::sumT;
volatile float TempHum::sumH;
unsigned int TempHum::sampleCount;
int TempHum::WindowLen;
volatile dht TempHum::dht11;

volatile float* TempHum::bufferH;
volatile float* TempHum::bufferT;
volatile bool TempHum::go;



TempHum::TempHum(int Pin) { 
  dPin = Pin;
  WindowLen = 1;
  minT = 3.4e10f;
  maxT = -3.4e10f;
  averageT = 0.0f;
  minH = 3.4e10f;
  maxH = -3.4e10f;
  averageH = 0.0f;
  sampleCount = 0;
  startMillis=0;
  Ts=4; //valeur par défaut
  go=false;
  
  
  
}



void TempHum::stop() {
  //Timer1.stop();
  //Ts=0;
  
  delete[] bufferT;  //on supprime les pointeurs
  delete[] bufferH;
  bufferT=NULL;
  bufferH=NULL;
  go=false;
}




// Démarrer l'échantillonnage
bool TempHum::start() {
  if (Ts > 0) {
    
    go=true;
    startMillis=millis();
    bufferT=new volatile float[WindowLen];
    bufferH=new volatile float[WindowLen];
    for (int i =0;i<WindowLen;i++){
      bufferT[i]=0;
      bufferH[i]=0;
    }
    
    return true;
  }
  else
    return false;
}


bool TempHum::results() {
  //Serial.println(Ts);
  if (millis()-startMillis>=Ts*1000 && Ts>0&& go){
    read_dht();
    startMillis=millis();
    return true;
  } else {
    return false;
  }
}
// Fonction de lecture du capteur DHT11
//la moyenne sera une moyenne flottante
void TempHum::read_dht() {
  if (dPin > 0) {
    
    //création du buffer circulaire pour le calcul de la moyenne flottante
    
    dht11.read11(dPin);
    
    temp = dht11.temperature;
    //Serial.println(temp);
    sumT = sumT+temp-bufferT[WindowLen-1];  //on ajoute la dernière valeur de la température et on soustrait la première valeur
    humidity = dht11.humidity;
    sumH = sumH+humidity-bufferH[WindowLen-1];
    // Comptabiliser min et max de la température
    maxT = temp > maxT ? temp : maxT;
    minT = temp < minT ? temp : minT;
    //min et max de l'humidité
    maxH = humidity > maxH ? humidity : maxH;
    minH = humidity < minH ? humidity : minH;
  for (int i =WindowLen-1; i>0; i--){
      bufferT[i]=bufferT[i-1];  //décalage des données dans le buffer
      bufferH[i]=bufferH[i-1];
      
    }
    bufferH[0]=humidity;
    bufferT[0]=temp;
    if (++sampleCount == WindowLen)
    {
    averageT =sumT/WindowLen; 
    averageH =sumH/WindowLen;
    sampleCount--; 
    }
  }
}


