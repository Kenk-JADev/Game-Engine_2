# frozen_string_literal: true
# Aether Demo – entry script

module Main
  module_function

  def boot
    $demo_booted = "yes"
    # Audio.bgm_play("Theme1", 80, 100)
  end
end

Main.boot
