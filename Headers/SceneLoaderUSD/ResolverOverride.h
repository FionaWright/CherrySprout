#ifndef H_RESOLVER_OVERRIDE_H
#define H_RESOLVER_OVERRIDE_H

#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/ar/defaultResolver.h>
#include <pxr/base/tf/token.h>

#include <filesystem>

namespace fs = std::filesystem;

class DdsFallbackResolver : public pxr::ArDefaultResolver
{
public:
    DdsFallbackResolver() = default;
    ~DdsFallbackResolver() override = default;

protected:

    // [[nodiscard]] pxr::ArResolvedPath _Resolve(const pxr::ArIdentifier assetPath) const override
    // {
    //     // ask USD for canonical resolution first
    //     std::string resolved = pxr::ArDefaultResolver::Resolve(assetPath);
    //     if (!resolved.empty())
    //         return pxr::ArResolvedPath(resolved);
    //
    //     fs::path p(assetPath);
    //
    //     if (!p.extension().empty())
    //     {
    //         p.replace_extension(".dds");
    //
    //         resolved = pxr::ArDefaultResolver::Resolve(p.string());
    //         if (!resolved.empty())
    //             return pxr::ArResolvedPath(resolved);
    //     }
    //
    //     return {};
    // }

    [[nodiscard]] pxr::ArResolvedPath _Resolve(const std::string& assetPath) const override
    {
        // ask USD for canonical resolution first
        std::string resolved = pxr::ArDefaultResolver::Resolve(assetPath);
        if (!resolved.empty())
            return pxr::ArResolvedPath(resolved);

        fs::path p(assetPath);

        if (!p.extension().empty())
        {
            p.replace_extension(".dds");

            resolved = pxr::ArDefaultResolver::Resolve(p.string());
            if (!resolved.empty())
                return pxr::ArResolvedPath(resolved);
        }

        return {};
    }

private:

    static pxr::ArResolvedPath TryDefaultResolve(const std::string& path)
    {
        // IMPORTANT: call base resolver logic
        return pxr::ArGetResolver().Resolve(path);
    }
};

#include <pxr/usd/ar/defineResolver.h>

PXR_NAMESPACE_OPEN_SCOPE

AR_DEFINE_RESOLVER(DdsFallbackResolver, pxr::ArDefaultResolver);

PXR_NAMESPACE_CLOSE_SCOPE

#endif