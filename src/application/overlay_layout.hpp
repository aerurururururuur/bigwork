#pragma once

#include "application/render_snapshot.hpp"

namespace application {

/** Fills title overlay copy and absolute grid hit box for the Start control. */
void prepareTitleOverlay(OverlayModel& overlay, int total_cols, int total_rows);

/** End-of-run panel; `victory` selects copy. */
void prepareBattleEndOverlay(OverlayModel& overlay, int total_cols, int total_rows, bool victory);

bool overlayStartButtonHit(const OverlayModel& overlay, int grid_col, int grid_row);

} // namespace application
