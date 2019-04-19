/*
 * noeud_Leq.ino développé par M.Bapst à partir
 * 
 * de la calsse calcule_Leq.ino du cours GPA788.
 * Ce programme est celui téléversé sur le noeud Leq pour le projet.
 * 
 * Voir notes de cours : Conception des objets (IIB)
 * pour les explications techniques de ce programme
 * 
 * GPA788 - ETS
 * T. Wong
 * 09-2018
 */

/* ------------------------------------------------------------- */
/* Constantes et variables globales                              */
/* ------------------------------------------------------------- */
#include <Wire.h>
#include <util/atomic.h>

bool coordonateur=true;  //indiquer si un coordonnateur est présent ou si l'on souhaite directement lire les valeurs du noeud.

const unsigned long SERIAL_BAUD_RATE{115200}; // Terminal serie
   
const byte G_PIN{A0};                    // Broche du Capteur sonore
const unsigned long G_TS{62.5};               // Fs = 16 Hz  
const unsigned int G_NB_SAMPLES{16};     // Li à toutes les 1 secondes
const unsigned int G_NB_LI{10};         // Calculer Leq chaque 10 secondes            
unsigned long G_countMillis;             // Compter les minutes

//valeurs pour i2c
const byte ADR_NOEUD=0x45; //adresse du noeud

enum commande {  //pour le démarrage ou l'arrêt de l'échantillonnage
  Go,Stop
};

const unsigned int NB_REGISTRES=6;
volatile commande cmd=Stop;
volatile double Leq;
volatile byte adrReg;
volatile bool ask=false;

union CarteRegistres {
  struct {
    volatile unsigned int NbLi;   //(2 octets)
    volatile double Leq;  //(4 octets)
  }champs;
  byte regs[NB_REGISTRES];
};

union CarteRegistres crLEQ;


/* ------------------------------------------------------------- */
/* Calculateur_VRMS                                              */
/* Une classe pour réaliser le calcul de la tension rms qui      */
/* représente le niveau sonore du capteur Electret MAX4466.      */
/* Note: La tension rms est disponible en volt rms et en dBV.    */
/* ------------------------------------------------------------- */
class Calculateur_VRMS {
public:
  // Constructeur
  Calculateur_VRMS(byte APin = A0, double v_max = 3.3, int max_adc = 1024) {
    m_APIN = APin; m_VMAX = v_max; m_ADCMAX = max_adc;
    m_Amplitude =  m_Vrms = m_dBV = m_TmpVrms = 0.0;
    m_nbTotalSamples = m_nbSamples = 0;
    m_VDC_OFFSET = m_VMAX / 2.0;
    m_C1 = m_VMAX / m_ADCMAX;
  analogReference(EXTERNAL); // utiliser VREF externe pour l'ADC = pin AREF sur l'Arduino
  pinMode(m_APIN, INPUT);       // capteur sonore à la broche PIN
  }
  ~Calculateur_VRMS() = default;                       // Destructeur

  /* Accès aux données membres de la classe                      */
  inline int getnbSamples() const { return m_nbSamples; }
  inline int getTotalSamples() const { return m_nbTotalSamples; }
  inline double getVrms() const { return m_Vrms; }
  inline int getAmplitude() const { return m_Amplitude; }
  inline double getdBV() const { return m_dBV; }
  inline byte getAPin() const { return m_APIN; }
  inline double getVMax() const { return m_VMAX; }
  inline int getADCMAX() const { return m_ADCMAX; }

  /* Services offerts                                           */
  void Accumulate() {   //calcule la valeur de v dans le cours : équation 9
    m_Amplitude = analogRead(m_APIN);
    // Convertir en volts
    double v = (m_Amplitude * m_C1) - m_VDC_OFFSET;
    // Accumuler v^2
    m_TmpVrms += (v * v);
    ++m_nbSamples;   
  }
  void Compute() {   //calcule la valeur de y' en dBV dans le cours : équations 10 et 11
    m_nbTotalSamples += m_nbSamples;    
    m_Vrms = sqrt(m_TmpVrms / m_nbSamples);
    m_dBV = 20.0 * log10(m_Vrms);
    // RAZ le décompte des échantillons
    m_nbSamples = 0;
    // RAZ le cumule des v^2
    m_TmpVrms = 0.0;
  }
  
private:
  byte m_APIN;                        // Broche du Capteur sonore
  double m_VMAX;                      // VREF est à 3.3 V par défaut
  int m_ADCMAX;                       // ADC à 10 bits
  double m_VDC_OFFSET;                // Tension décalage (niveau CC) 
  double m_C1;                        // Conversion bits -> volt
  int m_Amplitude;                    // Signal échantillonné en bits
  double m_Vrms;                      // Valeur Vrms
  double m_TmpVrms;                   // Pour acumuler v au carré
  double m_dBV;                       // Valeur dBV
  unsigned int m_nbTotalSamples;      // Nb. total des échantillons
  unsigned int m_nbSamples;           // Nb. d'échantillons

};

/* ------------------------------------------------------------- */
/* Calculateur_Li                                                */
/* Une classe pour réaliser le calcul de la valeur Li en         */
/* utilisant la sensibilité et le gain du capteur Electret       */
/* MAX4466.                                                      */
/* Note: Cette classe contient un objet de classe                */
/*       Calculateur_VRMS pour calculer la valeur dBV du signal  */
/*       échantillonné.                                          */
/* ------------------------------------------------------------- */
class Calculateur_Li {
public:
  // Pour le microphone Electret une application de 94 dB SPL
  // produit -44 dBV/Pa à sa sortie. Le gain du MAX4466 est par
  // défaut réglé à 125 ou 42 dBV.
  Calculateur_Li(double P = 94.0, double M = -44, double G = 42.0) {
    m_P = P; m_M = M; m_G = G;
  }
  ~Calculateur_Li() {}                       // Destructeur

  /* Accès aux données membres de la classe                      */
  inline double getLi() const { return m_Li; }
  inline double getP() const { return m_P; }
  inline double getM() const { return m_M; }
  inline double getG()const  { return m_G; }
  
  inline int getnbSamples() const { return c.getnbSamples(); }
  inline int getTotalSamples() const { return c.getTotalSamples(); }
  inline double getVrms() const { return c.getVrms(); }
  inline double getdBV() const { return c.getdBV(); }
  inline byte getAPin() const { return c.getAPin(); }
  inline double getVMax() const { return c.getVMax(); }
  inline int getADCMAX() const { return c.getADCMAX(); }

  /* Services offerts                                           */
  void Accumulate() { //lance le calcul de y' dBV de la l'objet calculateur_VRMS
    c.Accumulate();
  }
  double Compute() {   //calcule la valeur de Li dans le cours : équation 8
    c.Compute();
    m_Li = getdBV() + m_P - m_M - m_G;
    return m_Li;
  }
  
private:
  // Objet de classe Calculateur_VRMS pour réaliser les calculs
  // Vrms et dBV du signal échantillonné
  Calculateur_VRMS c;
  // Pour le calcul de Li
  double m_Li;                         // Niveau d'énergie sonore au temps ti
  double m_P;                          // Sensibilité Electret en dB SPL
  double m_M;                          // Sensibilité Electret en dBV/Pa
  double m_G;                          // Gain du MAX4466 en dBV
};

//objet calculateur_Leq

class calculateur_Leq {

public:
// constructeur
calculateur_Leq (int ts=62.5, unsigned int NB_Samples =16, unsigned int nbli=300 ){
  nb_li=0;
  sumLi=0.0;
  Leq=0.0;
  TS=ts;
  NB_SAMPLES=NB_Samples;
  NB_LI=nbli;
  startMillis=millis();
}
//destructeur
~calculateur_Leq ()=default;

//récupérer les attributs de li
  inline double getLi() const { return li.getLi(); }
  inline double getP() const { return li.getP(); }
  inline double getM() const { return li.getM(); }
  inline double getG()const  { return li.getG(); }
  inline int getnbSamples() const { return li.getnbSamples(); }
  inline int getTotalSamples() const { return li.getTotalSamples(); }
  inline double getVrms() const { return li.getVrms(); }
  inline double getdBV() const { return li.getdBV(); }
  inline byte getAPin() const { return li.getAPin(); }
  inline double getVMax() const { return li.getVMax(); }
  inline int getADCMAX() const { return li.getADCMAX(); }

//récupérer les attributs propres à Leq
inline double getLeq() const {return Leq;}
inline unsigned int getNbLi() const {return nb_li;}
inline double getSumLi() const {return sumLi;}
inline unsigned long getStartMillis() const {return startMillis;}
inline int getTS() const {return TS;}
inline unsigned int getNB_Samples() const {return NB_SAMPLES;}
inline unsigned int getNB_LI() const {return NB_LI;}
inline void setNB_LI(unsigned int nli) {NB_LI=nli;}



void Accumulate (){     //lance l'acquistion d'une valeur toutes les TS ms.
  if (millis()-startMillis>=TS){
    li.Accumulate();
    startMillis=millis();
  }
}



bool LeqComputer(){  //calcule une valeur de Leq dès que le nombre requis de Li est atteint
  if (li.getnbSamples()==NB_SAMPLES) {
    sumLi += pow(10.0, 0.1 * li.Compute());
    ++nb_li;
  }
  if (nb_li==NB_LI) {
  Leq = 10.0 * log10((1.0 / nb_li) * sumLi);
  nb_li = 0;
  sumLi = 0.0;
  return true;
  } else {
  return false;
  }
}




private:

Calculateur_Li li;
unsigned int nb_li;                 // Compter le nombre de Li
double sumLi;                     // Somme puissance 10 de Li
double Leq;                     // Valeur de Leq
unsigned long startMillis;
unsigned long TS;
unsigned int NB_SAMPLES;
unsigned int NB_LI;

};


// Créer un objet Calculateur_Leq en utilisant les paramètres
// par défaut.
calculateur_Leq leq(G_TS,G_NB_SAMPLES,G_NB_LI);
 
void setup() 
{
  // Initialiser le terminal série
  Serial.begin(SERIAL_BAUD_RATE);
 
  //initialiser le port i2c avec l'adresse adéquate.
  Wire.begin(ADR_NOEUD);
  Wire.onReceive(i2c_receiveEvent);   //initialisation des fonctions d'interruption générées par le coordonnateur.
  Wire.onRequest(i2c_requestEvent);
  // Démarrage la temporisation...
  //G_countMillis = millis();

  if (!coordonateur){    //condition qui permet de lancer le noeud sans présence de coordonnateur. La variable booléenne coordonnateur est à préciser en début de code.
    cmd=Go;
    Serial.println(F("Pas de coordonnateur"));
  }
}
 
 
void loop() 
{
  if (cmd==Go) {     //si le coordonnateur a démarré le noeud, on lance l'acquisition de données.
  leq.Accumulate();

if (leq.LeqComputer()) {               //lorsqu'une valeur de Leq est disponible, on la place dans le registre.
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){    //ATOMIC_BLOCK permet que cette opération soit interrupt_safe.
      crLEQ.champs.Leq=leq.getLeq();
      
    }
    /*Serial.print(leq.getLeq(), 3); Serial.print(F("\t\t"));
    Serial.println((1.0 * millis() - G_countMillis) / 60000); 
    G_countMillis = millis();*/
Serial.print(F("Leq en dB SPL : "));
Serial.println(leq.getLeq());
}
}
}

void i2c_receiveEvent(int combien) {    
 // Traiter les commandes ou les adresses de registre (1 octet)
 if (combien == 1) {
 // Un seul octet reçu. C'est probablement une commande.
 byte data = Wire.read();
 switch (data) {
 case 0xA1:
 cmd = Stop;
 Serial.println(F("commande 'Arrêter' reçue"));
 break;
 case 0xA2:
 cmd = Go;
 Serial.println(F("Commande 'Démarrer' reçue"));
 break;
 case 0xA3:
 Serial.println(F("Demande de disponibilité"));  //en début de programme, le coordonateur demande la disponibilité des noeuds.
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
 else if (combien == 2) {
 // Deux octets reçu. C'est probablement pour changer le
 // taux d'échantillonnage.
 byte data1 = Wire.read();
 byte data2 = Wire.read();
 //byte data3=Wire.read();
 Serial.println(F("Commande 'Changer Ts' reçue"));
 //Serial.println(data1); Serial.println(data2);
 if ((data1 == 0xA0) ) {
 crLEQ.champs.NbLi = data2; //A VERIFIER ordre des bits
 leq.setNB_LI(crLEQ.champs.NbLi);
 //Serial.print(F("La nouvelle valeur est: ")); 
 //Serial.print(crLEQ.champs.Ts); Serial.println(F(" secondes"));
 }
 }
}


void i2c_requestEvent(){
 // Le coordonnateur veut la valeur d'un registre. L'adresse du
 // registre a été reçue précédemment.
 if ((adrReg >= 0) && (adrReg < NB_REGISTRES)&&ask==false){
 Serial.print("");
 // Envoyer le contenu du registre au coordonnateur
 Wire.write(crLEQ.regs[adrReg]);
 }
 if (ask){
  Wire.write(true);
  ask=false;
 }
}

