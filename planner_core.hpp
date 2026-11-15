#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <limits>
#include <algorithm>
using namespace std;

// TEAM AVENGERS - The same Phase 3 scheduling engine is used by both interfaces.


// TEAM AVENGERS - SMART STUDY PLANNER - FINAL PBL
// C++17. Algorithms are written manually for DSA learning.
// A day means one study session; all hours and deadlines are integers.
struct Task {
    int id, importance, deadline, remaining;
    string name;

    int urgency() const {
        int points = 10;
        if (deadline <= 1) points = 50;
        else if (deadline <= 3) points = 30;
        return importance * 20 + points + remaining;
    }
};

class MaxHeap {
    vector<Task> data;
    bool higher(const Task& a, const Task& b) const {
        if (a.urgency() != b.urgency()) return a.urgency() > b.urgency();
        if (a.deadline != b.deadline) return a.deadline < b.deadline;
        return a.id < b.id;
    }
public:
    bool empty() const { return data.empty(); }
    void insert(Task t) {
        data.push_back(t);
        int child = (int)data.size() - 1;
        while (child > 0) {
            int parent = (child - 1) / 2;
            if (!higher(data[child], data[parent])) break;
            swap(data[child], data[parent]);
            child = parent;
        }
    }
    Task removeHighest() {
        Task answer = data[0]; // Call only when heap is not empty.
        data[0] = data.back();
        data.pop_back();
        int parent = 0;
        while (parent < (int)data.size()) {
            int left = parent * 2 + 1, right = parent * 2 + 2;
            int best = parent;
            if (left < (int)data.size() && higher(data[left], data[best])) best = left;
            if (right < (int)data.size() && higher(data[right], data[best])) best = right;
            if (best == parent) break;
            swap(data[parent], data[best]);
            parent = best;
        }
        return answer;
    }
};

struct Block { int day, id, hours; string name; };
struct Conflict { int deadline, needed, available, shortage; };
struct Plan {
    vector<Block> blocks;
    vector<Conflict> conflicts;
    int unscheduled = 0;
};

class StudyPlanner {
    vector<Task> tasks;
    int nextId = 1;
    int dailyHours = 3;
    int missedToday = 0;
    int currentDay = 1; // Session counter; day 1 is the first planning day.
    const string fileName = "planner_data.txt";

    int findIndex(int id) const { // Linear search: O(n).
        for (int i = 0; i < (int)tasks.size(); i++)
            if (tasks[i].id == id) return i;
        return -1;
    }

    vector<Task> deadlineOrder() const { // Insertion sort: O(n^2).
        vector<Task> result;
        for (Task t : tasks) if (t.remaining > 0) result.push_back(t);
        for (int i = 1; i < (int)result.size(); i++) {
            Task key = result[i];
            int j = i - 1;
            while (j >= 0 && (result[j].deadline > key.deadline ||
                   (result[j].deadline == key.deadline && result[j].id > key.id))) {
                result[j + 1] = result[j];
                j--;
            }
            result[j + 1] = key;
        }
        return result;
    }

    int capacity(int deadline, int hours, int missed) const {
        // Deadline 0 means overdue: no remaining time.
        if (deadline <= 0) return 0;
        return deadline * hours - missed;
    }

    vector<Conflict> findConflicts(int hours, int missed) const {
        vector<Task> ordered = deadlineOrder();
        vector<Conflict> found;
        int needed = 0;
        for (int i = 0; i < (int)ordered.size();) {
            int deadline = ordered[i].deadline;
            while (i < (int)ordered.size() && ordered[i].deadline == deadline) {
                needed += ordered[i].remaining;
                i++;
            }
            int available = capacity(deadline, hours, missed);
            if (needed > available)
                found.push_back({deadline, needed, available, needed - available});
        }
        return found;
    }

    Plan buildPlan(int hours, int missed) const {
        Plan plan;
        plan.conflicts = findConflicts(hours, missed);
        vector<Task> ordered = deadlineOrder();
        int lastDay = 0;
        for (Task t : ordered) lastDay = max(lastDay, t.deadline);
        int index = 0;
        for (int day = 1; day <= lastDay; day++) {
            int free = hours - (day == 1 ? missed : 0);
            while (free > 0 && index < (int)ordered.size()) {
                if (ordered[index].deadline < day) { index++; continue; }
                int used = min(free, ordered[index].remaining);
                plan.blocks.push_back({day, ordered[index].id, used, ordered[index].name});
                ordered[index].remaining -= used;
                free -= used;
                if (ordered[index].remaining == 0) index++;
            }
        }
        for (Task t : ordered) plan.unscheduled += t.remaining;
        return plan;
    }



    void showConflicts(int hours, int missed) const {
        vector<Conflict> conflicts = findConflicts(hours, missed);
        if (conflicts.empty()) {
            cout << "No deadline conflicts. All pending work fits.\n";
            return;
        }
        for (Conflict c : conflicts)
            cout << "Deadline " << c.deadline << ": need " << c.needed
                 << " h, available " << c.available << " h, shortage "
                 << c.shortage << " h.\n";
    }

    void showPlan(int hours, int missed) const {
        Plan plan = buildPlan(hours, missed);
        cout << "\nDAY-WISE PLAN (earliest deadline first)\n";
        int lastPrinted = 0;
        for (Block b : plan.blocks) {
            if (b.day != lastPrinted) {
                lastPrinted = b.day;
                cout << "Day " << b.day << " (session " << currentDay + b.day - 1 << "):\n";
            }
            cout << "  " << b.name << " [ID " << b.id << "]: " << b.hours << " h\n";
        }
        if (plan.blocks.empty()) cout << "No study blocks scheduled.\n";
        cout << "Unscheduled hours: " << plan.unscheduled << "\n";
        showConflicts(hours, missed);
        cout << "This is a suggestion; record actual study separately.\n";
    }

public:
    StudyPlanner() {
        if (loadData()) cout << "Saved planner loaded automatically.\n";
        else cout << "Starting a new planner.\n";
    }
    int getDailyHours() const { return dailyHours; }
    int getMissedToday() const { return missedToday; }
    int getCurrentDay() const { return currentDay; }
    vector<Task> getTasks() const { return tasks; }
    vector<Conflict> getConflicts() const { return findConflicts(dailyHours, missedToday); }
    vector<Conflict> getConflictsFor(int hours, int missed) const { return findConflicts(hours, missed); }
    Plan getPlan() const { return buildPlan(dailyHours, missedToday); }
    Plan getPlanFor(int hours, int missed) const { return buildPlan(hours, missed); }
    vector<Task> getPriorityTasks() const {
        MaxHeap heap;
        vector<Task> ordered;
        for (Task t : tasks) if (t.remaining > 0) heap.insert(t);
        while (!heap.empty()) ordered.push_back(heap.removeHighest());
        return ordered;
    }
    vector<pair<int, string>> getRisks() const {
        vector<pair<int, string>> result;
        vector<Task> ordered = deadlineOrder();
        int cumulative = 0;
        for (int i = 0; i < (int)ordered.size();) {
            int start = i;
            int deadline = ordered[i].deadline;
            while (i < (int)ordered.size() && ordered[i].deadline == deadline) {
                cumulative += ordered[i].remaining;
                i++;
            }
            int available = capacity(deadline, dailyHours, missedToday);
            string risk = "LOW";
            if (cumulative > available) risk = "HIGH";
            else if (deadline <= 2 || cumulative * 4 >= available * 3) risk = "MEDIUM";
            for (int j = start; j < i; j++) result.push_back({ordered[j].id, risk});
        }
        return result;
    }
    // Web UI applies input checks before calling the original core functions.
    bool validTaskId(int id) const { return findIndex(id) >= 0; }
    int remainingForId(int id) const {
        int index = findIndex(id);
        return index < 0 ? -1 : tasks[index].remaining;
    }

    void addTask(string name, int importance, int deadline, int hours) {
        if (nextId > 1000000) { cout << "Task ID limit reached.\n"; return; }
        tasks.push_back({nextId, importance, deadline, hours, name});
        cout << "Task added with ID " << nextId++ << ".\n";
        changed();
    }
    void showTasks() const {
        cout << "\nSession: " << currentDay << " | Daily hours: " << dailyHours
             << " | Missed today: " << missedToday << "\n";
        cout << "ID | Task | Importance | Deadline(days) | Remaining(h) | Urgency\n";
        for (Task t : tasks)
            cout << t.id << " | " << t.name << " | " << t.importance << " | "
                 << t.deadline << " | " << t.remaining << " | " << t.urgency() << '\n';
        if (tasks.empty()) cout << "No tasks.\n";
    }
    void priorityOrder() const {
        MaxHeap heap;
        for (Task t : tasks) if (t.remaining > 0) heap.insert(t);
        cout << "\nPRIORITY ORDER (custom Max-Heap)\n";
        int rank = 1;
        while (!heap.empty()) {
            Task t = heap.removeHighest();
            cout << rank++ << ". " << t.name << " (score " << t.urgency() << ")\n";
        }
        if (rank == 1) cout << "No pending tasks.\n";
    }
    void conflictCheck() const {
        cout << "\nCONFLICT CHECK (includes missed hours today)\n";
        showConflicts(dailyHours, missedToday);
    }
    void dayWisePlan() const { showPlan(dailyHours, missedToday); }
    void recordProgress(int id, int hours) {
        int index = findIndex(id);
        if (index < 0) { cout << "Task not found.\n"; return; }
        if (hours > tasks[index].remaining) { cout << "Hours exceed remaining work.\n"; return; }
        tasks[index].remaining -= hours;
        cout << "Progress recorded. Remaining: " << tasks[index].remaining << " h.\n";
        changed();
    }
    void recover(int missed) {
        missedToday = missed;
        changed();
        cout << "\nRECOVERY: Today's available hours = " << dailyHours - missedToday << "\n";
        showPlan(dailyHours, missedToday);
    }
    void riskReport() const {
        cout << "\nRISK REPORT (rule-based; shared deadlines evaluated together)\n";
        vector<Task> ordered = deadlineOrder();
        int cumulative = 0;
        for (int i = 0; i < (int)ordered.size();) {
            int start = i;
            int deadline = ordered[i].deadline;
            while (i < (int)ordered.size() && ordered[i].deadline == deadline) {
                cumulative += ordered[i].remaining;
                i++;
            }
            int available = capacity(deadline, dailyHours, missedToday);
            string risk = "LOW";
            if (cumulative > available) risk = "HIGH";
            else if (deadline <= 2 || cumulative * 4 >= available * 3) risk = "MEDIUM";
            for (int j = start; j < i; j++)
                cout << ordered[j].name << " (deadline " << deadline << "): " << risk << '\n';
        }
        if (ordered.empty()) cout << "No pending tasks.\n";
        cout << "These are rules, not machine-learning predictions.\n";
    }
    void whatIf(int hours, int missed) const {
        cout << "\nCURRENT: " << dailyHours << " h/day, " << missedToday << " missed\n";
        showConflicts(dailyHours, missedToday);
        cout << "\nWHAT-IF: " << hours << " h/day, " << missed << " missed\n";
        showPlan(hours, missed);
        cout << "Original planner remains unchanged.\n";
    }
    void setHours(int hours) {
        dailyHours = hours;
        if (missedToday > hours) missedToday = hours;
        changed();
        cout << "Daily capacity updated.\n";
    }
    void findTask(int id) const {
        int index = findIndex(id);
        if (index < 0) { cout << "Task not found.\n"; return; }
        Task t = tasks[index];
        cout << "Found: " << t.name << " | " << t.remaining << " h | deadline "
             << t.deadline << " days | urgency " << t.urgency() << '\n';
    }
    void deleteTask(int id) {
        int index = findIndex(id);
        if (index < 0) { cout << "Task not found.\n"; return; }
        tasks.erase(tasks.begin() + index);
        changed();
        cout << "Task removed.\n";
    }
    void nextDay() {
        // Start a new session. Yesterday's missed hours no longer apply
        // Remaining hours do not change automatically: user records actual progress.
        for (Task& t : tasks) if (t.deadline > 0) t.deadline--;
        currentDay++;
        missedToday = 0;
        changed();
        cout << "Advanced to session " << currentDay << ". Deadlines reduced by 1 day.\n";
        cout << "Tasks at deadline 0 are overdue until completed or removed.\n";
    }
    void saveNow() const { cout << (saveData() ? "Saved.\n" : "Save failed.\n"); }
    void reload() { cout << (loadData() ? "Reloaded.\n" : "Load failed; current data unchanged.\n"); }
};

