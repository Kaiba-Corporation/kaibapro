# Bot Theater script fixes

Corrected copies of card scripts that raised errors (or did the wrong thing) in automatic test
duels with TDOANE's cards. They are packed into `bot_theater_fixes.zip`, which goes in the game's
`expansions` folder. The game reads scripts from zip files in `expansions` before the `script`
folder, so these copies are used instead of TDOANE's, and TDOANE updates don't undo them.
Deleting the zip goes back to TDOANE's scripts.

The zip also holds `bot_theater_strings.conf`, the game's texts for choices that only come up in
four-player duels ("Attack Player 3 directly" and so on).

To rebuild the zip after changing a file here (from this folder):
`zip -r bot_theater_fixes.zip script bot_theater_strings.conf`

## Loops that let you choose again (222 scripts)

Many of TDOANE's scripts (Polymerization among them) let the player back out of choosing
materials and choose again, using `goto cancel`. When the engine says materials exist but
then can't find any, the script loops without end, and a bot gets asked the same thing
32,768 times (about 17 minutes of the duel looking frozen). `limit_cancel.py` adds a limit of
5 tries to every such loop; after that the effect simply ends. To redo this after a TDOANE
update (from this folder): `python limit_cancel.py PATH_TO_GAME/script script script`
(it starts from the copies already here, so the other fixes below are kept).

## Other fixes

| Card | Script | Problem | Fix |
|---|---|---|---|
| Ultimate Tyranno | c15894048 | Its "other monsters cannot attack" check called itself without end ("C stack overflow"), so the restriction failed | The public script, which checks the condition the other way round |
| Performapal Splashmammoth | c52963531 | Its activation check left out an argument ("Parameter 2 should be Effect") | Passes the effect |
| R-Genex Turing | c61052897 | Called its Synchro level helper without the Synchro Monster (nil 'sc') | Passes the Synchro Monster, as the public script does |
| Super Vehicroid - Stealth Union, Vylon Disigma, Transforming Sphere, D/D/D/D Super-Dimensional Sovereign Emperor Zero Paradox, and three anime versions (The Winged Dragon of Ra, Legendary Fiend, Goblin Attack Force) | c3897085, c39987184, c66094993, c97417863, c700000002, c700000066, c700000093 | Use `EFFECT_FLAG_COPY_INHERIT`, which this engine doesn't have | Define it as 0 (no effect) at the top of the script |
| Mirage Swordsman | c73936388 | Error when destroyed by battle (no reason effect) | Allows for that |
| Time Magic Hammer | c10960419 | Error when the monster it chose had a different current card number from its original (Pyro Clock of Destiny reads what it stores) | Stores it on the monster's own script table |
| Laughing Puffin, The Phantom Knights of Shadow Veil | c80275707, c77462146 | Error when the target was gone | Checks the target exists |
| Goblin Freefall Squad | c85123771 | Error when it had no target to cancel | Checks first |
| Morale Boost | c93671934 | Error when the event had no cards | Checks first |
| Battle Fusion | c43225434 | Error when the attack target was gone | Checks first |
