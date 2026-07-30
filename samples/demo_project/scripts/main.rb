# frozen_string_literal: true
# Aether Demo – entry script

module Main
  module_function

  def boot
    $demo_booted = "yes"
    # Graphics.frame_rate = 60
    # Audio.bgm_play("Theme1", 80, 100)
    # Weather.set("none", 0)
  end
end

Main.boot
