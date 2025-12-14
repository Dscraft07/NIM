# Testování s InTCPtor

## Přehled

InTCPtor simuluje **extrémně špatné síťové podmínky**:
- Fragmentace zpráv (posílání po 1-2 bytech)
- Zpoždění (100ms ± 10ms)
- Náhodné uzavírání spojení

## 1. Kompilace InTCPtor (na Linux PC)

```bash
cd InTCPtor
mkdir build
cd build
cmake ..
cmake --build .
```

Nebo jednodušeji:
```bash
cd InTCPtor
./build_and_env.sh
```

Po buildu budou v `build/`:
- `intcptor-run` - spouštěč
- `libintcptor-overrides.so` - knihovna pro intercepting

## 2. Příprava serveru

```bash
cd server_src
make clean && make
```

## 3. Spuštění serveru s InTCPtor

```bash
# Z adresáře InTCPtor/build
./intcptor-run ../../server_src/nim_server -p 10000 -v
```

Nebo manuálně:
```bash
cd server_src
LD_PRELOAD=../InTCPtor/build/libintcptor-overrides.so ./nim_server -p 10000 -v
```

## 4. Spuštění klienta s InTCPtor

```bash
# Klient také přes InTCPtor (na Linux PC)
cd client_src
LD_PRELOAD=../InTCPtor/build/libintcptor-overrides.so ant run
```

**Windows klient** - spusť normálně (bez InTCPtor):
```cmd
cd client_src
ant run
```

## 5. Konfigurace InTCPtor

Po prvním spuštění se vytvoří `intcptor_config.cfg`:

```ini
# Výchozí hodnoty
Send__1B_Sends = 0.1              # 10% šance - posílat po 1 bytu
Send__2B_Sends = 0.1              # 10% šance - posílat po 2 bytech
Send__2_Separate_Sends = 0.3      # 30% šance - rozdělit na polovinu
Send__2B_Sends_And_Second_Send = 0.2
Recv__1B_Less = 0.1               # 10% šance - přečíst o 1 byte méně
Recv__2B_Less = 0.1
Recv__Half = 0.3                  # 30% šance - přečíst polovinu
Recv__2B = 0.2                    # 20% šance - přečíst max 2 byty
Send_Delay_Ms_Mean = 100          # Průměrné zpoždění 100ms
Send_Delay_Ms_Sigma = 10
Drop_Connections = 0              # 0 = nevypínat spojení náhodně
Drop_Connection_Delay_Ms_Min = 5000
Drop_Connection_Delay_Ms_Max = 15000
Log_Enabled = 1
```

### Extrémní test (pro ověření robustnosti)

```ini
# Agresivní nastavení
Send__1B_Sends = 0.3
Send__2B_Sends = 0.2
Send_Delay_Ms_Mean = 200
Drop_Connections = 1              # Zapni náhodné odpojování
Drop_Connection_Delay_Ms_Min = 10000
Drop_Connection_Delay_Ms_Max = 30000
```

## 6. Co testovat

### Test 1: Základní hra
1. Spusť server s InTCPtor
2. Připoj oba klienty
3. Odehraj celou hru
4. ✅ Hra musí doběhnout bez pádů

### Test 2: Reconnect
1. Nastav `Drop_Connections = 1`
2. Hraj hru
3. InTCPtor náhodně zavře spojení
4. ✅ Klient se automaticky reconnectne
5. ✅ Hra pokračuje bez zásahu uživatele

### Test 3: Fragmentované zprávy
1. Nastav `Send__1B_Sends = 0.5`
2. Hraj hru
3. ✅ Všechny zprávy musí projít správně
4. ✅ Žádné rozsypané/neúplné zprávy v UI

## 7. Očekávané chování

| Situace | Očekávaný výsledek |
|---------|-------------------|
| Fragmentované zprávy | Server/klient správně složí |
| Zpoždění | Hra běží, jen pomalejší |
| Náhodné odpojení | Automatický reconnect |
| Server restart | Klient se vrátí na login |

## 8. Co se NESMÍ stát

- ❌ Pád aplikace (segfault, exception)
- ❌ Problikávání UI
- ❌ Dialog vyžadující kliknutí během reconnectu
- ❌ Neúplná/rozsypaná data v UI
- ❌ Deadlock/zamrznutí

## 9. Typický průběh prezentace

```
Linux PC #1 (Server):
  $ cd InTCPtor && ./build_and_env.sh
  $ cd ../server_src && make
  $ cd ../InTCPtor/build
  $ ./intcptor-run ../../server_src/nim_server -p 10000 -v

Linux PC #1 (Klient A):
  $ cd client_src && ant run
  > Připoj se jako "HracLinux"

Windows PC #2 (Klient B):
  > cd client_src
  > ant run
  > Připoj se jako "HracWindows"

Hra:
  1. HracLinux vytvoří místnost
  2. HracWindows se připojí
  3. Odehrají hru
  4. Učitel může upravit intcptor_config.cfg pro horší podmínky
  5. Hra musí stále fungovat
```

## 10. Troubleshooting

### InTCPtor nefunguje
```bash
# Ověř, že jsi na Linuxu
uname -a

# Ověř, že .so existuje
ls -la InTCPtor/build/libintcptor-overrides.so
```

### Server padá pod InTCPtor
```bash
# Spusť s GDB pro debug
gdb --args ./nim_server -p 10000 -v
(gdb) run
# Po pádu:
(gdb) bt
```

### Klient se nepřipojí
```bash
# Ověř firewall
sudo ufw allow 10000/tcp

# Ověř, že server běží
netstat -tlnp | grep 10000
```

