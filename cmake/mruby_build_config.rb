# AetherRPG – minimal mruby build configuration (used by rake in CI/local).
# Keep gems lean for faster CI and smaller binaries.

MRuby::Build.new do |conf|
  conf.toolchain

  # Core language + essentials for game scripts
  conf.gembox 'default'

  # Bewusst KEIN enable_cxx_exception: die Engine bindet mruby als C-Lib;
  # C++-Exceptions im VM-Build machen den Build komplexer ohne Nutzen.
  conf.cc.flags << '-fPIC' unless conf.cc.flags.any? { |f| f.include?('fPIC') }
  conf.cxx.flags << '-fPIC' unless conf.cxx.flags.any? { |f| f.include?('fPIC') }

  # Quiet-ish
  conf.cc.defines << 'MRB_NO_BOXING' if ENV['AETHER_MRUBY_NO_BOXING']
end
