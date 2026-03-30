/*
    PluginCollider Copyright (c) 2025-2026 Pascal Gauthier.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <map>

/** Ordered list of (paramName, "min max step") preserving the author's intended order. */
using SpecList = std::vector<std::pair<juce::String, juce::String>>;

/** Look up a spec range by param name. Returns empty string if not found. */
static inline juce::String findSpec(const SpecList &specs, const juce::String &name) {
    for (const auto &s : specs)
        if (s.first == name) return s.second;
    return {};
}

/**
 * Load parameter specs from a .spec or .txarcmeta file alongside a .scsyndef.
 * Returns an ordered list of (paramName, "min max step") — order reflects
 * the author's intended control bus assignment order.
 */
static SpecList loadSpecFile(const juce::File &scsyndef) {
    SpecList specs;

    // Try .txarcmeta (SC's metadata archive format)
    auto metaFile = scsyndef.withFileExtension("txarcmeta");
    if (!metaFile.existsAsFile()) {
        // Fall back to .spec (simple text format) — line order is preserved
        auto specFile = scsyndef.withFileExtension("spec");
        if (!specFile.existsAsFile())
            return specs;
        auto lines = juce::StringArray::fromLines(specFile.loadFileAsString());
        for (auto &line : lines) {
            line = line.trim();
            if (line.isEmpty() || line.startsWith("#"))
                continue;
            auto tokens = juce::StringArray::fromTokens(line, " \t", "");
            if (tokens.size() >= 4)
                specs.emplace_back(tokens[0], tokens[1] + " " + tokens[2] + " " + tokens[3]);
        }
        return specs;
    }

    // Parse .txarcmeta: extract ControlSpec minval/maxval/step keyed by param name
    auto content = metaFile.loadFileAsString();

    // 1. Find param name -> object index mapping from the array line like:
    //    'stutter',  o[4],  'div',  o[7],  ...  'ffreq',  o[9],
    //    Preserve insertion order — this is the author's .metadata_ order.
    std::vector<std::pair<juce::String, int>> orderedNames;
    std::map<int, juce::String> indexToName;
    int searchPos = 0;
    while (true) {
        int quoteStart = content.indexOf(searchPos, "'");
        if (quoteStart < 0) break;
        int quoteEnd = content.indexOf(quoteStart + 1, "'");
        if (quoteEnd < 0) break;
        auto paramName = content.substring(quoteStart + 1, quoteEnd);
        // Look for o[N] after the param name
        int oRef = content.indexOf(quoteEnd, "o[");
        if (oRef < 0) break;
        // Make sure we don't jump past the next param name
        int nextQuote = content.indexOf(quoteEnd + 1, "'");
        if (nextQuote >= 0 && oRef > nextQuote) {
            searchPos = quoteEnd + 1;
            continue;
        }
        int oBracketEnd = content.indexOf(oRef, "]");
        if (oBracketEnd < 0) break;
        int objIdx = content.substring(oRef + 2, oBracketEnd).getIntValue();
        if (paramName != "specs" && paramName != "spec") {
            orderedNames.emplace_back(paramName, objIdx);
            indexToName[objIdx] = paramName;
        }
        searchPos = oBracketEnd + 1;
    }

    // 2. Extract ControlSpec entries into a temporary map keyed by object index
    std::map<int, juce::String> specsByIdx;
    searchPos = 0;
    while (true) {
        int csPos = content.indexOf(searchPos, "// ControlSpec");
        if (csPos < 0) break;
        // Find the object index on the next line: "N, ["
        int lineStart = content.indexOf(csPos, "\n") + 1;
        auto idxToken = content.substring(lineStart, content.indexOf(lineStart, ",")).trim();
        int objIdx = idxToken.getIntValue();

        // Extract minval, maxval, step
        int blockStart = content.indexOf(lineStart, "[");
        if (blockStart < 0) break;
        // Find matching ] — skip nested o[N] references
        int blockEnd = blockStart + 1;
        int depth = 1;
        while (blockEnd < content.length() && depth > 0) {
            if (content[blockEnd] == '[') depth++;
            else if (content[blockEnd] == ']') depth--;
            blockEnd++;
        }
        if (depth != 0) break;
        blockEnd--; // point at the ]
        auto block = content.substring(blockStart, blockEnd + 1);

        auto extractValue = [&block](const juce::String &key) -> juce::String {
            int pos = block.indexOf(key + ":");
            if (pos < 0) return "";
            int valStart = pos + key.length() + 1;
            int valEnd = block.indexOf(valStart, ",");
            if (valEnd < 0) valEnd = block.indexOf(valStart, "]");
            return block.substring(valStart, valEnd).trim();
        };

        auto minval = extractValue("minval");
        auto maxval = extractValue("maxval");
        auto step = extractValue("step");

        if (minval.isNotEmpty() && maxval.isNotEmpty() && step.isNotEmpty()) {
            specsByIdx[objIdx] = minval + " " + maxval + " " + step;
        }
        searchPos = blockEnd + 1;
    }

    // 3. Build output in metadata order — only include params that have a ControlSpec
    for (const auto &[name, objIdx] : orderedNames) {
        auto it = specsByIdx.find(objIdx);
        if (it != specsByIdx.end())
            specs.emplace_back(name, it->second);
    }

    return specs;
}
