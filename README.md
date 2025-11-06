# Awale Game

A terminal-based implementation of the traditional African strategy game Awale (also known as Oware, Wari, or Mancala) written in pure C.

## Game Rules

1. **Setup**: The board has 12 pits (6 per player), each starting with 4 seeds
2. **Turns**: Players alternate turns, selecting a pit on their side
3. **Sowing**: Seeds from the selected pit are sown counter-clockwise, one per pit
4. **Empty Start**: The starting pit is left empty (seeds go to following pits)
5. **Capturing**: After sowing, if the last seed lands in an opponent's pit and that pit now has 2 or 3 seeds, those seeds are captured
6. **Chain Capturing**: Continue capturing backwards if previous opponent pits also have 2 or 3 seeds
7. **Feeding Rule**: If your opponent has no seeds, you must give them seeds if possible
8. **Protection**: You cannot capture all of your opponent's seeds
9. **Winning**: Game ends when one player captures 25+ seeds or no more moves are possible
10. **Victory**: Player with the most seeds wins

## How to Compile

```bash
make
```

## Start the game (server only fo now)
```bash
make run-server
```

## Test the rules
```bash
make run-server
```