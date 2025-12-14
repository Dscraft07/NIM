# Dokumentace - Síťová hra Nim

**Předmět:** KIV/UPS - Úvod do počítačových sítí  
**Autor:** Student  
**Datum:** 2025

---

## Obsah

1. [Popis hry](#1-popis-hry)
2. [Specifikace protokolu](#2-specifikace-protokolu)
3. [Implementace serveru](#3-implementace-serveru)
4. [Implementace klienta](#4-implementace-klienta)
5. [Požadavky a překlad](#5-požadavky-a-překlad)
6. [Závěr](#6-závěr)

---

## 1. Popis hry

### 1.1 Pravidla hry Nim (misère varianta)

Nim je strategická hra pro dva hráče. V této implementaci se hraje zjednodušená varianta s jednou hromádkou:

- **Počáteční stav:** Na začátku hry je na hromádce **21 kamínků**
- **Průběh tahu:** Hráči se střídají a v každém tahu odeberou **1 až 3 kamínky**
- **Podmínka výhry:** Kdo odebere **poslední kamínek, prohrává** (misère pravidlo)
- **Speciální pravidlo:** Každý hráč má možnost **jednou za hru přeskočit svůj tah**

### 1.2 Architektura

- **Server-klient** architektura (1:N)
- Více souběžných her v oddělených místnostech
- Textový protokol nad **TCP**

---

## 2. Specifikace protokolu

### 2.1 Formát zpráv

Protokol používá textové zprávy ve formátu:
```
COMMAND;param1;param2;...\n
```

- **Oddělovač parametrů:** středník (`;`)
- **Ukončovač zprávy:** nový řádek (`\n`)
- **Kódování:** UTF-8

### 2.2 Stavy klienta

```
[CONNECTING] --LOGIN_OK--> [LOBBY] --CREATE/JOIN--> [IN_ROOM]
      ^                       ^                         |
      |                       |                    (2 hráči)
      |                       |                         v
      +----GAME_OVER----------+-----LEAVE_ROOM---- [IN_GAME]
```

### 2.3 Klientské zprávy

| Zpráva | Formát | Popis |
|--------|--------|-------|
| LOGIN | `LOGIN;nickname` | Přihlášení hráče |
| LIST_ROOMS | `LIST_ROOMS` | Žádost o seznam místností |
| CREATE_ROOM | `CREATE_ROOM;name` | Vytvoření nové místnosti |
| JOIN_ROOM | `JOIN_ROOM;room_id` | Připojení do místnosti |
| LEAVE_ROOM | `LEAVE_ROOM` | Opuštění místnosti |
| TAKE | `TAKE;count` | Odebrání kamínků (1-3) |
| SKIP | `SKIP` | Přeskočení tahu |
| PING | `PING` | Kontrola spojení |
| LOGOUT | `LOGOUT` | Odhlášení |

### 2.4 Serverové zprávy

| Zpráva | Formát | Popis |
|--------|--------|-------|
| LOGIN_OK | `LOGIN_OK` | Úspěšné přihlášení |
| LOGIN_ERR | `LOGIN_ERR;code;reason` | Chyba přihlášení |
| ROOMS | `ROOMS;count;id,name,players,max;...` | Seznam místností |
| ROOM_CREATED | `ROOM_CREATED;room_id` | Místnost vytvořena |
| ROOM_JOINED | `ROOM_JOINED;room_id;opponent` | Připojení úspěšné |
| ROOM_ERR | `ROOM_ERR;code;reason` | Chyba místnosti |
| LEAVE_OK | `LEAVE_OK` | Opuštění úspěšné |
| WAIT_OPPONENT | `WAIT_OPPONENT` | Čekání na protihráče |
| GAME_START | `GAME_START;stones;your_turn;opponent` | Začátek hry |
| TAKE_OK | `TAKE_OK;remaining;your_turn` | Tah úspěšný |
| TAKE_ERR | `TAKE_ERR;code;reason` | Chyba tahu |
| SKIP_OK | `SKIP_OK;your_turn` | Přeskočení úspěšné |
| SKIP_ERR | `SKIP_ERR;code;reason` | Chyba přeskočení |
| OPPONENT_ACTION | `OPPONENT_ACTION;action;param;remaining` | Akce protihráče |
| GAME_OVER | `GAME_OVER;winner;loser` | Konec hry |
| GAME_RESUMED | `GAME_RESUMED;stones;your_turn;your_skips;opp_skips` | Obnovení po reconnectu |
| PLAYER_STATUS | `PLAYER_STATUS;nickname;status` | Změna stavu hráče |
| PONG | `PONG` | Odpověď na PING |
| ERROR | `ERROR;code;message` | Obecná chyba |
| SERVER_SHUTDOWN | `SERVER_SHUTDOWN` | Server se vypíná |

### 2.5 Chybové kódy

| Kód | Konstanta | Popis |
|-----|-----------|-------|
| 0 | ERR_NONE | OK |
| 1 | ERR_INVALID_FORMAT | Neplatný formát zprávy |
| 2 | ERR_UNKNOWN_COMMAND | Neznámý příkaz |
| 3 | ERR_INVALID_PARAMS | Neplatné parametry |
| 4 | ERR_NOT_LOGGED_IN | Hráč není přihlášen |
| 5 | ERR_ALREADY_LOGGED_IN | Hráč je již přihlášen |
| 6 | ERR_NICKNAME_TAKEN | Přezdívka je obsazena |
| 7 | ERR_NICKNAME_INVALID | Neplatná přezdívka |
| 8 | ERR_ROOM_NOT_FOUND | Místnost neexistuje |
| 9 | ERR_ROOM_FULL | Místnost je plná |
| 10 | ERR_ROOM_NAME_TAKEN | Název místnosti je obsazen |
| 11 | ERR_NOT_IN_ROOM | Hráč není v místnosti |
| 12 | ERR_NOT_IN_GAME | Hráč není ve hře |
| 13 | ERR_NOT_YOUR_TURN | Není váš tah |
| 14 | ERR_INVALID_MOVE | Neplatný tah |
| 15 | ERR_NO_SKIPS_LEFT | Žádné přeskočení nezbývá |
| 16 | ERR_SERVER_FULL | Server je plný |
| 17 | ERR_MAX_ROOMS | Dosažen limit místností |
| 18 | ERR_GAME_IN_PROGRESS | Hra již probíhá |
| 99 | ERR_INTERNAL | Interní chyba serveru |

### 2.6 Validace vstupů

**Přezdívka:**
- Délka: 1-32 znaků
- Povolené znaky: písmena (a-z, A-Z), číslice (0-9), podtržítko (_)
- Musí začínat písmenem

**Název místnosti:**
- Délka: 1-64 znaků
- Povolené znaky: písmena, číslice, podtržítko, mezera

**Počet kamínků k odebrání:**
- Rozsah: 1-3
- Nesmí přesáhnout aktuální počet kamínků

### 2.7 Řešení výpadků

**Krátkodobý výpadek (do 30 sekund):**
- Klient se automaticky pokouší o reconnect (max 10 pokusů s exponenciálním backoff)
- Server pozastaví hru a informuje protihráče
- Po úspěšném reconnectu se hra obnoví

**Dlouhodobý výpadek (nad 30 sekund):**
- Odpojený hráč prohrává
- Protihráč je vrácen do lobby
- Klient musí zahájit nové spojení manuálně

### 2.8 Ochrana proti útokům

**Binární data (/dev/urandom):**
- Server kontroluje, zda příchozí data obsahují pouze tisknutelné ASCII znaky
- Nevalidní data jsou zahozena a počítána jako chybná zpráva
- Po 3 nevalidních zprávách je klient odpojen

**Flood protection:**
- Maximálně 20 zpráv za sekundu od jednoho klienta
- Zpráva bez ukončovacího znaku může mít max 256 bajtů
- Rate limiting s počítadlem zpráv

**Login timeout:**
- Klient musí poslat LOGIN do 30 sekund po připojení
- Jinak je automaticky odpojen

**TCP Keepalive:**
- Server i klient používají TCP keepalive pro detekci mrtvých spojení
- Keepalive probe každých 10 sekund
- Odpojení po 3 neúspěšných probe

**PING/PONG:**
- Obě strany posílají PING zprávy každých 8-10 sekund
- Timeout pro PONG je 15 sekund

---

## 3. Implementace serveru

### 3.1 Struktura projektu

```
server_src/
├── Makefile
├── include/
│   └── config.h          # Konfigurační konstanty
└── src/
    ├── main.c            # Vstupní bod
    ├── server.c/h        # Hlavní serverová logika
    ├── protocol.c/h      # Zpracování protokolu
    ├── player.c/h        # Správa hráčů
    ├── room.c/h          # Správa místností
    ├── game.c/h          # Herní logika
    └── logger.c/h        # Logování
```

### 3.2 Dekompozice modulů

**main.c**
- Parsování argumentů příkazové řádky
- Inicializace loggeru
- Spuštění serveru

**server.c**
- Socket setup a bind
- Hlavní smyčka s `select()` pro multiplexing
- Přijímání nových klientů
- Čtení a odesílání zpráv
- Zpracování timeoutů

**protocol.c**
- Parsování příchozích zpráv
- Vytváření odchozích zpráv
- Validace dat

**player.c**
- Správa stavu hráče (CONNECTING, LOBBY, IN_ROOM, IN_GAME, DISCONNECTED)
- Tracking aktivity pro ping/pong
- Počítadlo nevalidních zpráv

**room.c**
- Vytváření a rušení místností
- Přidávání/odebírání hráčů
- Spuštění hry

**game.c**
- Herní logika Nim
- Validace tahů
- Detekce konce hry

**logger.c**
- Thread-safe logování
- Úrovně: DEBUG, INFO, WARNING, ERROR

### 3.3 Metoda paralelizace

Server používá **single-threaded event-driven** architekturu s `select()`:

```c
while (running) {
    FD_ZERO(&read_fds);
    FD_SET(listen_fd, &read_fds);
    // Přidej všechny klientské sockety
    
    select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
    
    // Zpracuj nová spojení
    // Zpracuj data od klientů
    // Kontroluj timeouty
}
```

### 3.4 Konfigurace

Server lze spustit s následujícími parametry:

```bash
./nim_server [-a ADDRESS] [-p PORT] [-c MAX_CLIENTS] [-r MAX_ROOMS] [-v]
```

| Parametr | Výchozí | Popis |
|----------|---------|-------|
| -a | 0.0.0.0 | IP adresa pro bind |
| -p | 10000 | Port |
| -c | 50 | Maximální počet klientů |
| -r | 10 | Maximální počet místností |
| -v | false | Verbose režim (logy na stdout místo do souboru) |

**Logování:**
- Bez `-v`: logy se zapisují do `nim_server.log`
- S `-v`: logy se zobrazují přímo v terminálu

---

## 4. Implementace klienta

### 4.1 Struktura projektu

```
client_src/
├── build.xml             # Ant build script
├── lib/                  # JavaFX knihovny (staženy automaticky)
└── src/main/java/nim/
    ├── Main.java         # Vstupní bod JavaFX
    ├── network/
    │   ├── Client.java   # Síťová vrstva
    │   └── Protocol.java # Zpracování protokolu
    ├── game/
    │   └── GameState.java # Model stavu hry
    ├── ui/
    │   ├── LoginView.java   # Přihlašovací obrazovka
    │   ├── LobbyView.java   # Lobby s místnostmi
    │   ├── GameView.java    # Herní obrazovka
    │   └── Components.java  # Sdílené UI komponenty
    └── util/
        └── Logger.java   # Logování
```

### 4.2 Dekompozice tříd

**Main.java**
- Entry point JavaFX aplikace
- Nastavení hlavního okna

**Client.java**
- TCP socket komunikace
- Background thread pro příjem zpráv
- Automatický reconnect
- Ping/pong pro kontrolu spojení

**Protocol.java**
- Parsování a vytváření zpráv
- Enum pro typy zpráv a chybové kódy

**GameState.java**
- Model stavu aplikace (Observer pattern)
- Fáze: DISCONNECTED, CONNECTING, LOGIN, LOBBY, IN_ROOM_WAITING, IN_GAME, GAME_OVER

**LoginView.java**
- Formulář pro připojení (IP, port, přezdívka)
- Validace vstupů

**LobbyView.java**
- Tabulka místností
- Vytváření a připojování do místností

**GameView.java**
- Vizualizace kamínků (animované kruhy)
- Ovládací prvky (spinner, tlačítka)
- Indikace stavu protihráče

**Components.java**
- Sdílené styly a komponenty
- Jemná, příjemná barevná paleta

### 4.3 Použité knihovny

- **JavaFX 21.0.1** - GUI framework (stažen automaticky build scriptem)
- Standardní knihovna Java (java.net.Socket, java.io.*)
- Žádné externí knihovny pro síťovou komunikaci (pouze BSD sockety)

### 4.4 Non-blocking UI

Síťová komunikace probíhá v odděleném vlákně:

```java
// Příjem zpráv
receiverThread.submit(() -> {
    while (connected) {
        String line = reader.readLine();
        Platform.runLater(() -> processMessage(line));
    }
});

// Ping scheduler
pingScheduler.scheduleAtFixedRate(() -> {
    send(Protocol.createPing());
}, PING_INTERVAL, PING_INTERVAL, TimeUnit.MILLISECONDS);
```

---

## 5. Požadavky a překlad

### 5.1 Požadavky na server

- **OS:** Linux (GNU/Linux)
- **Kompilátor:** GCC s podporou C11
- **Knihovny:** POSIX threads (pthread)

### 5.2 Požadavky na klienta

- **OS:** Linux, Windows nebo macOS
- **Java:** JDK 17+ (doporučeno 21)
- **Apache Ant:** 1.10+
- **Síťové připojení:** Pro stažení JavaFX knihoven při prvním buildu

### 5.3 Překlad serveru

```bash
cd server_src

# Release build
make

# Debug build (s debugovacími symboly)
make debug

# Vyčištění
make clean
```

### 5.4 Překlad klienta

```bash
cd client_src

# Kompilace (automaticky stáhne JavaFX)
ant compile

# Spuštění
ant run

# Vytvoření JAR
ant jar

# Vyčištění
ant clean
```

### 5.5 Spuštění

**Server:**
```bash
cd server_src
./nim_server -a 0.0.0.0 -p 10000 -c 50 -r 10

# Nebo s verbose logováním
./nim_server -p 10000 -v
```

**Klient:**
```bash
cd client_src
ant run

# Nebo spuštění JAR
java --module-path lib --add-modules javafx.controls,javafx.fxml -jar build/nim-client.jar
```

### 5.6 Instalace na školních PC (UC-326)

Pro spuštění na čistém Linux PC je potřeba:

```bash
# Server (C)
sudo apt install build-essential gcc make

# Klient (Java)
sudo apt install openjdk-17-jdk ant

# Ověření
gcc --version
java -version
ant -version
```

---

## 6. Závěr

### 6.1 Dosažené výsledky

- Plně funkční síťová hra Nim pro dva hráče
- Stabilní server schopný obsluhovat více her současně
- Moderní GUI klient s příjemným designem
- Robustní protokol s validací a ošetřením chyb
- Podpora reconnectu při výpadku spojení

### 6.2 Testování

Aplikace byla testována:
- Standardní průběh hry bez výpadků
- Simulace výpadku klienta (přerušení spojení)
- Simulace výpadku serveru
- Zasílání nevalidních dat (`/dev/urandom`)
- Zprávy ve špatném stavu
- Neplatné tahy

### 6.3 Možná rozšíření

- Registrace hráčů s heslem
- Historie her a statistiky
- Turnajový režim
- Konfigurovatelný počet kamínků při vytváření místnosti
- Chatovací funkce

