# Speech bubbles (Bot Theater)

In bot rooms there's no chat during the duel: each bot's line comes up in a speech bubble,
only as big as its words, for a few seconds (longer for a long line) before it fades. A line
said while another is showing in the same place waits a moment for it.

- **One-on-one and match duels:** the bubble hangs under the speaker's portrait (the near
  player's at the top left, the far player's at the top right), one bubble each side.
- **Tag duels:** each side of the screen has two portraits, one from each team (the far team's
  on top). Both use the same bubble, hanging under the bottom portrait, one bubble each side at a
  time. For the bottom portrait the tail points straight up at it; for the top one it comes out
  of the bubble's outer top corner and runs up beside the bottom portrait to the top one's outer
  side. At the end of a tag duel only two players speak, one each side: the player whose turn
  it is (for their team) and, from the other team, the player on the other side of the screen
  (WindBot's `TagEndSpeaker`).
- **Four-player duels:** each player's bubble comes from their portrait (FOUR_PLAYER.md).
- **Five-a-side:** the speaker's name, picture and bubble beside the field, one speaker per team
  (FIVE_A_SIDE.md).

Code: `Game::ShowTwoBubble` / `DrawTwoBubbles` (two-player and tag), `ShowFourBubble` /
`DrawFourBubbles`, `ShowFiveBubble` / `DrawFiveSpeakers` in `gframe/drawing.cpp`; the chat
lines are routed in `DuelClient` (STOC_CHAT), and the room waits for the last bubbles before it
closes.
