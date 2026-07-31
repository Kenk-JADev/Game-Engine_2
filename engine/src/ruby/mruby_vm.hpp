/**
 * @file mruby_vm.hpp
 * @brief Echte mruby-Einbettung (AETHER_WITH_MRUBY).
 */
#pragma once

#include <aether/ruby/ruby_vm.hpp>

#if defined(AETHER_WITH_MRUBY)

struct mrb_state;

namespace aether::ruby {

class MRubyVM final : public RubyVM {
public:
    MRubyVM();
    ~MRubyVM() override;

    void define_engine_api() override;
    EvalResult eval(std::string_view source, std::string_view filename) override;
    EvalResult load_file(std::string_view path) override;
    void set_global(std::string_view name, std::string_view value) override;
    std::string get_global(std::string_view name) const override;

    /** @brief Intern: Host-Dispatch aus mruby-Callbacks. */
    EvalResult dispatch_host(std::string_view module, std::string_view name,
                             const std::vector<std::string>& args);

private:
    void ensure_host_registered();
    void register_host_as_ruby();

    mrb_state* mrb_ = nullptr;
    bool host_registered_ = false;
};

} // namespace aether::ruby

#endif
