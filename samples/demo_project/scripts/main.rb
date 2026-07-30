# frozen_string_literal: true
# Aether Demo – Hauptskript

module Main
  module_function

  def boot
    $demo_booted = "yes"
    Graphics.frame_rate = 60 if defined?(Graphics)
    # Audio.bgm_play("Theme1", 80, 100)
    # Weather.set("none", 0)
  end
end

Main.boot
