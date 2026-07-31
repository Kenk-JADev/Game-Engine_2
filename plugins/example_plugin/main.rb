# frozen_string_literal: true
# ExamplePlugin – demonstriert den Plugin-Einstiegspunkt.

module ExamplePlugin
  def self.on_load
    # Wird von der Engine nach dem Laden des Plugins aufgerufen.
    # puts "[ExamplePlugin] loaded"
  end

  def self.on_unload
    # Cleanup
  end
end

ExamplePlugin.on_load
