
#include "TempHum.h"
#include <Wire.h>
#include <util/atomic.h>
/* ------------------------------------------------------------- */
/* Constantes et variables globales                              */
/* ------------------------------------------------------------- */

// a integrer eventuellement :
// moyenne temp et moyenne hum et longueur de fenetre

enum commande {   //pour démarrer ou arrêter le noeud, il suffit de modifier l'enum commande
  Go,Stop
};

const unsigned long SERIAL_BAUDRATE{115200};     // Communication série
const int DIGITAL_PIN = 8;


//pour i2c
const byte ADR_NOEUD=0x44; //adresse du noeud
const unsigned int NB_REGISTRES=10;
volatile commande cmd=Stop;
volatile bool etat=false;

volatile byte adrReg;
volatile bool ask=false;


union CarteRegistres {
  struct {
    volatile unsigned int TS; //periode echantillonnage  : 2 octets
    volatile float temp;   //valeur de temperature   : 4 octets
    volatile float hum;   //Valeur humidite   :4 octets
  } champs;

  byte regs[NB_REGISTRES];
};



/* ------------------------------------------------------------- */
/* Créer un objet de type TempHum                                */
/* ------------------------------------------------------------- */
TempHum myTempHum(DIGITAL_PIN);


//créer la carte de registres
union CarteRegistres crTH;



void setup() {
  
  // Ouvrir le terminal série
  Serial.begin(SERIAL_BAUDRATE);

//configurer la communication i2c
  Wire.begin(ADR_NOEUD);
  Wire.onReceive(i2c_receiveEvent);
  Wire.onRequest(i2c_requestEvent);
}

void loop() {
  //teste s'il y a un demarrage ou un arret demande
  if (etat==true) {
    if (cmd==Go){
      myTempHum.start();
      Serial.println(F("demarrage"));
    } else {
      myTempHum.stop();
    }
    etat=false;
  }
  
  if (myTempHum.results()){   //on teste si un nouveau résultat est disponible
    
    Serial.print(F("La Température est: "));
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){   //bloc interrupt_safe
      crTH.champs.temp=myTempHum.getTemperature();
      crTH.champs.hum=myTempHum.getHumidity();
      Serial.print(myTempHum.getTemperature());
      Serial.print(F(" et l'humidité est: "));
      Serial.println(myTempHum.getHumidity());
    }
  }
 

}

//interruption i2c de réception de commande de la part du coordonnateur
void i2c_receiveEvent (int cmb){
  if (cmb==1){
    byte data=Wire.read();
    switch (data) {
     case 0xA1:    //commande arrêter le noeud
     cmd = Stop;
     etat=true;
     Serial.println(F("commande 'Arrêter' reçue"));
     break;
     case 0xA2:    //commande démarrer le noeud
     cmd = Go;
     etat=true;
     Serial.println(F("Commande 'Démarrer' reçue"));
     break;
     case 0xA3:    //demande si le noeud est disponible
     Serial.println(F("Demande de disponibilité"));
     ask=true;
     break;
     default:
    // Sinon, c'est probablement une adresse de registre
    if ((data >= 0) && (data < NB_REGISTRES)) {
    adrReg = data;
    }
    else
 adrReg = -1; // Il y sans doute une erreur!
 }
 }
 else if (cmb == 2) {
 // Deux octets reçu. C'est probablement pour changer le
 // taux d'échantillonnage.
 byte data1 = Wire.read();
 byte data2 = Wire.read();
 //byte data3=Wire.read();
 
 Serial.println(F("Commande 'Changer Ts' reçue"));
 //Serial.println(data1); Serial.println(data2);
 if ((data1 == 0xA0)) {
 crTH.champs.TS = data2; 
 myTempHum.setTs((unsigned int)crTH.champs.TS);
 Serial.print(F("La nouvelle valeur est: ")); 
 Serial.print(crTH.champs.TS); Serial.println(F(" secondes"));
 }
 }
  }


//interruption i2c d'envoi de donnée
void i2c_requestEvent(){
 // Le coordonnateur veut la valeur d'un registre. L'adresse du
 // registre a été reçue précédemment.
 if ((adrReg >= 0) && (adrReg < NB_REGISTRES)&&ask==false){
 Serial.println(crTH.regs[adrReg]);
 // Envoyer le contenu du registre au coordonnateur
 Wire.write(crTH.regs[adrReg]);
 }
 if (ask){
  Wire.write(true);
  ask=false;
 }
}
