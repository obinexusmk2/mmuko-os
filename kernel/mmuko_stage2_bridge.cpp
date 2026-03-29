// Generated file. Do not edit by hand.
// Authoritative input: pseudocode/MMUKO-OS.txt
// Primary pseudocode: pseudocode/mmuko-boot.psc
#include "mmuko_codegen.h"

#include <sstream>
#include <string>
#include <vector>

namespace mmuko::generated {

std::vector<std::string> pseudocode_sources() {
    std::vector<std::string> sources;
    for (size_t index = 0; index < mmuko_pseudocode_source_count(); ++index) {
        sources.emplace_back(mmuko_pseudocode_source(index));
    }
    return sources;
}

std::string stage2_report() {
    std::ostringstream report;
    report << "Authoritative input: pseudocode/MMUKO-OS.txt\n";
    report << "Primary pseudocode: pseudocode/mmuko-boot.psc\n";
    report << "Phase count: " << mmuko_stage2_phase_count() << "\n";

    const auto *phases = mmuko_stage2_phases();
    for (size_t index = 0; index < mmuko_stage2_phase_count(); ++index) {
        report << phases[index].phase_id << " => " << phases[index].title
               << " :: " << phases[index].summary << "\n";
    }
    return report.str();
}

} // namespace mmuko::generated
