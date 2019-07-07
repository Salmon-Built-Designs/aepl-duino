//**************************************************************
//Aepl-Duino  Allumage electronique programmable - Arduino Nano et compatibles
//**************************************************************
#ifndef TINKERCAD
#include "TimerOne.h"
#else
static class
{
public:
  void initialize(unsigned long) {};
  void attachInterrupt(void (*)()) {};
  void stop() {};
} Timer1;
#endif

#include <SoftwareSerial.h>

char ver[] = "v0.1.2";

//******************************************************************************
//**************  Seulement  6 lignes à renseigner obligatoirement.****************
//**********Ce sont:  Na  Anga  Ncyl  AngleCapteur  CaptOn  Dwell******************

int Na[] = {0, 500, 800, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4200, 4600, 5100, 7000, 0};//t/*mn vilo
//Na[] et Anga[] doivent obligatoirement débuter puis se terminer par  0, et  contenir des valeurs  entières >=1
//Le dernier Na fixe la ligne rouge, c'est à dire la coupure de l'allumage.
//Le nombre de points est libre.L'avance est imposée à 0° entre 0 et Nplancher t/mn
//Anga [] degrés d'avance vilebrequin correspondant ( peut croitre ou decroitre)
int Anga[] = {0, 1 , 8 , 10  , 12,    14,  16,    22,  24,   24,   25,    26,   28,   28, 0};
int Ncyl = 4;           //Nombre de cylindres, moteur 4 temps.Multiplier par 2 pour moteur 2 temps
const int AngleCapteur = 45; //Position en degrès avant le PMH du capteur(Hall ou autre ).
const int CaptOn = 0;  //CapteurOn = 1 déclenchement sur front montant (par ex. capteur Hall "saturé")
//CapteurOn = 0 déclenchement sur front descendant (par ex. capteur Hall "non saturé").Voir fin du listing
const int Dwell = 3;
//Dwell = 1 pour alimenter la bobine en permanence sauf 1ms/cycle.Elle doit pouvoir chauffer sans crainte
//Dwell = 2 pour alimentation de la bobine seulement trech ms par cycle, 3ms par exemple
//Obligatoire pour bobine 'electronique'   de faible resistance: entre 2 et 0.5ohm.Ajuster  trech
//Dwell = 3 pour simuler un allumage à vis platinées: bobine alimentée 2/3 (66%) du cycle
//Dwell = 4 pour optimiser l'étincelle à haut régime.La bobine chauffe un peu plus.
//************************************************************************************
//******************************************************************************

//Valable pour tout type de capteur soit sur vilo soit dans l'allumeur (ou sur l'arbre à came)
//La Led(D13) existant sur tout Arduino suit le courant dans la bobine
//En option, multi-étincelles à bas régime pour denoyer les bougies
//En option, connexion d'un potard de 100kohm enter la patte A0 et la masse
//pour decaler "au vol" la courbe de quelques degrés, voir delAv plus bas
//En option,multi courbes , 2 courbes supplementaires, b et c, selectionables par D8 ou D
//Pour N cylindres,2,4,6,8,12,16, 4 temps, on a N cames d'allumeur ou  N/2 cibles sur le vilo
//Pour les moteurs à 1, 3 ou 5 cylindres, 4 temps, il FAUT un capteur dans l'allumeur (ou sur
//l'arbre à cames, c'est la même chose)
//Exception possible pour un monocylindre 4 temps, avec  capteur sur vilo et une cible:on peut génèrer
//une étincelle perdue au Point Mort Bas en utilisant la valeur Ncyl =2.
//Avance 0°jusqu'a Nplancher t/mn, anti retour de kick.
//**********************LES OPTIONS**********************
//Si Dwell=2, temps de recharge bobine, 3ms= 3000µs typique, 7ms certaines motos
const int trech  = 3000;
//Si Dwell=4, durée de l'étincelle tetin au delà de Ntrans
const int Ntrans = 3000; //Regime de transition
const int tetin = 500; //Typique 500 à 1000µs, durée etincelle regimes >Ntrans
//Si multi-étincelle désiré jusqu'à N_multi, modifier ces deux lignes
const int Multi = 0;//1 pour multi-étincelles
const int N_multi = 2000; //t/mn pour 4 cylindres par exemple
//*******************MULTICOURBES****IF FAUT METTRE D8 ou D9 A LA MASSE!!!!!!!*******
//A la place de la courbe a, on peut selectionner la courbe b (D8 à la masse)ou la c (D9 à la masse)
//*******//*********Courbe   b
int Nb[] = {0,  3100, 0};   //Connecter D8 à la masse
int Angb[] = {0,   12,   0};
//*******//*********Courbe   c
int Nc[] = {0,  6100,  0};    //Connecter D9 à la masse
int Angc[] = {0,  16,   0};
//**********************************************************************************
//************Ces 3 valeurs sont eventuellement modifiables*****************
//Ce sont: Nplancher, trech , Dsecu et delAv
const int Nplancher = 500; // vitesse en t/mn jusqu'a laquelle l'avance  = 0°
const int unsigned long Dsecu  = 1000000;//Securite: bobine coupee à l'arret apres Dsecu µs
int delAv = 2;//delta avance,par ex 2°. Quand Pot avance d'une position, l'avance croit de delAv
//Uniquement si l'on veut jouer à translater les courbes d'avance en connectant un potard
//Ceci est une option. Avec un potard de 100kohms entre A0 et la masse, de 0-1V environ, courbe originale
//de 1 a 2V environ avance augmentée de delAv, au dela de 2V, augmentée de 2*delAv
//Attention.....Pas de pleine charge avec trop d'avance, danger pour les pistons..

//***********************Variables du sketch************************************
const int Bob = 4;    //Sortie D4 vers bobine.En option, on peut connecter une Led avec R=330ohms vers la masse
const int Cible = 2;  //Entrée sur D2 du capteur, R PullUp
const int Pot = A0;   //Entrée analogique sur A0 pour potard de changement de courbes. R PullUp
const int Led13 = 13; //Temoin sur tout Arduino, suit le courant de bobine
const int Courbe_b = 8;  //Entré D8  R PullUp.Connecter à la masse pour courbe b
const int Courbe_c = 9;  //Entré D9  R PullUp. Connecter à la masse pour courbe c
//Ici 3 positions:Decalage 0° si <1V, delAV° si 1 à 2 V, 2*delAv° pour > 2V
int valPot = 0;       //0 à 1023 selon la position du potentiomètre en entree
float modC1 = 0;      //Correctif pour C1[], deplace la courbe si potard connecté
int unsigned long D = 0;  //Delai en µs à attendre après la cible pour l'étincelle
int milli_delay = 0;
int micro_delay = 0;
float RDzero = 0; //pour calcul delai avance 0° < Nplancher t/mn
float  Tplancher = 0; //idem//idem
int tcor  = 140; //correction en µs  du temps de calcul pour D
int unsigned long Davant_rech = 0;  //Delai en µs avant la recharge de la  bobine.
int unsigned long prec_H  = 0;  //Heure du front precedent en µs
int unsigned long T  = 0;  //Periode en cours
int unsigned long Tprec  = 0;//Periode precedant la T en cours, pour calcul de Drech
int N1  = 0;  //Couple N,Ang de debut d'un segment
int Ang1  = 0; //Angle d'avance vilo en degrès
int N2  = 0; //Couple N,Ang de fin de segment
int Ang2  = 0;
int*  pN = &Na[0];//pointeur au tableau des régimes. Na sera la courbe par defaut
int*  pA = &Anga[0];//pointeur au tableau des avances. Anga sera la  courbe par defaut
float k = 0;//Constante pour calcul de C1 et C2
float C1[30]; //Tableaux des constantes de calcul de l'avance courante
float C2[30]; //Tableaux des constantes de calcul de l'avance courante
float Tc[30]; //Tableau des Ti correspondants au Ni
//Si necessaire, augmenter ces 3 valeurs:Ex C1[40],C2[40],Tc[40]
int Tlim  = 0;  //Période minimale, limite, pour la ligne rouge
int j_lim = 0;  //index maxi des N , donc aussi  Ang
int unsigned long NT  = 0;//Facteur de conversion entre N et T à Ncyl donné
int AngleCibles = 0;//Angle entre 2 cibles, 180° pour 4 cyl, 120° pour 6 cyl, par exemple
int UneEtin = 1; //=1 pour chaque étincelle, testé et remis à zero par isr_GestionIbob()
int Ndem = 60;//Vitesse estimée du vilo entrainé par le demarreur en t/mn
int unsigned long Tdem  = 0;  //Periode correspondante à Ndem,forcée pour le premier tour
int Mot_OFF = 0;//Sera 1 si moteur detecté arrété par l'isr_GestionIbob()
int unsigned long Ttrans; //T transition de Dwell 4
int unsigned long T_multi  = 0;  //Periode minimale pour multi-étincelle
//Permet d'identifier le premier front et forcer T=Tdem, ainsi que Ibob=1, pour demarrer au premier front


static const int PIN_INLET_MANIFOLD_PRESSURE_SENSOR = A6;
static int relPressureMbar = 0;


// Singleton for sending/receiving data efficiently to the HC05 Bluetooth module.
class BluetoothManager
{
public:
  static BluetoothManager& get();

  // If time has come, performs an action (i.e. reads or sends one data item),
  // else returns. Make sure to call this method often.
  // Returns true if it has just performed the last action of a full exchange.
  bool exchange();

private:
  enum Action
  {
    NO_ACTION_PERFORMED_YET = -1,

    ACTION_READ_ITEM_ADVANCE_CORRECTION = 0,
    ACTION_SEND_ITEM_HEADER,
    ACTION_SEND_ITEM_RPM,
    ACTION_SEND_ITEM_TIMING_ADVANCE,
    ACTION_SEND_ITEM_INTAKE_PRESSURE,
    ACTION_SEND_ITEM_RECEIVED,

    AMOUNT_ACTIONS_PER_EXCHANGE
  };

  static const int PIN_PLUGGED_TO_HC05_TX = 10;
  static const int PIN_PLUGGED_TO_HC05_RX = 11;
  static const int BAUD_RATE = 9600;
  static const unsigned long MS_BETWEEN_ACTIONS = 50;
  static const unsigned long MS_BETWEEN_EXCHANGES = 50;
  static const unsigned char HEADER = 0xff;

  BluetoothManager();

  SoftwareSerial serialDevice;

  Action lastActionPerformed;
  unsigned long msLastTimeActionPerformed;
  unsigned char lastReceived;
};


//********************LES FONCTIONS*************************

void  CalcD ()//////////////////
// Noter que T1>T2>T3...
{ for (int j = 1; j <= j_lim; j++)//On commence par T la plus longue et on remonte
  {
    if  (T >=  Tc[j]) {     //on a trouvé le bon segment de la courbe d'avance
      D =  float(T * ( C1[j] - modC1 )  + C2[j]) ;//D en µs, C2 incorpore le temps de calcul tcor
      if ( T > Tplancher)D = T * RDzero;//Imposer 0° d'avance de 0 à 500t/mn
      break;  //Sortir, on a D
    }
  }
}

void  Genere_multi()//////////
{ //L'etincelle principale a juste été générée
  delay(1); //Attendre fin d'etincelle 1ms
  digitalWrite(Bob, 1);//Retablir  le courant
  delay(3); //Recharger 3ms
  digitalWrite(Bob, 0);//Première etincelle secondaire
  delay(1); //Attendre fin d'etincelle 1ms
  digitalWrite(Bob, 1);//Retablir  le courant
  delay(2); //Recharger 2 ms
  digitalWrite(Bob, 0);//Deuxième etincelle secondaire
  delay(1); //Attendre fin d'etincelle 1ms
  digitalWrite(Bob, 1);//Retablir  le courant pour étincelle principale
}

void Tst_Pot()///////////
{ valPot = analogRead(Pot);
  if (valPot < 240 || valPot > 900)modC1 = 0;//0° ou pas de potard connecté (valpot =1023 en théorie)
  else {
    if (valPot < 500)modC1 = float (delAv) / float(AngleCibles);//Position 1
    else modC1 = 2 * float (delAv) / float(AngleCibles);//Position 2
  }
}

// Reads and returns the relative pressure in mbar. Make sure to do the first call early as it will
// consider the first read absolute pressure to be the atmospheric pressure and make use of it in
// the next calls.
int getRelPressureMbar()
{
  static int absAtmPressureMbar = 0;

  // 0 to 1023, see https://www.arduino.cc/reference/en/language/functions/analog-io/analogread/
  const int absPressure = analogRead(PIN_INLET_MANIFOLD_PRESSURE_SENSOR);

  // 0 to 5 V, but the sensor won't actually have an output below 0.2 V:
  const float absPressureVolts = absPressure * .0049;

  if (absPressureVolts < .2) // sensor is disconnected
  {
    return 0;
  }

  // 200 to 3100 mbar, but the sensor won't actually go beyond 2500 mbar:
  const int absPressureMbar = 500.0 * (absPressureVolts - .2) + 200;
  if (!absAtmPressureMbar) // first time, current pressure must be the atmospheric pressure
  {
    if (absPressureMbar < 800 || absPressureMbar > 1200) // something is wrong, are we on Earth?
    {
      return 0;
    }
    absAtmPressureMbar = absPressureMbar;
  }

  return absPressureMbar - absAtmPressureMbar;
}

void Wait_For_Target(int wantedState)
{
  while (digitalRead(Cible) != wantedState)
  {
    if (BluetoothManager::get().exchange())
    {
      // If we have just finished a full bluetooth exchange, update the known pressure for next time
      // as we might be stuck here waiting if the engine isn't running:
      relPressureMbar = getRelPressureMbar();
    }
  }
}

void  Etincelle ()//////////
{ if (D < 14000) {         // Currently, the largest value that will produce an accurate delay is 16383 µs
    delayMicroseconds(D); //Attendre D }
  }
  else {
    milli_delay = ((D / 1000) - 2);//Pour ces D longs, delayMicroseconds(D)ne va plus.
    micro_delay = (D - (milli_delay * 1000));
    delay(milli_delay); //
    delayMicroseconds(micro_delay);
  }
  digitalWrite(Bob, 0);//Couper le courant, donc étincelle
  digitalWrite(Led13, 0); //Temoin
  //Maintenant que l'étincelle est émise, il faut rétablir Ibob au bon moment

  if (Multi && (T >= T_multi))Genere_multi();
  else {
    switch (Dwell)  //Attente courant coupé selon le type de Dwell

    { case 1:       //Ibob coupe 1ms par cycle seulement, la bobine doit supporter de chauffer
        Davant_rech = 1000; //1ms off par cycle
        break;

      case  2:      //Type bobine faible resistance, dite "electronique"
        Davant_rech = 2 * T - Tprec - trech;//On doit tenir compte des variations de régime moteur
        Tprec = T;    //Maj de la future periode precedente
        break;

      case  3:      //Type "vis platinées", Off 1/3, On 2/3
        Davant_rech = T / 3;
        break;


      case  4:     //Type optimisé haut régime
        if ( T > Ttrans )Davant_rech = T / 3; // En dessous de N trans, typique 3000t/mn
        else Davant_rech = tetin; // Au delà de Ntrans, on limite la durée de l'étincelle, typique 0.5ms
        break;
    }
    Timer1.initialize(Davant_rech);//Attendre Drech µs avant de retablire le courant dans la bobine
  }

  if ((Dwell != 4) || (T > Ttrans)) {
    BluetoothManager::get().exchange();
  }

  Tst_Pot();//Voir si un potard connecté pour deplacer la courbe ou selectionner une autre courbe
  UneEtin = 1; //Pour signaler que le moteur tourne à l'isr_GestionIbob().
}

void  Select_Courbe()///////////
//Par défaut, la courbe a est déja selectionnée
{ if (digitalRead(Courbe_b) == 0) {   //D8 à la masse
    pN = &Nb[0];  // pointer à la courbe b
    pA = &Angb[0];
  }
  if (digitalRead(Courbe_c) == 0) {    //D9 à la masse
    pN = &Nc[0];  // pointer à la courbe c
    pA = &Angc[0];
  }
}

void  isr_GestionIbob()//////////
{ Timer1.stop();    //Arreter le decompte du timer
  if (UneEtin == 1) {
    digitalWrite(Bob, 1);    //Le moteur tourne,retablire le courant dans bobine
    digitalWrite(Led13, 1);//Temoin
  }
  else
  { digitalWrite(Bob, 0);  digitalWrite(Led13, 0); //Temoin//Moteur arrete, preserver la bobine, couper le courant
    Mot_OFF = 1;//Permettra à loop() de detecter le premier front de capteur
  }
  UneEtin = 0;  //Remet  le detecteur d'étincelle à 0
  Timer1.initialize(Dsecu);//Au cas où le moteur s'arrete, couper la bobine apres Dsecu µs
}

void  Init ()/////////////
//Calcul de 3 tableaux,C1,C2 et Tc qui serviront à calculer D, temps d'attente
//entre la detection d'une cible par le capteur  et la generation de l'etincelle.
//Le couple C1,C2 est determiné par la periode T entre deux cibles, correspondant au
//bon segment de la courbe d'avance entrée par l'utilisateur: T est comparée à Tc
{ AngleCibles = 720 / Ncyl; //Cibles sur vilo.Ex pour 4 cylindres 180°,  120° pour 6 cylindres
  NT  = 120000000 / Ncyl; //Facteur de conversion Nt/mn moteur, Tµs entre deux PMH étincelle
  //c'est à dire deux cibles sur vilo ou deux cames d'allumeur
  Ttrans = NT / Ntrans; //Calcul de la periode de transition pour Dwell 4
  T_multi = NT / N_multi; //Periode minimale pour generer un train d'étincelle
  //T temps entre 2 étincelle soit 720°  1°=1/6N
  Tdem  = NT / Ndem; //Periode imposée à  la première étincelle qui n'a pas de valeur prec_H
  Tplancher = 120000000 / Nplancher / Ncyl; //T à  vitesse plancher en t/mn: en dessous, avance centrifuge = 0
  RDzero = float(AngleCapteur) / float(AngleCibles);
  Select_Courbe();  //Ajuster éventuellement les pointeurs pN et pA pour la courbe b ou c
  N1  = 0; Ang1 = 0; //Toute courbe part de  0
  int i = 0;    //locale mais valable hors du FOR
  pN++; pA++; //sauter le premier element de tableau, toujours =0
  for (i  = 1; *pN != 0; i++)//i pour les C1,C2 et Tc.Arret quand regime=0.
    //pN est une adresse (pointeur) qui pointe au tableau N.Le contenu pointé est *pN
  { N2 = *pN; Ang2 = *pA;//recopier les valeurs pointées dans N2 et Ang2
    k = float(Ang2 - Ang1) / float(N2  - N1);//pente du segment (1,2)
    C1[i] = float(AngleCapteur - Ang1 + k * N1) / float(AngleCibles);
    C2[i] = -  float(NT * k) / float(AngleCibles) - tcor; //Compense la durée de calcul de D
    Tc[i] = float(NT / N2);  //
    N1 = N2; Ang1 = Ang2; //fin de ce segment, début du suivant
    pN++; pA++;   //Pointer à l'element suivant de chaque tableau
  }
  j_lim = i - 1; //Revenir au dernier couple entré
  Tlim  = Tc[j_lim]; //Ligne rouge
  Serial.print("Tc = "); for (i = 1 ; i < 15; i++)Serial.println(Tc[i]);
  Serial.print("Tlim = "); Serial.println(Tlim);
  Serial.print("C1 = "); for (i = 1 ; i < 15; i++)Serial.println(C1[i]);
  Serial.print("C2 = "); for (i = 1 ; i < 15; i++)Serial.println(C2[i]);
  //Timer1 a deux roles:
  //1)couper le courant dans la bobine en l'absence d'etincelle pendant plus de Dsecu µs
  //2)après une étincelle, attendre le delais Drech avant de retablir le courant dans la bobine
  //Ce courant n'est retabli que trech ms avant la prochaine étincelle, condition indispensable
  //pour une bobine à faible resistance, disons inférieure à 3 ohms.Typiquement trech = 3ms à 7ms
  Timer1.attachInterrupt(isr_GestionIbob);//IT d'overflow de Timer1 (16 bits)
  Timer1.initialize(Dsecu);//Le courant dans la bobine sera coupé si aucune etincelle durant Dsecu µs
  Mot_OFF = 1;// Signalera à loop() le premier front
  digitalWrite(Bob, 0); //par principe, couper la bobine
  digitalWrite(Led13, 0); //Temoin
}

////////////////////////////////////////////////////////////////////////
void setup()///////////////
/////////////////////////////////////////////////////////////////////////
{
  Serial.begin(9600);
  Serial.println(ver);
  BluetoothManager::get();
  pinMode(Cible, INPUT_PULLUP); //Entrée front du capteur sur D2
  pinMode(Bob, OUTPUT); //Sortie sur D4 controle du courant dans la bobine
  pinMode(Pot, INPUT_PULLUP); //Entrée pour potard 100kohms, optionnel
  pinMode(Courbe_b, INPUT_PULLUP); //Entrée à la masse pour selectionner la courbe b
  pinMode(Courbe_c, INPUT_PULLUP); //Entrée à la masse pour selectionner la courbe c
  pinMode(Led13, OUTPUT);//Led d'origine sur tout Arduino, temoin du courant dans la bobine
  pinMode(PIN_INLET_MANIFOLD_PRESSURE_SENSOR, INPUT);

  Init();// Executée une fois au demarrage et à chaque changement de courbe
}

///////////////////////////////////////////////////////////////////////////
void loop()   ////////////////
////////////////////////////////////////////////////////////////////////////
{
  static bool firstTime = true;
  if (firstTime)
  {
    relPressureMbar = getRelPressureMbar();
    firstTime = false;
  }

  Wait_For_Target(CaptOn);

  T = micros() - prec_H;    //front actif, arrivé calculer T
  prec_H = micros(); //heure du front actuel qui deviendra le front precedent
  if ( Mot_OFF == 1 ) { //Demarrage:premier front de capteur
    T = Tdem;//Fournir  T = Tdem car prec_H n'existe par pour la première étincelle
    digitalWrite(Bob, 1);//Alimenter la bobine
    digitalWrite(Led13, 1); //Temoin
    Mot_OFF = 0; //Le moteur tourne
  }

  relPressureMbar = getRelPressureMbar();

  if (T > Tlim)     //Sous la ligne rouge?
  { CalcD(); // Top();  //Oui, generer une etincelle
    Etincelle();
  }

  Wait_For_Target(!CaptOn);
}


BluetoothManager& BluetoothManager::get()
{
  static BluetoothManager instance;
  return instance;
}

bool BluetoothManager::exchange()
{
  unsigned long now = millis();
  if (msLastTimeActionPerformed + MS_BETWEEN_ACTIONS > now ||
      !lastActionPerformed && msLastTimeActionPerformed + MS_BETWEEN_EXCHANGES > now)
  {
    return false;
  }

  Action nextAction = static_cast<Action>((static_cast<int>(lastActionPerformed) + 1) %
                                          static_cast<int>(AMOUNT_ACTIONS_PER_EXCHANGE));

#ifdef TINKERCAD
#define BT_DEV Serial
  BT_DEV.print("[BT] ");
#else
#define BT_DEV serialDevice
#endif

  switch (nextAction)
  {
    case ACTION_READ_ITEM_ADVANCE_CORRECTION:
      while (BT_DEV.available())
      {
        lastReceived = BT_DEV.read();
      }
      break;

    case ACTION_SEND_ITEM_HEADER:
#ifndef TINKERCAD
      BT_DEV.write(HEADER);
#endif
      break;

    case ACTION_SEND_ITEM_RPM:
    {
      // T:  time in micro-seconds of an engine revolution
      // NT: constant for converting T into rotations per minute
      const unsigned long rpm = NT / T;

#ifndef TINKERCAD
      // Format it so that it can never contain HEADER:
      unsigned short rpmShort = rpm * 2;
      BT_DEV.write(reinterpret_cast<const char*>(&rpmShort), sizeof(rpmShort));
#else
      BT_DEV.print("RPM = ");
      BT_DEV.print(rpm);
#endif
      break;
    }

    case ACTION_SEND_ITEM_TIMING_ADVANCE:
    {
      // tcor:         constant correction of D due to the time it takes to calculate it
      // D:            corrected (tcor has already been subtracted) delay in micro-seconds to wait
      //               after the target
      // D + tcor:     uncorrected delay
      // AngleCibles:  angle in degrees between two targets, e.g. 180 degrees with 4 cylinders
      // AngleCapteur: position in degrees of the sensor before the top dead center
      const int timingAdvanceDeg = AngleCapteur - (D + tcor) * AngleCibles / T;

#ifndef TINKERCAD
      // It should be between -100 and +100, so we can have it fit in a byte:
      unsigned char timingAdvanceByte = 0;
      if (timingAdvanceDeg >= -100 && timingAdvanceDeg <= 100)
      {
        timingAdvanceByte = timingAdvanceDeg + 100;
      }
      BT_DEV.write(timingAdvanceByte);
#else
      BT_DEV.print("advance = ");
      BT_DEV.print(timingAdvanceDeg);
      BT_DEV.print(" deg");
#endif
      break;
    }

    case ACTION_SEND_ITEM_INTAKE_PRESSURE:
    {
#ifndef TINKERCAD
      // It should be between -1200 and +1200, so with a precision of 10 mbar we can fit it in a
      // byte:
      unsigned char relPressureByte = 0;
      if (relPressureMbar >= -1200 && relPressureMbar <= 1200)
      {
        relPressureByte = (relPressureMbar + 1200) / 10;
      }
      BT_DEV.write(relPressureByte);
#else
      BT_DEV.print("intake pres. = ");
      BT_DEV.print(relPressureMbar);
      BT_DEV.print(" mbar");
#endif
      break;
    }

    case ACTION_SEND_ITEM_RECEIVED:
#ifdef TINKERCAD
      BT_DEV.print("received = ");
#endif
      BT_DEV.write(lastReceived);
      break;
  }

#ifdef TINKERCAD
  BT_DEV.println("");
#endif
#undef BT_DEV

  lastActionPerformed = nextAction;
  msLastTimeActionPerformed = now;

  return lastActionPerformed == AMOUNT_ACTIONS_PER_EXCHANGE - 1;
}

BluetoothManager::BluetoothManager()
  : serialDevice(PIN_PLUGGED_TO_HC05_TX, PIN_PLUGGED_TO_HC05_RX),
    lastActionPerformed(NO_ACTION_PERFORMED_YET),
    msLastTimeActionPerformed(0)
{
  serialDevice.begin(BAUD_RATE);
  serialDevice.flush();
}


/////////////////Exemples de CAPTEURS/////////////////
//Capteur Honeywell cylindrique 1GT101DC,contient un aimant sur le coté,type non saturé, sortie haute à vide,
//et basse avec une cible en acier. Il faut  CapteurOn = 0, declenchement sur front descendant.
//Le capteur à fourche SR 17-J6 contient un aimant en face,type saturé, sortie basse à vide,
//et haute avec une cible en acier. Il faut  CapteurOn = 1, declenchement sur front montant.

//Pour les Ncyl pairs:2,4,6,8,10,12, le nombre de cibles réparties sur le vilo est Ncyl/2
//Dans les deux cas (capteur sur vilo ou dans l'alumeur) la periode entre deux cibles et la même car l'AàC tourne à Nvilo/2
//Pour les Ncyl impairs 1,3 5, 7?,9? il FAUT un capteur dans l'alumeur (ou AàC)

////////////////DEBUGGING////////////////////////
//Macro  ps(v) de debug pour imprimer le numero de ligne, le nom d'une variable, sa valeur
//puis s'arreter definitivement
//#define ps(v) Serial.print("Ligne_") ; Serial.print(__LINE__) ; Serial.print(#v) ; Serial.print(" = ") ;Serial.println((v)) ; Serial.println("  Sketch stop"); while (1);
//Exemple, à la ligne 140, l'instruction     ps(var1);
//inprimera  "Ligne_140var1 = 18  Sketch stop"
//Macro  pc(v)de debug pour imprimer le numero de ligne, le nom d'une variable, sa valeur,
//puis s'arreter et attendre un clic de souris  sur le bouton 'Envoyer'en haut de l'ecran seriel pour continuer.
//#define pc(v) Serial.print("Ligne_") ; Serial.print(__LINE__) ; Serial.print(#v) ;Serial.print(" = ") ; Serial.println((v)) ; Serial.println(" Clic bouton 'Envoyer' pour continuer") ;while (Serial.available()==0);{ int k_ = Serial.parseInt() ;}
//Exemple, à la ligne 145, l'instruction    pc(var2);
// inprimera   "Ligne_145var2 = 25.3   Clic bouton 'Envoyer' pour continuer"
//float gf = 0;//pour boucle d'attente,gf  GLOBALE et FLOAT indispensable
//  gf = 1; while (gf < 2000)gf++;//10= 100µs,100=1.1ms,2000=21.8ms
//void  Top()//////////
//{ digitalWrite(Bob, 1); //Crée un top sur l'oscillo
//  gf = 1; while (gf < 10)gf++;//gf DOIT être Globale et Float 10=100µs,2000=21.8ms, retard/Cible=50µs
//  digitalWrite(Bob, 0); //
//}
//void software_Reset()  //jamais testé
// Redémarre le programme depuis le début mais ne
// réinitialiser pas les périphériques et les registresivre...
//{
//  asm volatile ("  jmp 0");
//}
