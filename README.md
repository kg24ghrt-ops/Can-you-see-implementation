# Graphics Engine Core - F* -> C

Core logic only for Android. No NDK calls, no GPU calls in core.

## Flow
F* Low* (src/*.fst) -> KaRaMeL -> C11 (out/) -> Clang -O2 -> C tests (tests/c/)

## You do NOT install compilers locally
Push a module, GitHub Actions installs F*, KaRaMeL, Z3, Clang and verifies.

## Kilo Code workflow
1. Kilo finishes ONE module in src/
2. Kilo pushes to repo
3. You paste CI errors back to Kilo
4. Kilo fixes and re-pushes
5. Repeat for next module

## Layout
- src/ - F* Low* modules, one per file, max 300 lines
- tests/c/ - C tests that call generated C
- out/ - generated C, DO NOT edit by hand, generated in CI
- .github/workflows/fstar-ci.yml - CI
- .kilo/rules/fstar-graphics-core.md - Kilo rule

## First modules (in order)
1. Vec3
2. Mat4
3. Quaternion
4. Transform
5. Culling (AABB, Frustum)
6. Animation (Lerp, Slerp)
# Can-you-see-implementation
