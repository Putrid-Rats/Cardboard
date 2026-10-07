# Cardboard: Summary for Claude Code

Written in a Claude Cowork session on 2026-10-07 from the user's (Gordon's) handoff document and the university assignment brief. Read this before working on the project.

## Project

- **Cardboard**: a 2-player multiplayer card game (TCG), Unreal Engine 5.7, PC/Steam (dev App ID 480).
- Project root: `D:/Studia/Cardboard` (`Cardboard.uproject`). Content is under `/Game/TCG_Main/`.
- University team project ("Projektowanie gier sieciowych"), in a team of 3.
- Split: **C++** handles sessions, replicated state and infrastructure. **Blueprints** handle GameModes, maps, the PlayerController, UI and game flow. Keep it that way, and don't move Blueprint classes to C++ without a concrete reason.

## Working with Gordon

- He's a beginner to intermediate Unreal user. Give **exact step-by-step instructions**, **one change at a time**, and name the exact nodes and pins for Blueprints.
- Make small, incremental changes. Don't rewrite working graphs or architecture. Add Sequence branches and comment boxes instead of restructuring.
- When debugging, add **one checkpoint at a time**.
- Always label each thing as **confirmed working**, **implemented but untested**, **proposed**, or **broken**.
- **You can't read Blueprints** (`.uasset` is binary). Ask Gordon to select the nodes, press Ctrl+C and paste them as text. Use that rather than guessing.
- There's no second PC for Steam testing on two machines yet. His teammates may be able to help with that. Never claim the multiplayer lifecycle is verified just because the code exists.

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

## Next tasks

1. Lobby player names: truncate above 15 characters with "..." and shrink the font to fit the panel (in progress).
2. Test the full flow over Steam on two machines (teammates).
3. Remove the temporary debug prints once each step works (including GM_Lobby_TCG's literal `"STARTING GAME - PLAYERS: " + player count` print).

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
| Travel to the gameplay level | Works over IP (2-player PIE); untested over Steam |
| Clear goal: a PVP or PVE win condition | Not started (card game rules) |
| End of match: automatic stop for all players, summary screen (results/stats), restart / next level / back to lobby | Not started |
| Packaged standalone build, ready to share | Not done. Include `steam_appid.txt` for testing |
| Docs: how to run it, feature list, assets and plugins used (SteamSockets, OnlineSubsystemSteam, AdvancedSessions), split of work in the team, **how much was AI-generated** | Not started |

Optional extras in the brief: lobby chat or settings, map selection, detailed stats, ranking, saved match history, progression.
