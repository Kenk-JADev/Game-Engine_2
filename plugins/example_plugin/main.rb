# frozen_string_literal: true
# ExamplePlugin – demonstriert den Plugin-Einstiegspunkt.

module ExamplePlugin
  module_function

  def on_load
    # Wird von der Engine nach dem Laden des Plugins aufgerufen.
    # puts "[ExamplePlugin] loaded"
  end

  def on_unload
    # Cleanup
  end
end

ExamplePlugin.on_load
