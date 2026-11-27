# Cardboard

Sieciowa gra karciana dla dwóch graczy (PvP), w stylu Hearthstone'a, osadzona w kartonowym świecie.
Projekt zaliczeniowy z przedmiotu **Projektowanie gier sieciowych**, wykonany w **Unreal Engine 5.7**
dla **Windows (PC)**, z rozgrywką sieciową przez **Steam**.

Gracze siedzą naprzeciw siebie przy stole w zaułku, zagrywają karty-zwierzęta, atakują nimi
karty przeciwnika lub samego przeciwnika. Wygrywa ten, kto pierwszy sprowadzi zdrowie rywala do zera.

---

## Spis treści

1. [Instrukcja uruchomienia](#1-instrukcja-uruchomienia)
2. [Opis funkcjonalności](#2-opis-funkcjonalności)
3. [Spis użytych assetów i pluginów](#3-spis-użytych-assetów-i-pluginów)
4. [Podział obowiązków w zespole](#4-podział-obowiązków-w-zespole)
5. [Wykorzystanie AI](#5-wykorzystanie-ai)

---

## 1. Instrukcja uruchomienia

### 1.1. Wersja spakowana (zalecana)

Wymagania: Windows 10/11 (64-bit), karta graficzna z obsługą DirectX 12, zainstalowany i
**zalogowany Steam**.

1. Rozpakuj archiwum z grą w dowolnym folderze.
2. Uruchom **Steam** i zaloguj się.
3. Uruchom `Cardboard.exe`.
4. Jeden gracz wybiera **Host** i tworzy lobby, drugi wybiera **Join**, odświeża listę sesji
   (wyszukiwarka serwerów) i dołącza do lobby.

Uwagi:
- Każdy z graczy musi być zalogowany na **innym koncie Steam** — Steam nie pozwala dołączyć do
  własnej sesji.
- Gra używa testowego identyfikatora aplikacji Steam **480 (Spacewar)**. Jeśli Steam pokazuje,
  że gracz „gra w Spacewar”, to jest to oczekiwane.
- Logi gry zapisują się w `Cardboard\Saved\Logs\Cardboard.log` (w folderze z grą).

### 1.2. Test lokalny na jednym komputerze (bez Steam)

Dwie kopie gry nie mogą jednocześnie korzystać ze Steam na jednym komputerze, dlatego do testów
lokalnych gra uruchamiana jest z parametrem `-nosteam` (połączenie przez sieć lokalną / IP):

```powershell
Start-Process ".\Cardboard.exe" -ArgumentList "-nosteam","-windowed","-ResX=1280","-ResY=720"
```

Polecenie należy wykonać dwa razy (w folderze z grą). W jednym oknie hostujemy, w drugim dołączamy.

### 1.3. Uruchomienie z kodu źródłowego

Wymagania: **Unreal Engine 5.7**, **Visual Studio 2022** z komponentem
„Game development with C++”.

1. Sklonuj repozytorium.
2. Kliknij prawym przyciskiem `Cardboard.uproject` → **Generate Visual Studio project files**.
3. Otwórz `Cardboard.uproject` (przy pierwszym uruchomieniu edytor zaproponuje kompilację modułu
   C++ — należy się zgodzić).
4. Test w edytorze: w **Editor Preferences → Level Editor → Play → Additional Launch Parameters**
   wpisz `-nosteam`, otwórz `L_MainMenu` i uruchom grę z **Number of Players = 2**,
   **Net Mode = Play As Listen Server** (lub w trybie Standalone).

### 1.4. Budowanie wersji spakowanej

W edytorze: **Platforms → Windows → Package Project**, lub z wiersza poleceń:

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="<ścieżka>\Cardboard.uproject" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -archivedirectory="<folder wyjściowy>"
```

Lista map do spakowania oraz katalogi z tabelami kart są ustawione w `Config/DefaultGame.ini`
(sekcja `ProjectPackagingSettings`).

---

## 2. Opis funkcjonalności

### 2.1. Realizacja wymagań projektu

| Wymaganie | Realizacja w grze |
|---|---|
| Gra multiplayer w czasie rzeczywistym | Rozgrywka 1 na 1 przez Steam (listen server). Na żywo synchronizowane są m.in. ruchy głowy gracza, ułożenie i podnoszenie kart w ręce, strzałka celowania podczas ataku oraz liczniki czasu tury. |
| Menu: host / join / wyszukiwarka serwerów | Menu główne z opcjami **Host** (nazwa i prywatność lobby), **Join** z listą znalezionych sesji, **Settings** i **Quit**. |
| Lobby z systemem „ready” | Dwa miejsca (host i gość), przycisk **Ready** z oznaczeniem gotowości, host uruchamia grę dopiero, gdy obaj gracze są gotowi (sprawdzane również po stronie serwera). |
| Przejście na poziom rozgrywki | Seamless travel z lobby na poziom `L_Gameplay` — sesja Steam i połączenie zostają zachowane. |
| Jasno określony cel | **PvP:** sprowadzić zdrowie przeciwnika (15) do zera — atakami kart, zdolnościami lub zmęczeniem talii (fatigue). |
| Automatyczne zakończenie | Gdy zdrowie gracza spadnie do zera, mecz kończy się dla obu graczy jednocześnie (napis VICTORY / DEFEAT). |
| Podsumowanie (wyniki, statystyki) | Ekran podsumowania: wynik, liczba tur, czas meczu oraz statystyki obu graczy (zagrane karty, wydana mana, obrażenia zadane przeciwnikowi i kartom, zniszczone i stracone karty, liczba tur, pozostałe zdrowie). |
| Restart rozgrywki | Przycisk **Rewanż** — nowy mecz startuje, gdy obaj gracze go zatwierdzą. |
| Kolejny poziom | Gra ma jeden poziom rozgrywki (stół w zaułku), więc ta opcja nie występuje. |
| Powrót do lobby | Przycisk **Powrót do lobby** na ekranie podsumowania przenosi obu graczy do lobby. |
| Wersja niezależna od silnika | Spakowana wersja dla Windows (`Cardboard.exe`), patrz punkt 1.1. |

### 2.2. Zasady gry

- **Początek meczu:** rzut monetą decyduje, kto zaczyna. Każdy gracz dobiera 3 karty i może je
  wymienić (**mulligan**, 30 s). Gracz drugi dostaje dodatkowo kartę **The Coin** (+1 mana na jedną turę).
- **Tura:** trwa 30 sekund. Na początku tury gracz dostaje +1 do maksymalnej many (do 10),
  odnawia manę i dobiera kartę. Karty zagrywa się, przeciągając je z ręki na stół.
- **Ataki:** karta na stole może atakować od następnej tury (wyjątek: **Charge**). Celem może być
  karta przeciwnika lub sam przeciwnik. Obie karty zadają sobie nawzajem obrażenia równe swojemu ataku.
- **Limity:** maksymalnie 6 kart w ręce (nadmiarowo dobrane karty przepadają) i 6 kart na stole
  po każdej stronie.
- **Zmęczenie (fatigue):** dobieranie z pustej talii zadaje graczowi rosnące obrażenia: 1, 2, 3…

**Cechy kart:**

| Cecha | Działanie |
|---|---|
| Taunt | Przeciwnik musi najpierw zniszczyć tę kartę, zanim zaatakuje gracza lub inne karty. |
| Fly | Może atakować z pominięciem kart z Taunt. |
| Stealth | Nie może być celem ataku, dopóki sama nie zaatakuje. |
| Charge | Może atakować w turze, w której została zagrana. |

**Zdolności przy zagraniu karty:** przeciwnik dobiera N kart, dobierz N kart, ulecz N zdrowia,
zadaj N obrażeń przeciwnikowi.

### 2.3. Karty i talie

W grze są **22 karty-zwierzęta**. Ich statystyki zostały zbalansowane według zasady z podstawowego
zestawu Hearthstone'a: suma ataku i zdrowia ≈ 2 × koszt + 1, pomniejszona o „koszt” cech i zdolności.
Dane kart znajdują się w `Data/Cards.csv` (tabela `DT_Cards`).

Przed meczem każdy gracz wybiera w lobby jedną z trzech talii (po 22 karty, `Data/Decks.json`):

| Talia | Styl gry |
|---|---|
| **Swarm** | Szybki atak: tanie karty z Charge i Fly, obrażenia zadawane bezpośrednio przeciwnikowi. |
| **Bury Them** | Kontrola: karty z Taunt i leczeniem, a Mouse i Donkey zmuszają przeciwnika do dobierania kart, aż wyczerpie talię i zacznie otrzymywać obrażenia ze zmęczenia. |
| **Big Beasts** | Późna gra: przetrwanie pierwszych tur i przejęcie stołu dużymi kartami (Bear, Hippo). |

Przycisk **Lista kart** w lobby pokazuje wszystkie karty wybranej talii z kosztem, statystykami i zdolnościami.

### 2.4. Funkcje dodatkowe

- **Wybór talii w lobby** (rozszerzenie „zaawansowane lobby”) — wybór jest zapamiętywany przy
  przejściu do meczu i z powrotem do lobby.
- **Rozbudowane statystyki i zapis historii** — po każdym meczu wynik i statystyki zapisują się
  lokalnie (`Saved/SaveGames/MatchHistory.sav`), a ekran podsumowania pokazuje bilans wygranych i porażek.
- **Ustawienia** (z menu głównego i w trakcie meczu): rozdzielczość, tryb okna, jakość grafiki,
  V-Sync, skala rozdzielczości, jasność, pole widzenia, głośność (ogólna, muzyka, efekty) i czułość myszy.
  Ustawienia zapisują się między uruchomieniami gry.
- **Menu pauzy w meczu** (klawisz **Esc**): Wznów, Ustawienia, Wyjdź do menu głównego.
  Wyjście hosta kończy mecz dla obu graczy.
- **Animowane menu główne** — w tle spadają losowe karty z gry, odbijając się od siebie (fizyka).
- **Obsługa rozłączenia** — gdy host wyjdzie lub połączenie zostanie przerwane, gość wraca do menu głównego.

### 2.5. Sterowanie

| Akcja | Sterowanie |
|---|---|
| Rozglądanie się | Mysz |
| Widok stołu (zagrywanie i atakowanie kartami) | TODO: klawisz przypisany do `IA_TableView` w `IMC_MouseLook` |
| Zagranie karty | Przeciągnięcie karty z ręki na stół |
| Atak | Przeciągnięcie od swojej karty do celu (strzałka celowania) |
| Menu pauzy | Esc |

### 2.6. Architektura (skrót)

Projekt łączy C++ i Blueprinty: **C++** odpowiada za sesje, stan gry replikowany przez sieć i
logikę po stronie serwera, a **Blueprinty** za tryby gry (GameMode), mapy, kontroler gracza i wygląd UI.
Serwer jest autorytatywny — klient wysyła tylko prośby (RPC), a serwer je weryfikuje.

| Klasa (C++) | Rola |
|---|---|
| `UMySessionSubsystem` | Tworzenie, wyszukiwanie, dołączanie i zamykanie sesji Steam, powrót do menu głównego. |
| `ALobbyGameState`, `ALobbyPlayerState` | Stan lobby: gracze, gotowość, host. |
| `ACardPlayerState` | Wybrana talia, przenoszona między lobby a meczem. |
| `ACardGameState` | Przebieg meczu: rzut monetą, mulligan, tury z limitem czasu, zwycięzca. |
| `ACardTable` | Karty na stole po obu stronach, ataki, cechy kart (replikowane). |
| `ASeatedPawn` | Gracz przy stole: ręka, talia, mana, zdrowie, zagrywanie kart, zdolności, statystyki. |
| `ACardActor` | Wygląd pojedynczej karty (model, materiał, liczby). |
| `UCardboardUserSettings`, `USettingsWidget` | Ustawienia gracza i ich ekran. |
| `USummaryWidget`, `UMatchHistorySave` | Ekran podsumowania i lokalna historia meczów. |
| `UDeckPickerWidget`, `UDeckCardListWidget`, `UPauseMenuWidget` | Wybór talii, lista kart, menu pauzy. |
| `ACardRainActor` | Spadające karty w tle menu głównego. |

---

## 3. Spis użytych assetów i pluginów

### 3.1. Pluginy

| Plugin | Autor / źródło | Zastosowanie |
|---|---|---|
| Online Subsystem Steam | Epic Games (wbudowany w UE) | Sesje i połączenie przez Steam. |
| Steam Sockets | Epic Games (wbudowany w UE) | Sterownik sieciowy oparty na Steam (SteamSocketsNetDriver). |
| Advanced Sessions / Advanced Steam Sessions | Joshua Statzer (mordentral), licencja MIT | Węzły Blueprint do sesji (m.in. przejście z lobby do gry). |
| Enhanced Input | Epic Games (wbudowany w UE) | Obsługa sterowania. |

### 3.2. Assety

| Asset | Źródło | Licencja |
|---|---|---|
| Model karty, grafiki kart | Wykonane przez zespół (SmollMaggie, GTZMCHN) | Własne |
| Model zaułka (poziom rozgrywki), skrzynie | Wykonane przez zespół (SmollMaggie) | Własne |
| Kontener na śmieci, worki i sterty śmieci, skrzynka elektryczna | Fab.com — TODO: nazwy pakietów | Fab Standard License |
| Dźwięki przycisków i otwierania kartonu | Freesound.org, użytkownik *newagesoup*: „hit tap strike cardboard box 13 times” (pocięty na pojedyncze dźwięki) — TODO: link | TODO: licencja ze strony dźwięku (CC0 / CC BY) |
| Tekstury kartonu w interfejsie | TODO: nazwa strony | TODO: licencja (darmowe) |
| Czcionka **Permanent Marker** | Font Diner, Google Fonts | Apache License 2.0 |
| Zawartość szablonów UE (m.in. materiały silnika) | Epic Games | Licencja Unreal Engine |

---

## 4. Podział obowiązków w zespole

| Osoba | Zakres prac |
|---|---|
| **TODO: twój nick** | Programowanie (C++ i Blueprinty): sesje Steam, menu główne, lobby z systemem ready, przebieg meczu i zasady gry, karty i talie, interfejs (podsumowanie, wybór talii, ustawienia, menu pauzy), menu główne z animowanym tłem, przygotowanie wersji spakowanej. |
| **SmollMaggie** | Grafika 3D i 2D: model karty, tekstury i grafiki kart, poziom rozgrywki i model zaułka wraz z otoczeniem, placeholdery postaci graczy. |
| **GTZMCHN** | Grafiki kart. |

---

## 5. Wykorzystanie AI

Przy projekcie korzystaliśmy z asystenta AI **Claude** (Anthropic) — w aplikacji Claude
(Claude Code / Cowork) — od października 2026. AI pracowało jako asystent programisty:
proponowało i pisało kod, a zespół testował wszystko w grze, wprowadzał zmiany w edytorze
i podejmował decyzje projektowe.

**Zakres treści wygenerowanych lub współtworzonych z AI:**

- **Kod C++** — znaczna część klas rozgrywki i interfejsu: przebieg meczu (`ACardGameState`),
  stół i ataki (`ACardTable`), gracz przy stole (`ASeatedPawn`), wygląd kart (`ACardActor`),
  zdolności kart, talie i ich wybór (`ACardPlayerState`, widżety wyboru talii i listy kart),
  ekran podsumowania i historia meczów, ustawienia, menu pauzy, powrót do menu głównego,
  animowane tło menu oraz poprawki w obsłudze lobby i sesji.
- **Diagnozowanie błędów** — m.in. problemy z przejściem lobby → gra przez Steam (seamless travel),
  awaria klienta przy powrocie do lobby, problemy z czcionkami i materiałami.
- **Instrukcje krok po kroku do edytora** — Blueprinty i układy widżetów (UMG) zespół wykonywał
  samodzielnie w edytorze według wskazówek AI, AI sprawdzało wklejone grafy i hierarchie.
- **Balans kart i projekt talii** — statystyki 22 kart i skład trzech talii (zaproponowane przez AI,
  zatwierdzone i modyfikowane przez zespół).
- **Konfiguracja projektu** — ustawienia pakowania i budowanie wersji spakowanej.
- **Ta dokumentacja** — pierwsza wersja tekstu, uzupełniona i sprawdzona przez zespół.

**Wykonane bez udziału AI:** wszystkie grafiki, modele 3D, tekstury, poziom rozgrywki, dźwięki
(wybór i montaż), koncepcja gry i wygląd interfejsu.

TODO: zespół — sprawdźcie i popraw ten rozdział, jeśli zakres pracy z AI wyglądał inaczej
(np. które części sesji i lobby powstały przed rozpoczęciem pracy z AI).
