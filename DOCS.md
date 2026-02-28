# Web HD

Web HD is a multiplayer game server Spelunky HD. New game modes will be added over time that may facilite head-to-head, large groups, and viewer spectating and interaction with your game through a web browser. This specifically is for the DLL portion of Web HD, which hooks into your game to connect with the game server.

## First Time Setup

### 1. Get an API Token

You need a MossRanking account and API token to use Web HD.

1. Go to [mossranking.com/settings](https://mossranking.com/settings)
2. Create an account or log in
3. Copy your API token

### 2. Configure the Plugin

1. Launch Spelunky HD, then launch Web HD from this tab at the top right.
2. Click **Settings** in the Web HD menu bar
3. Paste your API token into the **API Token** field
4. The **Server** field is pre-filled with the default server (`wss://webhd.mossranking.com/ws`) and should not need to be changed outside of development.

Your settings are saved automatically and persist between sessions.

### 3. Start a Lobby

1. Click **Play Online** in the menu bar
2. Select a game mode and click **Start**
3. Check the **Private lobby** box before starting if you don't want your lobby listed publicly on [webhd.mossranking.com](https://webhd.mossranking.com)

### 4. Share your lobby

Once connected, the **Manage** window shows your lobby info.

Depending on your game mode you might share a lobby url (with viewers) or a lobby id (when coordinating with other players). A button to copy the link or id will exist in the manage window for the game mode.

## Game Modes

### vs Chat

_Chat viewers interact with your Spelunky run by spawning enemies, items, and traps in real time._

Viewers earn points over time and spend them on interactions that affect your game. Interactions range from cheap nuisances like spawning a web or a snake to expensive plays like stunning the player.

Viewers pick an interaction and click on the level map to place it. Some interactions like Web Storm and Stun Player are global and don't require placement. Others like Spawn Bomb and Spawn Arrow let viewers aim with a velocity.
