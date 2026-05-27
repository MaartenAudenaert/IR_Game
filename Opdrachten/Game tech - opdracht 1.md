## Game tech

##### bachelor in de elektronica – ICT - Brugge

##### docent: Van Gaever T.

##### academiejaar 2026 - 2027

#### Opdracht 1:


# LaserTag Game

###### Infrarood Communicatie met STM32L432KC

```
Gebaseerd op ST Application Note AN
```
### 1. Inleiding

In dit vak ga je een volledig functioneel LaserTag-systeem bouwen met behulp van infrarood
communicatie. Je maakt gebruik van de STM32L432KC microcontroller en past het RC5-
protocol toe, zoals beschreven in de ST Application Note AN4834.

Het RC5-protocol is oorspronkelijk ontwikkeld door Philips voor afstandsbedieningen en
gebruikt Manchester-codering met een 36kHz draaggolf. In dit project wijken we af naar 38kHz
om optimaal gebruik te maken van de beschikbare TSOP4838 IR-ontvangers.

### 2. Fase 1: IR Transmitter

In deze fase bouw je de IR-zender die het RC5-signaal genereert.

#### 2.1. Project Setup

1. Download X-CUBE-IRREMOTE van de ST website
2. Maak een nieuw STM32CubeMX project voor de NUCLEO-L432KC
3. Kopieer de IR middleware bestanden (rc5_encode.c/h, ir_common.h)
4. Pas ir_common.h aan voor de L432KC timer configuratie (38kHz)

#### 2.2. Timer Configuratie voor 38kHz

Configureer de timer om een 38kHz draaggolf te genereren


#### 2.3. IRTIM Output Activeren

1. Bouw de h
2. ardware op breadboard volgens het schema in AN4834 Figure 8
3. Implementeer RC5_Encode_Init() en RC5_Encode_SendFrame()

### 3. Rapporteren

Meet met de oscilloscoop de output pin die de IR transmitter aanstuurt:

1. Geef een beeld weer van de 38kHz draaggolf op de oscilloscoop.
2. Meet de 25% duty cycle en geef deze weer.
3. Duid de gestuurde bits aan op het oscilloscoop beeld.
4. Geef het schema en de berekening van de aansturing van de IR LED a.d.h.v. de
    datasheet.
5. IR LED knippert bij button press.


