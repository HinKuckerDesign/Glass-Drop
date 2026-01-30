# Contributing to GLASS DROP

Thanks for your interest in improving GLASS DROP 🍺

## How to add a new Game Mode

1. Fork this repository
2. Create a new branch:


3. Add your mode:
- Extend the `GameMode` enum
- Implement logic inside the main `loop()` state machine
4. Keep changes focused:
- Do NOT modify `updateGlassState()`
- Do NOT change hardware pin assignments
5. Open a Pull Request and describe:
- What your mode does
- How it is started
- How it ends

## Code Style
- Use clear variable names
- Comment new logic
- Avoid long blocking delays

## Review
All contributions will be reviewed before merging.
