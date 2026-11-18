# Kilo Rule: C++ Graphics Core - Push After Every Module

## Project truth
- Graphics engine core for Android, core logic ONLY.
- C++20, no Android APIs, no NDK calls, no Vulkan, no OpenGL inside core.
- I do NOT use vision. I describe bugs in text, you fix from text.
- Model: Use Nemotron 3.5 Super VL for main work. It is best for C++.
- CI is source of truth, hosted runners, 2 min.

## Workflow - one module at a time
1. Research + Plan only, wait for me to say "go"
2. Code one module: src/<Module>.h + src/<Module>.cpp, max 300 lines
3. Test: tests/test_<module>.cpp with asserts
4. PUSH AFTER EVERY MODULE:
   ```
   git add src/<Module>.h src/<Module>.cpp tests/test_<module>.cpp
   git commit -m "feat(core): <Module> C++20"
   git push origin HEAD
   ```
5. STOP and wait. Tell me: "Pushed <Module>. Waiting for CI."
6. I paste CI errors, you fix ONLY that module, re-push with fix(core): <Module> <fix>

## Never do
- Never push 2 modules at once.
- Never add Android / NDK / GPU code to src/.
- Never use raw new/delete, use value types.
