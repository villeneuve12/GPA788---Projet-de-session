#ifndef _TEMP_HUM
#define _TEMP_HUM
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
 * Modifié par M.Bapst
 */
//#include <TimerOne.h> on supprime le Timer car il utilise des interruptions
#include <dht.h>

class TempHum {
public:  
  TempHum(int Pin); //constructeur
TempHum() = default; //constructeur si on veut pas spécifier la pin tout de suite   . pour creer une composition avec cette classe
~TempHum()=default;  //destructeur

  // Services offerts (voir cas d'utilisation)      assesseurs et mutateurs
  void setDigitalPin(int Pin) { dPin = Pin; }
  int getDigitalPin() const { return dPin; }
  void setTs(unsigned int t) { Ts = t; }
  unsigned long getTs() const { return Ts; }
  void setWindowLength(unsigned int len) { WindowLen = len; }
  unsigned int getWindowLength() const { return WindowLen; }
  unsigned long getStartMillis() const {return startMillis;}
  bool start();
  void stop();
  bool results();
  float getTemperature() const { return temp; }
  float getHumidity() const { return humidity; }
  float getAverageT() const { return averageT; }
  float getAverageH() const { return averageH; }
  float getTempMax() const { return maxT; };
  float getTempMin() const { return minT; };
  float getHumMax() const { return maxH; };
  float getHumMin() const { return minH; };
  
  // Cette fonction membre statique sera exécutée
  // périodiquement par le timer
  static void read_dht();

 
  
  
private:


  
  static volatile dht dht11;            // Objet DHT    pour réaliser la composition
  static int dPin;            // Broche numérique pour DHT11
 unsigned int Ts;         // Temps d'échantillonnage
  static int WindowLen;    // Largeur fenêtre   max 50 éléments

 static unsigned int sampleCount;  // Nb. échantillons lus
 static volatile float temp, humidity;
 static float minT, maxT, minH, maxH;
 static volatile float averageT, averageH;
 static volatile float sumT, sumH;
 
//on utilise des buffers pour la température et l'humidité afin de réaliser une moyenne mobile des ces grandeurs.
//On utilise des pointeurs car la taille des buffers est réglable en fonction du nombre d'échantillons choisi pour réaliser la moyenne mobile
static volatile float* bufferT=0; 
static volatile float* bufferH=0;
volatile unsigned long startMillis;
volatile static bool go;

 
 

};

#endif
