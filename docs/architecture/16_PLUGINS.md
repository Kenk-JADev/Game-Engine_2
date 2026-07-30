# Modul 16 – Plugins

**Namespace:** `aether::plugin`  
**Status:** Scanner + Loader implementiert

```
plugins/my_plugin/
  plugin.json
  main.rb
  assets/
```

```json
{
  "name": "MyPlugin",
  "version": "1.0.0",
  "entry": "main.rb",
  "api_version": 1,
  "dependencies": []
}
```

`PluginLoader::scan` → `activate_all(RubyVM)`

---

*Dependency-Auflösung und Sandboxing folgen.*
