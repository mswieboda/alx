---
name: archive-plan-alx
description: Audit plan documents in plans/ against current C++ codebase state in src/, classify implementation status, provide archiving recommendations, and move qualified plans to plans/archive/ preserving relative subpath structure and .rgignore compliance.
---

# Archive Plan Skill (`archive-plan-alx`)

This skill provides a standardized, intelligent workflow for auditing implementation plans, phase specs, and brainstorming documents in `plans/` against actual C++ source files in `src/`. It classifies their completion state, delivers actionable archiving recommendations, and moves qualified files to `plans/archive/` while preserving exact directory hierarchy and workspace hygiene.

---

## 1. Trigger Criteria & Activation

Activate this skill when:
- The user runs `/archive-plan-alx` or requests to archive plan or phase documents.
- A plan document in `plans/` has completed all actionable subtasks (`- [x]`) and is ready for archiving.
- A plan document in `plans/` has become obsolete or superseded by codebase refactoring in `src/`.

---

## 2. Target Plan Resolution Protocol

Identify the candidate plan document(s) using the following precedence:

1. **Explicit File Path**: The user provides a specific file path or pattern (e.g. `plans/PLAN_PHASE_4.md` or `plans/levels/*.md`).
2. **Current Context Plan**: The user requests to "archive this plan" while discussing an active plan in the current conversation.
3. **Scan & Discover Candidates**: If no file is specified, inspect `plans/` (excluding `plans/archive/`) to find plan files and report candidate status to the user.

---

## 3. Pre-Archive Codebase Audit Protocol

Before moving any file, inspect the target plan content and cross-reference it against the actual C++ codebase in `src/`:

1. **Checkbox Audit**: Check for pending (`- [ ]`) vs completed (`- [x]`) items.
2. **Symbol & Logic Audit**: Use `grep_search` (scoped to `src/`) to check if key structs, classes, functions, or features proposed in the plan actually exist in `src/`.
3. **Classification & Recommendation Engine**:

| Category | Codebase Criteria | Skill Recommendation | Execution Action |
| :--- | :--- | :--- | :--- |
| **Category A: Fully Completed** | All subtasks `- [x]` and proposed C++ logic is verified in `src/`. | **Recommend: Safe to Archive** | Append `(COMPLETED)` to Phase title header $\rightarrow$ Move to `plans/archive/<subpath>`. |
| **Category B: Superseded / Obsolete** | Code evolved differently in `src/` or proposed design was replaced. | **Recommend: Archive with Banner** | Add top banner `> [!NOTE] Superseded by refactor in src/...` $\rightarrow$ Move to `plans/archive/<subpath>`. |
| **Category C: Still Active / Incomplete** | Unfinished subtasks (`- [ ]`) remain or key proposed features are missing from `src/`. | **Recommend: DO NOT Archive (Keep in `plans/`)** | Report remaining work to user. Do NOT archive unless explicit user override is provided. |

---

## 4. Archiving Execution Protocol

Once the user approves or confirms the audit recommendations:

1. **Document Banner & Header Update**:
   - For **Category A**: Append `(COMPLETED)` to the Phase title header (e.g., `### [PH-DTSM]: Phase 1 - Corrupted Ley-Nodes (COMPLETED)`).
   - For **Category B**: Prepend a prominent GitHub-style alert banner at the top of the file:
     ```markdown
     > [!NOTE]
     > **Superseded / Obsolete Plan**
     > Archived on YYYY-MM-DD. Architecture evolved in `src/` (see relevant source files).
     ```
2. **Relative Subpath Preservation**:
   - Determine relative path of the file inside `plans/`.
   - Recreate exact relative sub-directory structure inside `plans/archive/`.
   - *Example*: `plans/phase4/sub.md` $\rightarrow$ `plans/archive/phase4/sub.md`
   - *Example*: `plans/PLAN_FEATURE.md` $\rightarrow$ `plans/archive/PLAN_FEATURE.md`
3. **File Transfer**:
   - Write updated file to `plans/archive/<relative_path>` using `write_to_file`.
   - Remove/clean up original un-archived file in `plans/`.
4. **Formatting Standards**:
   - Enforce no trailing whitespace and a single trailing newline at EOF on all created/edited files.

---

## 5. Search Scoping & `.rgignore` Verification

1. **Verify `.rgignore` Entry**:
   - Confirm `plans/archive/` exists in `.rgignore` at repository root so `grep_search` and `ripgrep` skip archived plans during code searches.
2. **Git Tracking Compliance**:
   - Archived files remain tracked in Git version control.
   - Do NOT execute `git add`, `git mv`, or `git commit` commands unless explicitly requested by the user during the active turn.

---

## 6. Summary & Audit Report Output

Upon completing the audit and archiving operation, output a structured report:

- **Audited Plan Path**
- **Classification Result** (Category A / B / C)
- **Recommendation Given & User Decision**
- **Source Path $\rightarrow$ Target Archived Path**
- **`.rgignore` Compliance Status**
