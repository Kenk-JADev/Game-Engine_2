# DemoPlugin – Milestone 01 / Plugin-API-Beispiel
# Wird von PluginLoader geladen (main.rb)

# Beispiel: Einfache Ausgabe bei Plugin-Aktivierung
puts "[DemoPlugin] Geladen – EventBridge bereit für Spiellogik."

# Optional: Ruby-API-Integration (wenn Engine-Module verfügbar)
# Aether::Event.register("DemoEvent", proc { |id| puts "Event #{id} ausgelöst" })
