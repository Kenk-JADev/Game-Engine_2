# AetherRPG – minimal mruby build configuration (used by rake in CI/local).
# Keep gems lean for faster CI and smaller binaries.

MRuby::Build.new do |conf|
  conf.toolchain

  # Core language + essentials for game scripts
  conf.gembox 'default'

  conf.enable_cxx_exception
  conf.cc.flags << '-fPIC' unless conf.cc.flags.any? { |f| f.include?('fPIC') }
  conf.cxx.flags << '-fPIC' unless conf.cxx.flags.any? { |f| f.include?('fPIC') }

  # Quiet-ish
  conf.cc.defines << 'MRB_NO_BOXING' if ENV['AETHER_MRUBY_NO_BOXING']
end
