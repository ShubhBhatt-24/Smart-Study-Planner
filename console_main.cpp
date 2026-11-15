#include "planner_core.hpp"

int readNumber(string prompt, int minimum, int maximum) {
    while (true) {
        cout << prompt;
        int number;
        if (cin >> number && number >= minimum && number <= maximum) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return number;
        }
        if (cin.eof()) return minimum - 1;
        cout << "Enter a number from " << minimum << " to " << maximum << ".\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

int main() {
    StudyPlanner planner;
    cout << "SMART STUDY PLANNER | TEAM AVENGERS | FINAL PBL\n";
    while (true) {
        cout << "\n1 Add task            2 View tasks\n"
             << "3 Priority order      4 Deadline conflicts\n"
             << "5 Day-wise plan       6 Record progress\n"
             << "7 Adaptive recovery   8 Academic risks\n"
             << "9 What-if simulator  10 Set daily hours\n"
             << "11 Find task         12 Delete task\n"
             << "13 Advance one day   14 Save now\n"
             << "15 Reload saved data  0 Exit\n";
        int choice = readNumber("Choose: ", 0, 15);
        if (choice < 0 || choice == 0) break;
        if (choice == 1) {
            string name;
            cout << "Task name: ";
            getline(cin, name);
            if (name.empty() || name.size() > 70) {
                cout << "Name must contain 1-70 characters.\n";
                continue;
            }
            int imp = readNumber("Importance (1-3): ", 1, 3);
            int deadline = readNumber("Deadline in days (1-365): ", 1, 365);
            int hours = readNumber("Study hours required (1-10000): ", 1, 10000);
            if (imp < 0 || deadline < 0 || hours < 0) break;
            planner.addTask(name, imp, deadline, hours);
        } else if (choice == 2) planner.showTasks();
        else if (choice == 3) planner.priorityOrder();
        else if (choice == 4) planner.conflictCheck();
        else if (choice == 5) planner.dayWisePlan();
        else if (choice == 6) {
            int id = readNumber("Task ID: ", 1, 1000000);
            int hours = readNumber("Hours studied: ", 0, 10000);
            if (id < 0 || hours < 0) break;
            planner.recordProgress(id, hours);
        } else if (choice == 7) {
            int missed = readNumber("Hours missed today: ", 0, planner.getDailyHours());
            if (missed < 0) break;
            planner.recover(missed);
        } else if (choice == 8) planner.riskReport();
        else if (choice == 9) {
            int hours = readNumber("Try hours per day (1-24): ", 1, 24);
            if (hours < 0) break;
            int missed = readNumber("Try missed hours (0 to daily hours): ", 0, hours);
            if (missed < 0) break;
            planner.whatIf(hours, missed);
        } else if (choice == 10) {
            int hours = readNumber("New hours per day (1-24): ", 1, 24);
            if (hours < 0) break;
            planner.setHours(hours);
        } else if (choice == 11) {
            int id = readNumber("Task ID: ", 1, 1000000);
            if (id < 0) break;
            planner.findTask(id);
        } else if (choice == 12) {
            int id = readNumber("Task ID: ", 1, 1000000);
            if (id < 0) break;
            planner.deleteTask(id);
        } else if (choice == 13) planner.nextDay();
        else if (choice == 14) planner.saveNow();
        else if (choice == 15) planner.reload();
    }
    cout << "Goodbye. Changes were automatically saved.\n";
    return 0;
}
