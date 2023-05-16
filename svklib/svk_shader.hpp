#ifndef SVKLIB_SHADER_HPP
#define SVKLIB_SHADER_HPP

#include <glslang/Public/ShaderLang.h>

namespace svklib {

class shader {
public:
    shader(const char* path, EShLanguage stage);
    // shader(std::initializer_list<const char*> paths, EShLanguage stage); //todo figure out why this doesn't work
    ~shader();

    shader(const shader&) = delete;
    shader& operator=(const shader&) = delete;

    static void setShaderVersion(uint32_t apiVersion);

    std::vector<uint32_t> getSpirvCode();
    VkShaderModule createShaderModule(VkDevice device);

private:
    void compileShader(EShLanguage stage, const char* path, const char* cSpvPath);
    const std::string getSourceCode(const char* path);
    void loadSpvCode(const char* path);
    void saveSpvCode(const char* path);
    bool ends_with(const char* str, const char* suffix);

    std::vector<uint32_t> code;
};

} // namespace svklib

#endif // SVKLIB_SHADER_HPP
