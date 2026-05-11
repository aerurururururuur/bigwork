#pragma once

#include "application/render_snapshot.hpp"
#include "domain/ports/iterminal.hpp"

#include <vector>

namespace representation {

/** Builds one full-screen FrameCell buffer from RenderSnapshot (single tick). */
class FrameComposer {
public:
    std::vector<domain::FrameCell> compose(const application::RenderSnapshot& snap) const;
};

} // namespace representation
