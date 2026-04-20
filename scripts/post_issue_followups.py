#!/usr/bin/env python3
"""Post English follow-up comments on ScanTailor-Advanced issues (one-shot helper)."""
from __future__ import annotations

import subprocess
import sys

REPO = "ScanTailor-Advanced/scantailor-advanced"

# (issue_number, body) — varied tone per plan; no false claims about unrelated PRs.
COMMENTS: list[tuple[int, str]] = [
    (
        61,
        """**Follow-up from maintainers**

`imageproc_tests` failing in a **clean offline chroot** is still an open packaging/reproducibility concern. Nothing in the general application merge PRs replaces a proper fix here.

**To move forward, please reply with:**
- Full **`ctest` / `imageproc_tests` output** from the failing build
- **Build environment** (distro, chroot tool, whether the network is completely blocked)
- Whether **`BUILD_TESTS=OFF`** is an acceptable workaround for your packagers in the meantime

Once we can reproduce the exact failure mode, we can target missing fixtures vs accidental network fetches in [`src/imageproc/tests`](https://github.com/ScanTailor-Advanced/scantailor-advanced/tree/master/src/imageproc/tests).

Thank you.""",
    ),
    (
        62,
        """We're tracking the **right-to-left / Japanese book** ordering pain for **Split page** and page numbering.

Before anyone implements a fix, it would help to lock the **desired UX**:

1. **Thumbnail order only** — reverse how pages appear in the strip while keeping internal processing as today?  
2. **Split logic only** — keep thumbnails as now but change how split assigns left/right or page indices?  
3. **Both**

Please reply with **1 / 2 / 3** (or a short alternative). Screenshots or a tiny sample project name pattern also help.

This issue stays open until we have that agreement and a concrete patch.""",
    ),
    (
        67,
        """**Status check:** selecting thumbnails with a **rubber-band / frame while holding Ctrl** is still reported as missing or broken.

If you still see this on a **current `master` build**, please comment with:

- **OS + Qt version** (Qt5 vs Qt6 if you know)
- **Exact steps** (e.g. Ctrl+drag on thumbnail strip vs main view)
- Whether it **ever worked** in a specific older version you remember

That will tell us whether the regression is in [`ThumbnailSequence`](https://github.com/ScanTailor-Advanced/scantailor-advanced/tree/master/src/app) / view code or elsewhere.

Thanks.""",
    ),
    (
        81,
        """**Feature request note:** “Freeze auto dewarping” (stop automatic re-adjustment while you work) is still **not implemented** as a dedicated option — it remains a **backlog enhancement**.

If this still matters for your workflow, describe **when** you want processing to pause (per page, per project, only in Output, etc.). Community patches are welcome; the dewarp pipeline lives under [`src/core/filters/output`](https://github.com/ScanTailor-Advanced/scantailor-advanced/tree/master/src/core/filters/output) and related code.

Issue stays open as a feature tracker.""",
    ),
    (
        82,
        """**Output view guides** (rulers / alignment lines) are still a **feature request** — not shipped yet.

If you have a concrete use case (print margins, bleed, centering text blocks), add a short comment; that helps if someone picks up UI work in [`page_layout` / `output`](https://github.com/ScanTailor-Advanced/scantailor-advanced/tree/master/src/core/filters) views.

Leaving this open for tracking.""",
    ),
    (
        85,
        """You asked to **split or refine** the behaviour of **“Match size with other pages”** in alignment / page layout.

This is still **open enhancement** work: no small default change has been agreed yet.

Please add **which part** should be optional (width only, height only, linked to specific page, etc.) if you can — it reduces ambiguity for a future patch in the page layout filter.

Thanks.""",
    ),
    (
        86,
        """**“Pause sorting pages”** (stop automatic re-ordering while arranging thumbnails) remains a **feature request**.

If you still need it, say whether you mean **pausing deviation-based auto-sort**, **manual drag reorder**, or something else — that clarifies where hooks would live ([`MainWindow`](https://github.com/ScanTailor-Advanced/scantailor-advanced/tree/master/src/app) / thumbnail sequence).

Open until implemented or explicitly out of scope.""",
    ),
    (
        91,
        """Extra **offcut / split guides** (top-bottom / left-right variants) are still **under consideration** as a **feature request**.

No merged change specifically tracks this thread yet. If you can attach a **sketch or scan example**, it helps prioritize against existing split modes.

Leaving open.""",
    ),
    (
        92,
        """**Fill outside page box** (lighter alternative to #91) is still an **open feature request**.

Contributions need a clear definition of **which rectangle is “outside”** once margins and split vary per page — comment if you have a precise rule.

Stays open for design + implementation.""",
    ),
    (
        93,
        """**“Fix DPI” exposed in the Margins panel** is still a **feature request** — not a committed roadmap item from this thread alone.

If you still want it, note whether DPI should follow **Output**, **Select Content**, or **global project** semantics; that affects where settings live.

Open for tracking.""",
    ),
    (
        95,
        """**Guidelines on the Margins view** (similar to other stages) remain a **feature request**.

If you need horizontal **and** vertical guides, or snapping, mention it — UI work would touch margins / page layout views.

Issue stays open.""",
    ),
    (
        96,
        """**Working examples / sample projects** come up often.

Practical pointers today:
- Build from [`README`](https://github.com/ScanTailor-Advanced/scantailor-advanced/blob/master/README.md) or use **Releases** / CI artifacts when available ([#64](https://github.com/ScanTailor-Advanced/scantailor-advanced/issues/64)).
- For **Flatpak**, see [#105](https://github.com/ScanTailor-Advanced/scantailor-advanced/issues/105) and the in-repo manifest under `flatpak/`.

If something **specific** still fails (crash, empty window), open a **bug** with version + OS rather than this umbrella thread.

Keeping this open as **question** until docs improve.""",
    ),
    (
        101,
        """**Windows 7** is legacy: current toolchains and Qt versions increasingly drop support.

We are **not planning** dedicated Win7 CI or guarantees. If you must stay on Win7, you may need an **older tagged release** and accepted security risk.

This issue can stay open as **documentation / expectation** or be closed as **wontfix** if maintainers prefer — feedback welcome.""",
    ),
    (
        103,
        """**Output image pipe** (stream each rendered page to an external command instead of only writing files) is a **significant feature**: process model, errors, DPI, temp files, and UI all need design.

Still **open**. If you want to champion it, a short **design sketch** (one shell command line, how failures surface) would kick off discussion.

No ETA from this comment alone.""",
    ),
    (
        104,
        """Bundling **multiple “scantailor-experimental” ideas** plus misc optimisations is a **meta feature bucket** — too broad for a single patch.

Suggest **splitting** into separate issues per idea (with links to forks like scantailor-experimental where relevant). That makes review possible.

Leaving open but **please narrow** future comments to one topic at a time.""",
    ),
    (
        1,
        """**Fill tool / magic wand** in zone or picture editing is a long-standing **enhancement** — not implemented in this fork from this ticket alone.

If you still want it, specify **which stage** (Output fill zones, picture zones, etc.). Patches welcome; zone editors live under [`src/core/filters/output`](https://github.com/ScanTailor-Advanced/scantailor-advanced/tree/master/src/core/filters/output).

Open as feature request.""",
    ),
    (
        2,
        """**Extend selection in Select Content in one direction** (keyboard or handles) remains a **feature request**.

Good first step for a contributor: prototype in [`select_content`](https://github.com/ScanTailor-Advanced/scantailor-advanced/tree/master/src/core/filters/select_content) with clear key bindings.

Stays open.""",
    ),
    (
        4,
        """**Automatic page-number detection** to drive Select Content is a **large research + UX** item — still open, no committed implementation tracked here.

If you narrow the scope (e.g. footer-only Arabic numerals), reply with constraints; that could make a future PR feasible.

Leaving open.""",
    ),
    (
        5,
        """**Export to indexed PNG with transparency** is an **output format enhancement** — still open.

Implementation touches TIFF/PNG writers and Output pipeline; contributors should check existing bit depth / alpha paths.

Feature stays on the backlog.""",
    ),
    (
        10,
        """**Crop / trim without re-encoding** the image is a **feature request** (lossless crop in pipeline sense) — still not delivered from this issue alone.

Clarify whether you mean **physical crop of source files** or **logical crop** in ST’s page box only.

Open for discussion.""",
    ),
    (
        12,
        """**Multiple threshold regions on the foreground layer** remains a **major enhancement** — open.

Would require substantial Output / zone model work. Splitting into smaller milestones (per-zone threshold already vs new UI) would help.

Thanks for the suggestion — keeping the issue.""",
    ),
    (
        13,
        """**“Type area”** (text block definition) is a **broad feature** — still open without a spec tied to this thread.

If you can reference a **paper or competitor** behaviour, it helps scope.

Backlog.""",
    ),
    (
        15,
        """**Default zone creation mouse mode** (behaviour when starting a new zone) is a **UX enhancement** — still open.

Please say which mode you want as default (rectangle, freehand, etc.) and in **which editor**.

Tracking.""",
    ),
    (
        21,
        """**Persistent zoom** across page changes / sessions is a **feature request** — not implemented from this ticket alone.

Note whether you mean **per-filter zoom** or **global UI zoom** — storage might use `ApplicationSettings`.

Open.""",
    ),
    (
        22,
        """**“Beep when finished” on an alternate audio device/channel** is **still open** as a small enhancement.

Platform-specific audio routing may differ on Linux/Windows/macOS; a contributor can prototype behind a setting.

Leaving open.""",
    ),
    (
        25,
        """The **“original alignment”** request is light on detail in the title alone.

Could you add a **short comment** describing the workflow you miss (which stage, which control)? That turns this into something actionable.

Issue stays open pending clarification.""",
    ),
    (
        26,
        """**Separate checkboxes for applying left/right/top/bottom margins** (instead of all-or-nothing) is a **feature request** — still open.

Implementation would touch margin apply dialogs and batch application logic in [`page_layout`](https://github.com/ScanTailor-Advanced/scantailor-advanced/tree/master/src/core/filters/page_layout).

Tracking.""",
    ),
    (
        28,
        """**Perspective correction** (GIMP-style) is a **large feature** — still open; related requests exist on other forks.

No ETA here. If you start a prototype, link the branch early for feedback.

Keeping open as the main tracking issue for this repo.""",
    ),
    (
        42,
        """**Project website** hosting is **outside** the core codebase — this issue stays as a **meta / organisational** request.

Release and install pointers: see **README**, **Releases**, [#64](https://github.com/ScanTailor-Advanced/scantailor-advanced/issues/64), [#105](https://github.com/ScanTailor-Advanced/scantailor-advanced/issues/105).

Maintainers may close or repurpose this if a domain/plan exists.""",
    ),
    (
        43,
        """**Examples / tutorials** are still mostly community-driven.

See also [#96](https://github.com/ScanTailor-Advanced/scantailor-advanced/issues/96) and the **README** build section. Contributions (docs, sample projects) welcome via PR.

Open as documentation request.""",
    ),
    (
        49,
        """**RIS / bibliographic export** integration is a **niche enhancement** — still open without implementation attached.

If you specify **which tool** should consume RIS and **what fields** map from ST projects, a future PR becomes imaginable.

Backlog.""",
    ),
    (
        60,
        """**Explicit page size** control (beyond what margins/output already do) is a **feature request** — open.

Clarify whether you need **physical mm sizes**, **forced output aspect**, or **template presets**.

Tracking.""",
    ),
    (
        72,
        """**Disabling the content box** entirely for certain workflows is a **feature request** — still open.

Describe the **pipeline you want** (content disabled but page box used, etc.) so implementers know which params to bypass in [`select_content`](https://github.com/ScanTailor-Advanced/scantailor-advanced/tree/master/src/core/filters/select_content).

Open.""",
    ),
    (
        73,
        """**Scan Tailor “Experimental”** features are a **wide umbrella** — this issue stays open as **discussion + pointer** to forks (e.g. scantailor-experimental, other community trees).

There is **no single merge plan** from this thread. Please open **focused issues** for individual features you need from Experimental.

Thanks for keeping the links alive.""",
    ),
    (
        74,
        """**Alternate content-box centering** (different from current auto rules) is an **enhancement** — still open.

If you can describe **one algorithm** (e.g. center in page box vs center in detected text block), it helps scope.

Tracking.""",
    ),
    (
        27,
        """**Meta: releases, binaries, CI, Flatpak**

Several threads overlap with “what ships next and where to download”:

- **GitHub Releases / Linux `.deb` + AppImage:** [#64](https://github.com/ScanTailor-Advanced/scantailor-advanced/issues/64) — tag-driven [`release.yml`](https://github.com/ScanTailor-Advanced/scantailor-advanced/blob/master/.github/workflows/release.yml) attaches assets when a `v*` tag is pushed; verification still needs **real tagged releases**.
- **CI coverage (Windows/macOS):** [#37](https://github.com/ScanTailor-Advanced/scantailor-advanced/issues/37) — today’s PR builds are **Linux-centric**; Win/Mac matrix jobs are still wanted.
- **Flatpak / Flathub under a new app ID:** [#105](https://github.com/ScanTailor-Advanced/scantailor-advanced/issues/105) — local manifest under `flatpak/`; Flathub listing needs a **separate submission**, not replacing the old `com.github._4lex4.*` app.

Please continue **detailed** discussion in those linked issues so each stays actionable. This comment is only a **router**.""",
    ),
]


def main() -> int:
    dry = "--dry-run" in sys.argv
    for num, body in COMMENTS:
        cmd = [
            "gh",
            "issue",
            "comment",
            str(num),
            "--repo",
            REPO,
            "--body",
            body,
        ]
        if dry:
            print(f"DRY #{num} ({len(body)} chars)")
            continue
        print(f"Posting #{num} ...")
        subprocess.run(cmd, check=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
