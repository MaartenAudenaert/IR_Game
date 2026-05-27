**==> picture [550 x 372] intentionally omitted <==**

## **Game tech** 

**bachelor in de elektronica – ICT - Brugge docent: Van Gaever T. academiejaar 2026-2027** 

**Opdracht 3: Bluetooth integratie** 

**==> picture [534 x 98] intentionally omitted <==**

## Opdracht: Bluetooth Communicatie met LilyGO BLE Module via NUCLEO-L432KC 

**Hardware.** De Nucleo-L432KC wordt via UART verbonden met een **ESP32-C3 LilyGO module** die vooraf geflasht is met firmware die een transparante BLE-UART bridge implementeert. Op de ESP32 draait de **Nordic UART Service (NUS)** , een GATT-profiel dat een seriële verbinding over BLE emuleert. Elk byte dat de STM32 over UART naar de ESP32 stuurt, wordt automatisch als BLE-notificatie doorgestuurd naar de verbonden telefoon of PC — en omgekeerd. 

**Belangrijk verschil met de oude versie.** Je werkt **niet meer met AT-commando's** om de BLE-module te configureren. De ESP32 adverteert zichzelf onder de naam `blueart****` en een BLE-client (telefoon-app) kan rechtstreeks connecteren en data uitwisselen. 

**DMA.** De UART-communicatie tussen de Nucleo-L432KC en de ESP32-C3 moet nog steeds worden uitgevoerd met **DMA (Direct Memory Access)** . Zo blijft de CPU vrij voor het RC5protocol (strikte timing) terwijl er tegelijk data kan worden verzonden en ontvangen via BLE. 

BLE-communicatie is gestructureerd rond **GATT (Generic Attribute Profile)** . GATT definieert hoe data is georganiseerd en uitgewisseld tussen twee toestellen. 

- **GATT Server** — houdt de data (attributen) bij. In dit project: de ESP32-C3. 

- **GATT Client** — leest en schrijft data bij de server. In dit project: de BLE-app op je telefoon of PC. 

De hiërarchie op de ESP32 ziet er als volgt uit: 

```
GATT Server (ESP32-C3)
```

```
 └── Service: Nordic UART Service (NUS)
```

```
     ├── Characteristic RX  (telefoon → ESP32, WRITE)
```

```
     └── Characteristic TX  (ESP32 → telefoon, NOTIFY)
```

Elke entiteit heeft een **UUID** . De Nordic UART Service gebruikt 128-bit custom UUIDs (geen officiële Bluetooth SIG profielen): 

|||
|---|---|
|**Rol**|**UUID**|
|||
|**Service**|`6E400001-B5A3-F393-E0A9-E50E24DCCA9E`|
|**RX (telefoon → ESP32)**|`6E400002-B5A3-F393-E0A9-E50E24DCCA9E`|
|**TX (ESP32 → telefoon)**|`6E400003-B5A3-F393-E0A9-E50E24DCCA9E`|



_Let op:_ de namen RX en TX staan steeds **vanuit het perspectief van de ESP32** . De RXcharacteristic wordt dus _geschreven_ door de telefoon (data _in_ de ESP32), de TX-characteristic wordt _genotified_ door de ESP32 (data _uit_ de ESP32). 

## **1. Opdracht 1: UART Configuratie met DMA** 

Doel: Configureer de Nucleo-L432CK voor UART-communicatie met de LilyGO BLE module waarbij de data-overdracht via DMA verloopt. 

- Open STM32CubeMX en configureer de UART-interface voor communicatie met de LilyGO BLE module. 

- Configureer GPDMA voor zowel TX (transmit) als RX (receive) op de UART-peripheral. 

De instellingen voor GPDMA en UART worden niet gegeven, we hebben hier reeds voldoende ervaring mee opgebouwd om dit zelf in te stellen. 

## **2. Opdracht 2: BLE-verbinding opzetten met de telefoon/PC** 

## **Doel** 

Maak verbinding met de ESP32-C3 via een BLE-app op je telefoon of PC, en verifieer dat bytes die je typt in de app op de STM32 aankomen (en omgekeerd). 

## **Firmware van de ESP32-C3** 

De ESP32-C3 is vooraf geflasht met een BLE-UART-bridge sketch. Je hoeft hier dus **niets** aan te passen. De firmware doet drie dingen: 

1. Start een GATT-server met de Nordic UART Service en de bijhorende RX/TXcharacteristics. 

2. Adverteert zichzelf onder de naam `blueart` met het NUS-service UUID in het advertisement packet — hierdoor kunnen BLE-apps het toestel automatisch herkennen als een NUS-device. 

3. Brugt elke byte transparant tussen BLE en UART0 (naar de STM32). 

**Geen pairing, geen encryptie.** Elke BLE-client die het toestel ziet adverteren, mag verbinden en schrijven/lezen. Dat is voor een lokale, losstaande UART-bridge voldoende. 

## **App-installatie** 

Omdat BLE + NUS geen standaard SPP-profiel is, werkt verbinden via de gewone Bluetoothinstellingen van Windows, Android of iOS **niet** . Je hebt een BLE-app nodig die de NUS-UUIDs kent. Twee bruikbare keuzes: 

- **Serial Bluetooth Terminal** (Android) — automatische herkenning van NUS, eenvoudige terminal-interface. 

- **nRF Toolbox** (iOS, desktop PC) — toont alle services en characteristics, geschikt voor debugging. 

## **Verbindingsprocedure** 

1. Start de Nucleo-L432KC en de ESP32-C3 op. De ESP32 begint onmiddellijk te adverteren. 

2. Open de BLE-app en scan naar toestellen. 

3. Je zou het toestel moeten zien verschijnen met de naam `blueart` . Selecteer het. 

4. De app schrijft automatisch `0x0001` naar de CCCD (BLE2902-descriptor) van de TXcharacteristic om notificaties te activeren. 

5. Vanaf dit moment is de link transparant: typ je een bericht in de app, dan verschijnt het op UART1 RX van de STM32. Schrijft de STM32 iets op UART1 TX, dan komt dat binnen als notificatie in de app. 

## **Verificatie** 

Schrijf een eenvoudige loopback-test op de STM32: bij opstart zendt de STM32 één keer `Hello BLE` naar de ESP32 via DMA. Bevestig dat deze tekst in de BLE-app verschijnt. Typ vervolgens een byte in de app en controleer (met een debugger of LED-toggle) dat hij in de DMA-RX buffer van de STM32 aankomt. 

## **3. Opdracht 3: Commandosysteem en hit-tracking** 

## **Doel** 

Implementeer een commandosysteem bovenop de transparante BLE-UART link waarmee je de RC5-instellingen kan configureren en hit-data kan opvragen en resetten. Alle commando's worden door de STM32 ontvangen via de DMA-RX buffer en beantwoord via de DMA-TX buffer. 

## **Te implementeren commando's** 

|||
|---|---|
|**Commando**|**Beschrijving**|
|||
|**`current_settings`**|Retourneert het huidige adres en commando dat de<br>STM32 zelf zal uitzenden via RC5.|
|**`set_address:"<waarde>"`**|Stelt het adres in op de opgegeven waarde (0–31).|
|**`set_command:"<waarde>"`**|Stelt het commando in op de opgegeven waarde (0–63).|
|**`current_hits`**|Retourneert het aantal hits per speler (adres).|
|**`reset_hits`**|Wist het interne hit-geheugen.|



## **Vereisten** 

- Implementeer een commando-parser die de inhoud van de **DMA Circular RX buffer** periodiek (of op basis van een IDLE-line event) inspecteert en volledige commando's extraheert. 

- Houd per speler (per RC5-adres) het aantal ontvangen hits bij in een array of structuur in RAM. 

- Alle antwoorden gaan terug naar de BLE-app via **UART1 TX met DMA** . Stuur steeds een duidelijk afgelijnde tekstlijn terug (bv. afgesloten met `\r\n` ). 

- Zorg voor correcte foutafhandeling bij ongeldige commando's of ongeldige waarden (bv. adres > 31). 

Denk na over volgende vragen: 

- Waarom is DMA Circular mode geschikter dan Normal mode voor het ontvangen van data via UART? Wat zou er gebeuren als je Normal mode gebruikt voor RX? 

- Waarom is DMA hier extra belangrijk wanneer je RC5-timing en Bluetoothcommunicatie combineert op dezelfde microcontroller? 

- Hoe detecteer je het einde van een bericht in een DMA Circular buffer als de berichten variabele lengte hebben? 

- Waarom werkt een verbinding via de gewone Bluetooth-instellingen van Windows of Android **niet** met deze ESP32-firmware, en heb je een specifieke BLE-app nodig? 

