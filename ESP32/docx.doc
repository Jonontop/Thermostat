1. Pregled projekta in nastavitev strojne opreme
Ta projekt uporablja mikrokontroler ESP32 za izdelavo pametnega termostatskega sistema, ki lahko krmili rele za gretje ali hlajenje na podlagi temperaturnih odčitkov senzorja DHT11. ESP32 je povezan z WiFi, kar omogoča integracijo z Google Home prek Sinric Pro, kar omogoča daljinsko upravljanje termostata s pametnega telefona ali glasovnega asistenta. Dodatni strojni deli, uporabljeni v projektu, vključujejo:
•	DHT11 temperaturni senzor: zagotavlja odčitke temperature okolja.
•	Rele modul: vklaplja/izklaplja priključene naprave za gretje ali hlajenje glede na želene nastavitve temperature.
•	OLED zaslon: prikazuje trenutno temperaturo, ciljno temperaturo, način termostata in porabo energije.
1.1 Nastavitev strojne opreme
•	DHT11 senzor: povezan na GPIO 15 na ESP32.
•	Rele pin: povezan na GPIO 5 za krmiljenje grelne ali hladilne naprave.
•	OLED zaslon: uporablja I2C protokol, povezan na privzete I2C pine na ESP32.
1.2 Ključne knjižnice
•	WiFi: upravlja WiFi povezljivost.
•	SinricPro: omogoča integracijo z Google Home in daljinski dostop prek Sinric Pro.
•	DHT: upravlja temperaturne odčitke iz senzorja DHT11.
•	Adafruit_SSD1306: nadzoruje OLED zaslon za prikaz ustreznih podatkov v realnem času.
________________________________________
2. Integracija Sinric Pro za pametni dom
Koda uporablja Sinric Pro za povezavo termostata z Google Home ali Alexo, kar omogoča glasovno in aplikacijsko upravljanje. Sinric Pro je oblačna platforma, ki ESP32 omogoča interakcijo s pametnimi napravami v gospodinjstvu. Koraki integracije:
1.	Nastavitev računa Sinric Pro: Ustvari se račun na Sinric Pro, in doda se virtualna termostatska naprava, da se pridobijo appKey, appSecret in deviceID.
2.	Nastavitev povratnih klicev: Trije glavni povratni klici omogočajo interakcijo s termostatom iz Google Home:
o	onPowerState: krmili stanje napajanja releja (vklop/izklop) na podlagi uporabniških ukazov.
o	onTargetTemperature: nastavi ciljno temperaturo, ko je prejet ukaz.
o	onThermostatMode: nastavi način termostata (cool, heat, off) in ustrezno posodobi stanje releja.
Ti povratni klici omogočajo, da termostat upravljamo prek Google Home z glasovnimi ukazi in aplikacijo Sinric Pro.
2.1 Nadzor načina termostata
•	Način ogrevanja: aktivira rele, ko je trenutna temperatura pod ciljno.
•	Način hlajenja: aktivira rele, če je trenutna temperatura nad ciljno.
•	Izklopljen način: onemogoči rele ne glede na temperaturo.
________________________________________
3. Spremljanje temperature in logika krmiljenja releja
Senzor DHT11 se uporablja za neprekinjeno spremljanje temperature okolice. Funkcija updateTemperature() vsakih 5 sekund prebere trenutno temperaturo s senzorja DHT11 in izvede naslednja dejanja:
1.	Krmiljenje releja: posodobi stanje releja na podlagi trenutne temperature, ciljne temperature in načina.
2.	Posodobitev prikaza: na OLED zaslonu prikaže trenutno in ciljno temperaturo, način ter porabo energije.
3.	Posodobitev Sinric Pro: pošlje trenutni temperaturni odčitek Sinric Pro, kar omogoča, da termostat prikaže posodobljene informacije v aplikaciji Google Home.
3.1 Logika krmiljenja releja
Logika krmiljenja releja je kapsulirana v funkciji updateRelayState(), ki določa, ali naj bo rele vklopljen ali izklopljen glede na način termostata:
•	Način ogrevanja: vklopi rele, če je trenutna temperatura pod ciljno.
•	Način hlajenja: vklopi rele, če je trenutna temperatura nad ciljno.
•	Izklopljen način: izklopi rele.
Ta logika zagotavlja, da termostat greje ali hladi le, kadar je to potrebno, kar prispeva k varčevanju z energijo.
________________________________________
4. Spremljanje porabe energije
Implementiran je preprost sistem za spremljanje porabe energije, ki izračuna kumulativno porabo energije, kadar je rele aktiven. To dosežemo s formulo:
powerConsumption+=powerRate×(duration3600)\text{powerConsumption} += \text{powerRate} \times \left(\frac{\text{duration}}{3600}\right)powerConsumption+=powerRate×(3600duration)
kjer:
•	powerRate: stopnja porabe energije priključene naprave, privzeto nastavljena na 0,1 kWh.
•	duration: čas (v milisekundah), ko je bil rele aktiven od zadnjega izračuna.
Izračunana poraba energije je prikazana na OLED zaslonu, kar zagotavlja sprotne informacije o porabi energije priključene naprave. S prilagoditvijo powerRate lahko ta izračun prilagodimo specifičnim stopenjam porabe energije.
4.1 Prikaz porabe energije
Vrednost porabe energije se posodablja v realnem času in prikazuje na OLED zaslonu. To uporabnikom omogoča vpogled v energijo, ki jo uporablja priključena grelna ali hladilna naprava, kar spodbuja energetsko varčne prakse.
________________________________________
5. Tok kode in izvajanje
Koda je strukturirana v funkcijah setup() in loop(), ki inicializirata in upravljata glavne funkcije sistema.
5.1 Funkcija setup
Funkcija setup() inicializira komponente:
1.	WiFi: poveže ESP32 z omrežjem WiFi z vnaprej določenimi poverilnicami.
2.	DHT11 senzor: inicializira DHT senzor za začetek temperaturnih odčitkov.
3.	Rele: konfigurira pin releja kot izhod.
4.	OLED zaslon: nastavi zaslon in prikaže začetne informacije.
5.	Sinric Pro povratni klici: registrira povratne klice za stanje napajanja, ciljno temperaturo in način termostata pri Sinric Pro.
6.	Povezava Sinric Pro: zažene storitev Sinric Pro za omogočanje daljinskega dostopa prek Google Home.
5.2 Funkcija loop
Funkcija loop() neprekinjeno izvaja naslednja opravila:
1.	SinricPro Handle: posluša za dohodne ukaze s strežnika Sinric Pro, kar omogoča daljinsko upravljanje termostata.
2.	Posodobitev temperature: vsakih 5 sekund pokliče updateTemperature(), da:
o	Prebere temperaturo.
o	Krmili rele glede na način in temperaturo.
o	Posodobi OLED zaslon s trenutnimi podatki.
3.	Posodobitev porabe energije: posodobi porabo energije, če se je stanje releja spremenilo od zadnjega izračuna.
S takšno organizacijo funkcij sistem ostane odziven na ukaze in posodobitve, kar omogoča brezhibno integracijo v pametni dom in energetsko učinkovito upravljanje temperature.
5.3 Nasveti za odpravljanje težav
•	Težave s povezavo WiFi: Preverite, ali sta uporabljena pravilna SSID in geslo.
•	Povezava s Sinric Pro: Poskrbite, da so appKey, appSecret in deviceID pravilno vnešeni.
•	Odčitki temperature: Če so odčitki netočni, preverite povezave DHT senzorja.
________________________________________
Zaključek: Ta pametni termostatski sistem z uporabo ESP32, DHT11, releja in integracije Sinric Pro z Google Home omogoča učinkovito in energetsko varčno rešitev za nadzor temperature. Arhitektura kode podpira enostavno prilagoditev različnih pragov, naprav in prikaznih formatov,

