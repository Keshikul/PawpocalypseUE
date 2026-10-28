# Handoff log

Newest first. Every session that changes the state of this project adds an entry: where the project is, what happened, what is open,
and the exact next step. Read `docs/00-START-HERE.md` first, then the newest entry here.

---

## 2026-10-28 - the port is planned; nothing is built yet

### Where the project is
- `C:\Games\pawpocalypseUE\Pawpocalypse`: a **blank UE 5.8.3 C++ project** (`5.8.3-58210709`) created by the designer, plus the
  complete port handoff. Git repo on `master` (LFS configured), pushed to **`https://github.com/Keshikul/PawpocalypseUE`**
  (its own repo, separate from the Godot one: designer, 2026-10-28).
- **No engine settings changed yet.** The template still has Lumen, ray tracing, Substrate, VSM and distance fields ON and the
  OpenWorld startup map: phase 0 step 3 fixes that (docs/03 §7). **Do not build anything on the template defaults.**
- The Godot game (`C:\Games\pawpocalypse`, GitHub `JanePonomareva/pawpocalypse`, itch 0.2.5) is the specification and keeps
  shipping until parity (docs/01 D12).

### What happened this session
1. Read the whole knowledge base (`C:\Games\Knowledge`: Unreal 00-26, Steam, PlayStation, Godot 03-05) and the Godot project's
   design and technical docs (`pawpocalypse.md`, GDD v9, technical design, outstanding, handoff headings, data schemas).
2. Web research on 2D in Unreal and on 2D optimisation (sources: docs/13). Key findings: Paper2D gets little development; one
   Actor per sprite does not scale; the tonemapper and auto exposure shift flat colours; translucency sorts per component, never
   inside an instanced component; masked edges need MSAA + alpha-to-coverage or temporal AA; texture streaming blurs sprites;
   no production SVG import; software cursors lag; idle games need their own unfocused throttle; Valve suggests 1 MiB pak padding
   alignment for SteamPipe deltas.
3. Wrote the handoff: docs 00-13, `CLAUDE.md`, and three generated appendices.
4. Exported every Godot `/data` resource to `Data/*.csv` (266 resources, 11 classes, enums by name) with
   `tools/export_godot_data.py`; listed the 456 constants living in Godot code (`tools/list_godot_code_constants.py`, appendix C);
   mapped all 79 Godot scripts (29,258 lines) to their Unreal destinations (`tools/make_port_map.py`, appendix B).
5. Designer, mid-session: **"No lumen, no raytracing"** (recorded in CLAUDE.md, docs/03 §7, docs/04); asked for 2D optimisation
   research and for the knowledge base to be split by dimension.
6. Knowledge base: new `2D/` folder (`01-2d-optimization-best-practices.md`, `02-unreal-2d.md`), `3D/` (an index into the
   3D-specific Unreal docs, which were NOT moved because the global CLAUDE.md links them by path) and `VR/` (empty placeholder);
   README and INDEX updated. The knowledge folder is not a git repository.

### Open: questions for the designer (docs/01 §3)
Q1 what the move must deliver (consoles? Deck? look?); Q2 freeze design until parity (recommended yes); Q3 import itch saves
(recommended yes); Q4 Next Fest Feb 2027 demo from Godot (recommended); Q5 2x rasters for 4K (recommended); Q6 exact Godot colours
first (recommended); Q7 music licence unchanged; Q8 Boba unchanged. **None blocks phase 0.**

### Open: decisions the spikes must make (docs/04 §8, record in docs/04 §10 and Knowledge/2D/02)
S1 anti-aliasing method and exposure value (colour readback within +/-2); S2 renderer (instanced quads vs Paper2D vs Slate canvas);
S3 raster scale, compression, atlas size, OGG import; S4 dock and skill tree widget structure on a pad.

### Next step
Open a session in `C:\Games\pawpocalypseUE\Pawpocalypse` and run:
*"Read docs/00-START-HERE.md and docs/HANDOFF-LOG.md, then do phase 0 steps 1-4 of docs/11-roadmap.md. Do not start the spikes."*
Phase 0 step 2 (git) is already done. Push only when the designer asks in that message.
