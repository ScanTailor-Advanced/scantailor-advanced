// Copyright (C) 2026 ScanTailor-Advanced contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Spike notes for GitHub #116 (TIFF deskew / external deskew integration).
// This header is documentation only; it is not wired into the build.
//
// Integration outline:
// 1. Decide scope: batch-only vs interactive, and whether to shell out to a
//    system tool (e.g. tiffcp/tiffutil) vs link libtiff directly.
// 2. If external binary: locate executable per platform (Windows/macOS/Linux),
//    validate on startup or lazily, surface a clear error in the UI.
// 3. Pipe I/O: write a temporary TIFF, run deskew, read back; or use in-memory
//    API if linking libtiff — mind thread safety with the worker pool.
// 4. Preserve metadata (DPI, photometric) and failure modes (OOM, corrupt TIFF).
// 5. Add an optional CMake feature flag so packagers can disable the dependency.
//
// Prefer keeping experimental code on a branch until packaging and UX are settled.

#ifndef SCANTAILOR_FIX_ORIENTATION_DESKEW_EXTERNAL_TOOL_SPIKE_H_
#define SCANTAILOR_FIX_ORIENTATION_DESKEW_EXTERNAL_TOOL_SPIKE_H_

namespace fix_orientation {
// Intentionally empty: see file comment above.
}  // namespace fix_orientation

#endif
