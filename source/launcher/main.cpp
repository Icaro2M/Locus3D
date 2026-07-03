// SPDX-FileCopyrightText: 2026 Icaro2M
// SPDX-License-Identifier: Apache-2.0

#include "editor/history/HistoryConfig.h"
#include "editor/history/HistoryStack.h"

#include <iostream>
#include <string>

namespace {

    using namespace locus::editor;

    int g_failures = 0;

    void check(bool condition, const std::string& message)
    {
        if (condition) {
            std::cout << "[OK] " << message << '\n';
            return;
        }

        std::cout << "[FAIL] " << message << '\n';
        ++g_failures;
    }

    void print_config(const std::string& label, const HistoryConfig& config)
    {
        std::cout << label << '\n';
        std::cout << "  maxEntries: " << config.maxEntries << '\n';
        std::cout << "  enabled: " << (config.enabled ? "true" : "false") << '\n';
    }

    void test_history_config_constants()
    {
        std::cout << "\n=== HistoryConfig constants ===\n";

        check(
            UnlimitedHistoryEntries == 0u,
            "UnlimitedHistoryEntries e 0");

        check(
            MinHistoryMaxEntries == 1u,
            "MinHistoryMaxEntries e 1");

        check(
            DefaultHistoryMaxEntries == 128u,
            "DefaultHistoryMaxEntries e 128");

        check(
            MaxHistoryMaxEntries == 4096u,
            "MaxHistoryMaxEntries e 4096");

        check(
            DefaultHistoryMaxEntries >= MinHistoryMaxEntries,
            "DefaultHistoryMaxEntries respeita o minimo");

        check(
            DefaultHistoryMaxEntries <= MaxHistoryMaxEntries,
            "DefaultHistoryMaxEntries respeita o maximo");

        check(
            MaxHistoryMaxEntries >= MinHistoryMaxEntries,
            "MaxHistoryMaxEntries respeita o minimo");
    }

    void test_normalize_history_max_entries()
    {
        std::cout << "\n=== normalize_history_max_entries ===\n";

        check(
            normalize_history_max_entries(UnlimitedHistoryEntries) == UnlimitedHistoryEntries,
            "normalize preserva UnlimitedHistoryEntries");

        check(
            normalize_history_max_entries(0u) == UnlimitedHistoryEntries,
            "normalize preserva 0 como historico ilimitado");

        check(
            normalize_history_max_entries(MinHistoryMaxEntries) == MinHistoryMaxEntries,
            "normalize preserva valor minimo");

        check(
            normalize_history_max_entries(DefaultHistoryMaxEntries) == DefaultHistoryMaxEntries,
            "normalize preserva valor padrao");

        check(
            normalize_history_max_entries(MaxHistoryMaxEntries) == MaxHistoryMaxEntries,
            "normalize preserva valor maximo");

        check(
            normalize_history_max_entries(MaxHistoryMaxEntries + 1u) == MaxHistoryMaxEntries,
            "normalize limita valor acima do maximo");

        check(
            normalize_history_max_entries(MaxHistoryMaxEntries + 1000u) == MaxHistoryMaxEntries,
            "normalize limita valor muito acima do maximo");
    }

    void test_normalize_history_config()
    {
        std::cout << "\n=== normalize_history_config ===\n";

        {
            HistoryConfig config{};
            print_config("config padrao antes", config);

            const HistoryConfig normalized = normalize_history_config(config);
            print_config("config padrao depois", normalized);

            check(
                normalized.maxEntries == DefaultHistoryMaxEntries,
                "config padrao preserva maxEntries");

            check(
                normalized.enabled,
                "config padrao preserva enabled true");
        }

        {
            HistoryConfig config{};
            config.maxEntries = UnlimitedHistoryEntries;
            config.enabled = true;

            print_config("config ilimitada antes", config);

            const HistoryConfig normalized = normalize_history_config(config);
            print_config("config ilimitada depois", normalized);

            check(
                normalized.maxEntries == UnlimitedHistoryEntries,
                "config ilimitada preserva maxEntries 0");

            check(
                normalized.enabled,
                "config ilimitada preserva enabled true");
        }

        {
            HistoryConfig config{};
            config.maxEntries = MaxHistoryMaxEntries + 250u;
            config.enabled = false;

            print_config("config acima do maximo antes", config);

            const HistoryConfig normalized = normalize_history_config(config);
            print_config("config acima do maximo depois", normalized);

            check(
                normalized.maxEntries == MaxHistoryMaxEntries,
                "config acima do maximo foi limitada");

            check(
                !normalized.enabled,
                "config acima do maximo preserva enabled false");
        }
    }

    void test_history_stack_with_config()
    {
        std::cout << "\n=== HistoryStack usando HistoryConfig ===\n";

        {
            HistoryStack history;

            check(
                history.max_entries() == UnlimitedHistoryEntries,
                "HistoryStack comeca ilimitado por padrao");

            HistoryConfig config{};
            config.maxEntries = DefaultHistoryMaxEntries;

            const HistoryConfig normalized = normalize_history_config(config);
            history.set_max_entries(normalized.maxEntries);

            check(
                history.max_entries() == DefaultHistoryMaxEntries,
                "HistoryStack recebeu DefaultHistoryMaxEntries");
        }

        {
            HistoryStack history;

            HistoryConfig config{};
            config.maxEntries = UnlimitedHistoryEntries;

            const HistoryConfig normalized = normalize_history_config(config);
            history.set_max_entries(normalized.maxEntries);

            check(
                history.max_entries() == UnlimitedHistoryEntries,
                "HistoryStack aceitou historico ilimitado pelo config");
        }

        {
            HistoryStack history;

            HistoryConfig config{};
            config.maxEntries = MaxHistoryMaxEntries + 500u;

            const HistoryConfig normalized = normalize_history_config(config);
            history.set_max_entries(normalized.maxEntries);

            check(
                history.max_entries() == MaxHistoryMaxEntries,
                "HistoryStack recebeu valor normalizado para o maximo");
        }

        {
            HistoryStack history;

            HistoryConfig config{};
            config.maxEntries = 16u;
            config.enabled = false;

            const HistoryConfig normalized = normalize_history_config(config);
            history.set_max_entries(normalized.maxEntries);

            check(
                history.max_entries() == 16u,
                "HistoryStack recebeu valor customizado valido");

            check(
                !normalized.enabled,
                "enabled false continua preservado no config normalizado");

            check(
                history.empty(),
                "HistoryStack continua vazio apos aplicar config");
        }
    }

} // namespace

int main()
{
    std::cout << "=== Locus3D Editor HistoryConfig Smoke Test ===\n";

    test_history_config_constants();
    test_normalize_history_max_entries();
    test_normalize_history_config();
    test_history_stack_with_config();

    std::cout << "\n=== Resultado Final ===\n";

    if (g_failures == 0) {
        std::cout << "Todos os testes passaram.\n";
        return 0;
    }

    std::cout << g_failures << " teste(s) falharam.\n";
    return 1;
}