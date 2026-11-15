# Smart Study Planner — Team Avengers

A clean, browser-based **final PBL project** that runs the **same C++17 algorithm engine** used in our Phase 3 console application. The interface uses plain HTML, CSS and JavaScript; the backend is a small local C++ HTTP server using standard networking APIs (Winsock on Windows, POSIX sockets on Linux).

**Five pillars**

| Pillar | C++ implementation | Browser view |
|---|---|---|
| Smart Scheduling | Custom binary Max-Heap (priority order) + earliest-deadline-first day-wise timetable | Scheduling |
| Deadline Conflict Detection | Manual insertion sort + cumulative hours feasibility | Conflicts |
| Adaptive Recovery | Missed-hour capacity reduction + rebuilt timetable | Recovery |
| Academic Risk Predictor | Rule-based LOW / MEDIUM / HIGH grouped by shared deadline | Risk |
| What-If Simulator | Temporary hypothetical hours/day and missed hours; original data unchanged | What-if |

The browser displays **real JSON results from the C++ program**, not JavaScript copies of the planning algorithms.

## Run on Windows (recommended for viva)

1. Install **G++** (MinGW-w64, for example via MSYS2) and confirm `g++ --version` works in Command Prompt.
2. Download/clone this entire repository and extract/open it in a folder.
3. Double-click **`START_PROJECT.bat`**. It compiles the web server and opens Chrome/your default browser.
4. Visit **http://127.0.0.1:8080**. Keep the black terminal window open while using the interface.
5. Press **Ctrl+C** in the terminal to stop.

If the browser opens before compilation/server startup finishes, refresh the page once. If port 8080 is already occupied, close the other server before restarting.

**No npm, Node.js, database, internet connection or hosted website is needed to run the project.** The G++ compiler is needed for the first build and when code changes.

## Run on Linux / macOS

From the project root:

```bash
g++ -std=c++17 -Wall -Wextra -pedantic backend/server.cpp -o planner_server
./planner_server
```

Open **http://127.0.0.1:8080** in a browser. All files are served locally.

## Optional: console version (same C++ core)

```bash
g++ -std=c++17 backend/console_main.cpp -o planner_console
./planner_console
```

On Windows, use `START_CONSOLE.bat`.

## Suggested evaluator walkthrough (about 3–5 minutes)

1. Go to **My tasks → Load sample tasks** (works when the task list is empty) or add tasks manually.
2. View **Smart scheduling**: explain urgency score, custom Max-Heap, then earliest-deadline-first allocation.
3. View **Deadline conflicts**: show cumulative required vs available hours.
4. In **Adaptive recovery**, miss 2 study hours today. Return to conflicts to prove the change affected all pillars.
5. View **Academic risks**: explain why risk is LOW / MEDIUM / HIGH.
6. Run **What-if** with 5 hours per day and zero missed hours. Explain why it does not change the saved planner.
7. On **Overview**, update daily study hours or advance one study day; return to My tasks and log 1 hour of actual progress.
8. Stop and restart the application to demonstrate automatic persistence.

## Files

```
frontend/index.html       Minimal dashboard layout
frontend/style.css        Responsive styling
frontend/script.js        Fetches the C++ API and renders its results
backend/planner_core.hpp  Shared Phase 3 DSA algorithms, plus read-only JSON getters
backend/server.cpp        Local HTTP/JSON integration layer
backend/console_main.cpp  Original Phase 3 menu, using the same core
tests/test_web.py         Integration tests against the real backend
CONTRIBUTORS.md           Team ownership and authentic GitHub workflow
docs/                    Existing Phase 3 reference materials
```

## How the integration works

`Browser → JavaScript fetch() → HTTP (localhost:8080) → C++ StudyPlanner → JSON response → dashboard`.

Example `GET /api/state` returns tasks, priority order, conflicts, timetable and risk levels. `POST /api/whatif` computes a temporary scenario without changing the saved state.

## Important assumptions to explain honestly

- Whole-number study hours, relative deadlines measured in **study sessions**, and a single daily capacity; not a real calendar app.
- Select **Advance study day** manually. Deadlines decrease by one and missed hours reset.
- Actual study progress must be explicitly logged; viewing a suggested timetable does not complete a task.
- Priorities and day allocation intentionally differ: Max-Heap ranks urgency; earliest-deadline-first protects deadlines.
- Risk classification is **rule-based**, not artificial intelligence or machine learning.
- Local files are stored as `planner_data.txt` in the project root and automatically saved after changes; do not commit this file.
- The local HTTP service binds only to `127.0.0.1`; it is intended for classroom demonstrations, **not public internet hosting**.

## Tests

```bash
python tests/test_web.py
```

Test runner builds the server, uses a temporary working directory, and checks the actual HTTP/C++ integration. It requires Python 3 and G++.

## Contributors

See [CONTRIBUTORS.md](CONTRIBUTORS.md). Each member should create their **own real commits** through their GitHub account; listing names alone will not generate GitHub attribution.
