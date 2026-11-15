# Team Avengers — contributors and learning ownership

| Member | Assigned area | Code to study and extend |
|---|---|---|
| **Shubh Bhatt** (team lead) | Integration, adaptive recovery, project architecture | `backend/planner_core.hpp`: `buildPlan`, `recover`, `nextDay`; frontend-to-backend integration |
| **Rohan Rawat** | Smart Scheduling and Priority Queue | `Task::urgency`, `MaxHeap`, `getPriorityTasks` |
| **Ayushman Bharti** | Deadline Conflict Detection and Risk Predictor | `deadlineOrder`, `findConflicts`, `getRisks` |
| **Yash Taragi** | File persistence, What-If Simulator, testing | `saveData`, `loadData`, `getPlanFor`, `tests/` |

These are **documented team responsibilities**, not evidence that each person authored a GitHub commit. To appear in GitHub's contributor history, each member must make authentic changes and push from their own account with a linked/verified Git email.

## Contribution workflow

1. Repository owner adds each teammate as a collaborator: **Settings → Collaborators → Add people**.
2. Each teammate clones the repository and creates a branch (for example `rohan-heap-explanation`).
3. Each member uses their own Git email: `git config user.name "Their Name"` and `git config user.email "their-verified-github-email"`.
4. Make meaningful improvements, tests, diagrams or documentation in the assigned area. Avoid cosmetic commits made solely to boost contribution statistics.
5. Commit, push and open a pull request; the team lead reviews and merges it.

Example:

```bash
git checkout -b rohan-heap-tests
# Add a test or clarify the heap implementation.
git add .
git commit -m "test: cover Max-Heap priority order"
git push -u origin rohan-heap-tests
```

GitHub contribution credits are based on real Git commit authorship, account email verification, repository branch/merge rules and GitHub's contribution-counting behavior. Do not impersonate another teammate.
