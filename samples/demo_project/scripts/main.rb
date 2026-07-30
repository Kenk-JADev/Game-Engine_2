# frozen_string_literal: true
# Aether Demo – Hauptskript

module Main
  module_function

  def boot
    $demo_booted = "yes"
    # BGM startet, sobald Audio-Dateien vorhanden sind
    Audio.bgm_play("Theme1", 70, 100) if defined?(Audio)
  end
end

Main.boot
