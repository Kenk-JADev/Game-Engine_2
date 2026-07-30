# CI & mruby

## GitHub Actions

Siehe [docs/dev/CI.md](../dev/CI.md) und:

- `.github/workflows/ci.yml`
- `.github/workflows/release.yml`

## mruby-Integration

| CMake-Option | Default | Wirkung |
|--------------|---------|---------|
| `AETHER_WITH_MRUBY` | OFF | ON in CI-Linux-Full-Job |

Voraussetzung: Host-`ruby` (+ idealerweise `rake`/`bison`).

Build-Kette:

```
aether_require_mruby()
  → third_party/mruby-src oder FetchContent mruby 3.3.0
  → ExternalProject rake (MRUBY_CONFIG=cmake/mruby_build_config.rb)
  → libmruby.a
  → engine/src/ruby/mruby_vm.cpp
  → AETHER_WITH_MRUBY=1
```

Runtime:

```cpp
auto vm = ruby::RubyVM::create(); // MRuby wenn gelinkt, sonst Stub
vm->define_engine_api();
vm->load_file("scripts/main.rb");
```

Unit-Tests nutzen weiterhin explizit `RubyBackend::Stub` für deterministische Mini-Eval-Tests.
