# Cardboard: Summary for Claude Code

Written in a Claude Cowork session on 2026-10-07 from the team's handoff document and the university assignment brief. Read this before working on the project.

## Project

- **Cardboard**: a 2-player multiplayer card game (TCG), Unreal Engine 5.7, PC/Steam (dev App ID 480).
- Project root: `D:/Studia/Cardboard` (`Cardboard.uproject`). Content is under `/Game/TCG_Main/`.
- University team project ("Projektowanie gier sieciowych"), in a team of 3.
- Split: **C++** handles sessions, replicated state and infrastructure. **Blueprints** handle GameModes, maps, the PlayerController, UI and game flow. Keep it that way, and don't move Blueprint classes to C++ without a concrete reason.

## Tips

- Blueprints (`.uasset`) are binary. To share or review a graph or widget layout, select the nodes (or widgets in the Hierarchy), press Ctrl+C and paste them as text.
- There's no second PC for Steam testing yet. Don't call the multiplayer flow verified until it has been tested over Steam on two machines.

## C++ classes (in `Source/`)

- `UMyGameInstance`: the game instance.
- `UMyUIManagerSubsystem` (GameInstanceSubsystem): creates `WBP_MenuRoot`, removing any existing root first because it persists across map changes.
- `UMenuRootWidget`: base class of `WBP_MenuRoot`. Its BindWidgets are MainMenu, SessionMenu, HostMenu, Lobby and Settings, each with a `Show*()` function.
- `UMySessionSubsystem` (GameInstanceSubsystem): Steam sessions.
  - Functions: `CreateLobby(Name, ESessionPrivacy)`, `FindLobbies()`, `JoinLobby(Index)`, `DestroyLobby()`.
  - Delegates: OnSessionCreated, OnSessionsFound, OnSessionJoined, OnSessionDestroyed and OnUnexpectedDisconnect. `AvailableSessions` is an array of `FSessionInfo`.
  - Sessions are 2 public slots, use presence and lobbies, store `LOBBY_NAME`, and search with `SEARCH_LOBBIES`.
  - When a session is created it calls `ServerTravel("/Game/TCG_Main/Maps/L_Lobby?listen")`.
  - Stale-session recovery: if a session already exists it destroys it and then creates the new one (`bCreateAfterDestroy`).
  - `HandleNetworkFailure`: on a client only, broadcasts OnUnexpectedDisconnect and calls DestroyLobby. `HandlePreExit` destroys the session on exit.
- `ALobbyGameState` (GameStateBase): overrides AddPlayerState/RemovePlayerState to broadcast `OnLobbyPlayersChanged`. `NotifyReadyStateChanged()` broadcasts the same delegate.
- `ALobbyPlayerState` (PlayerState): `bIsReady` replicates with `OnRep_IsReady`. `SetReady(bool)` is authority-only and calls `OnRep_IsReady()` by hand. `IsReady()` returns the value.
  - `OnRep_IsReady()` calls `ALobbyGameState::NotifyReadyStateChanged()`, and `SetReady` is `BlueprintAuthorityOnly` (applied).

Steam setup is in `Config/DefaultEngine.ini`: the SteamSockets net driver (the legacy IP driver failed), `DefaultPlatformService=Steam`, `bUseSteamNetworking=true`, `bInitServerOnClient=true`. The project also includes the AdvancedSessions plugin but doesn't use it for the core session logic.

## Blueprints

- GameModes: `GM_Menu_TCG` (L_MainMenu), `GM_Lobby_TCG` (L_Lobby), `GM_Gameplay_TCG` (L_Gameplay).
- `GM_Lobby_TCG` should use GameState = `LobbyGameState`, PlayerState = `LobbyPlayerState` and PlayerController = `PlayerController_TCG`.
- `PlayerController_TCG` is Blueprint only, and is used on L_Lobby **and** L_Gameplay. On BeginPlay, if it's the local controller:
  - Then 0: Get Current Level Name → Branch (NOT contains "L_Gameplay") → True: InitializeUI → Set Input Mode Game And UI → Show Mouse Cursor; False: skip InitializeUI, just set input mode and cursor. Get Current Level Name is an exec node, so it must run *before* the Branch.
  - Then 1: shows the lobby UI if the map name contains "L_Lobby".
  - Then 2: binds OnUnexpectedDisconnect to a custom event that opens L_MainMenu.
  - It also has the `Server_SetReady(bool New Ready)` RPC (Run on Server, Reliable): **Player State** getter (the controller's own variable, *not* GameplayStatics "Get Player State" with an index) → Cast To LobbyPlayerState → Set Ready → Get Game State → Cast To LobbyGameState → Notify Ready State Changed.
- Widgets: WBP_MenuRoot, WBP_MainMenu, WBP_SessionMenu, WBP_HostMenu, WBP_Lobby, WBP_Settings, WBP_Button, WBP_SessionEntry.
- `WBP_Lobby` has player 1 and 2 panels, `Player1_Checkmark` and `Player2_Checkmark` (hidden by default), `Ready_Button`, START GAME, LEAVE and a status text.
  - `RefreshLobbyUI` is large and works. It's bound to OnLobbyPlayersChanged and sets names, host/client labels, the Start button state, status text and the checkmarks (from PlayerArray[i] → Cast LobbyPlayerState → IsReady).
  - Host Leave calls DestroyLobby, waits for OnSessionDestroyed, then opens L_MainMenu. Client Leave opens L_MainMenu.

## Testing multiplayer on one PC

Steam can't run two clients on one machine, so local tests bypass it:

1. Editor Preferences → search `launch parameters` → put `-nosteam` in **Additional Launch Parameters** (and the server variant if shown). SteamSockets then fails to load and the config's `DriverClassNameFallback` switches both sides to `IpNetDriver`.
2. Open **L_Lobby** directly, Play options: Number of Players = 2, Net Mode = Play As Listen Server.
3. Both windows start in the lobby without a Steam session (lobby name may be blank). Clear `-nosteam` to test real Steam sessions again.

The "Video memory has been exhausted" message in this setup is just the editor + 2 game windows sharing one GPU, not a project bug.

## Progress log

**2026-10-07 (Claude Code session):**

- "Cast To LobbyPlayerState fails" is fixed. Ready on host: confirmed working.
- Client Ready toggled the host's flag: `Server_SetReady` used GameplayStatics "Get Player State" (index 0 = always host). Replaced with the controller's own Player State getter. Ready on client: **confirmed working** (2-player PIE).
- Lobby → gameplay travel: **confirmed working** in 2-player PIE with `-nosteam` (client gets "Join succeeded" on L_Gameplay). The earlier "client rejected" problem didn't reproduce over IP. **Untested over Steam.**
- Main menu appeared on L_Gameplay because PlayerController_TCG BeginPlay always ran InitializeUI. Now skipped on L_Gameplay: **confirmed working**.
- UI scaling: Project Settings → User Interface → DPI Scale Rule = Scale To Fit, Design Screen Size 1920×1080.
- Known harmless log: right after travel, WBP_Lobby's 0.5s Delay → RefreshLobbyUI fires once more and prints "LobbyGameState CAST FAILED". Clean up with the debug prints.

**2026-10-08:**

- `ALobbyGameState::AreAllPlayersReady()` (C++, BlueprintPure): false if fewer than 2 players, otherwise every player must be a ready LobbyPlayerState. Used in two places, both **confirmed working**:
  - WBP_Lobby → RefreshLobbyUI: Set Is Enabled (StartGame_Button) = AreAllPlayersReady.
  - GM_Lobby_TCG → StartGame: Cast To LobbyGameState → Branch(AreAllPlayersReady) → ServerTravel (server-side guard).
- ServerTravel in GM_Lobby_TCG is the **AdvancedSessions** node — list the plugin in the docs.
- Lobby player names: `ShortenName` (pure function in WBP_Lobby) truncates past 15 chars with "...", names sit in Scale Boxes (Scale to Fit, Down Only, slot H-Align Fill). Player 1/2 panels now use identical slot settings. Confirmed working.
- **First Steam test (two machines): travel broken.** Non-seamless ServerTravel made the host recreate its Steam listen socket on vport 7777 before the old one was released (`Cannot create listen socket. Already have a listen socket on P2P vport 7777` → `LoadMap: failed to Listen` → host falls back to L_MainMenu). The client was disconnected and refused while reconnecting.
- Fix applied, **untested**: Seamless Travel. `GM_Lobby_TCG` → Use Seamless Travel = true; new empty map `L_Transition` set as Project Settings → Maps & Modes → Transition Map (also add it to the packaging map list later).
  - Watch for: seamless travel keeps the existing `PlayerController_TCG` (same class on L_Gameplay), so its BeginPlay may not fire again on L_Gameplay (UI/cursor setup). Seamless travel doesn't run in PIE by default — test standalone over Steam.

**2026-10-09 (branch `gameplay_seating`):**

- Seamless travel over Steam (two machines): **confirmed working**. Both players reach L_Gameplay.
- `ASeatedPawn` (C++) is the gameplay pawn. Its components are SeatRoot → YawPivot → PitchPivot → Camera, plus a TableViewPoint.
  - Free mouse look reads the raw mouse delta (`GetInputMouseDelta` in Tick), because the Enhanced Input mouse action never delivered a value. Yaw is limited to ±100° and pitch to −60..40°.
  - Space (`IA_TableView` in `IMC_MouseLook`) glides the camera to TableViewPoint and shows the cursor. Pressing it again glides back.
  - `LookRotation` replicates to the other player (`COND_SkipOwner` plus an unreliable server RPC), so each player sees the other's cutout turn. Network rotators arrive as 0..360, so the code normalises them before clamping; without that, the client's left/down look snapped to the opposite limit.
  - `SeatIndex` is set when the pawn spawns (ExposeOnSpawn) and replicates once (`COND_InitialOnly`).
  - `OnTableViewChanged(bool)` is a Blueprint event, the hook for showing the hand of cards later.
  - **Confirmed working** in 2-player PIE.
- `BP_Table` (`Content/TCG_Main/Gameplay`) has a TableTop mesh, `Seat_0` and `Seat_1` arrows facing each other, and a pure function `GetSeatTransform(SeatIndex)`. It's placed in L_Gameplay, which uses External Actors so the team can edit the level in parallel.
- `GM_Gameplay_TCG` overrides **HandleStartingNewPlayer**, which also runs for seamless-travel players. It doesn't call the parent, so no default pawn spawns. The steps are:
  - Find BP_Table once and pick a random first seat.
  - Spawn `BP_SeatedPawn` at `GetSeatTransform(NextSeatIndex)`, with Seat Index and Owner set.
  - Possess it, then set `NextSeatIndex = 1 − NextSeatIndex`.
  - Players sitting opposite each other is **confirmed working**.
- Random table yaw: Set Actor Rotation (random yaw 0..360) runs on the server before the first spawn. BP_Table has Replicates and Replicate Movement on. **Confirmed working.**
- Both players currently use the same cutout. Per-seat Danny/Fiona cutouts are postponed.

**2026-10-09 (later):**

- `ACardActor` (C++) is one card. The placeholder is the engine cube scaled to 6.35 × 8.89 cm (2.5″ × 3.5″), and only Visibility traces hit it.
- Hand, in `ASeatedPawn`, **confirmed working**:
  - The local player gets up to 6 placeholder cards (`MaxHandSize`). They're attached to `HandRoot`, a child of the camera, so in table view they always rise from the bottom of the screen.
  - The hand is local only, so the other player never has it.
  - Hover is picked by hand slot rather than by trace, so it doesn't flicker. The hovered card faces the screen, comes forward and rises.
  - Holding left mouse drags a card. Moving it sideways reorders the hand. Positions of cards moved towards the camera are corrected for perspective.
  - The left mouse button is read raw with `IsInputKeyDown`, like the mouse look.
- Board, `ACardTable` (C++, now the parent class of BP_Table), **confirmed working** in 2-player PIE:
  - There's one replicated row of `FBoardCard` (only an `InstanceId` for now) per seat, up to 7 cards (`MaxRowSize`). Rows are centred, and each is ordered left to right from its own seat.
  - Each machine spawns its own card visuals from the rows and slides them into place.
  - Dragging a card above the hand opens a local gap in your row where it would land. Releasing calls `ServerPlaceCard(InsertIndex)` on the pawn, and the server inserts the card. Both players see it.
  - The server currently trusts the client, because the hand isn't server-side yet.

**2026-10-09, card data (branch `gameplay_cards`):**

- The card sheet lives in `Data/Cards.csv` (edited in Excel) with the columns `ID,CardName,Cost,Attack,Health,Trait,AbilityId`.
  - It's imported as the **DT_Cards** DataTable (`Content/TCG_Main/Gameplay`), with row struct `FCardDefinition`. The row name is the card ID.
  - After editing the sheet, right-click DT_Cards → Reimport.
  - Save it with commas. Polish Excel may use `;` instead.
- Trait is the `ECardTrait` enum: None, Taunt, Fly or Stealth. There's one per card, and new traits are added in `CardDefinition.h`. AbilityId (FName, empty = none) is reserved for later.
- The game finds the table through **Project Settings → Game → Cardboard → Card Data Table** (`UCardboardSettings`, stored in DefaultGame.ini).
  - It's a soft reference, so **add DT_Cards (or its folder) to the packaging cook list** before making a packaged build.
- `ACardActor::SetCard(CardId)` shows the cost, name, trait, attack and health as text on the card's front.
- The placeholder hand draws random cards from the table. `FBoardCard` now replicates the CardId, so both players see which card was played.
- **Confirmed working.** Traits are shown but have no gameplay effect yet.
- The deck is currently every row of the sheet once (30 rows = 30 cards).

## Next tasks

1. Server-side deck and hand: shuffle the 30 cards, deal the opening hand, and have the server check every play. This replaces the local placeholder hand.
2. Attacking and combat, including Taunt, Fly and Stealth.
3. Card game rules: 5-card opening draw, mana +1 per round, turns.
4. Optional: per-seat cutouts (Danny for seat 0, Fiona for seat 1, chosen by `SeatIndex`). For now both players use the same cutout mesh, `Player/f_player`, renamed from `f_player_danny`.
5. Remove the temporary debug prints once each step works (including GM_Lobby_TCG's literal `"STARTING GAME - PLAYERS: " + player count` print).

Ready button flow (working):

```
OnClicked → Get Owning Player → Cast PlayerController_TCG → Player State
→ Cast LobbyPlayerState → IsReady → Branch → Server_SetReady(!IsReady)
```

The button must not set the checkmark itself. The checkmark only reflects the replicated `bIsReady`.

Gameplay travel URL:

```
/Game/TCG_Main/Maps/L_Gameplay?game=/Game/TCG_Main/GameMode/GM_Gameplay_TCG.GM_Gameplay_TCG_C
```

## Assignment requirements and their status

| Requirement | Status |
|---|---|
| Menu: host, join, server browser | Done |
| Lobby and ready system; host starts only when everyone is ready | Done (2-player PIE; untested over Steam) |
| Travel to the gameplay level | Done (seamless travel; works over IP and over Steam on two machines) |
| Clear goal: a PVP or PVE win condition | Not started (card game rules) |
| End of match: automatic stop for all players, summary screen (results/stats), restart / next level / back to lobby | Not started |
| Packaged standalone build, ready to share | Not done. Include `steam_appid.txt` for testing |
| Docs: how to run it, feature list, assets and plugins used (SteamSockets, OnlineSubsystemSteam, AdvancedSessions), split of work in the team, **how much was AI-generated** | Not started |

Optional extras in the brief: lobby chat or settings, map selection, detailed stats, ranking, saved match history, progression.
