// Team Avengers - minimal local HTTP bridge for the real C++ study planner.
// No external C++ libraries. Windows: Winsock. Linux/macOS: POSIX sockets.
#include "planner_core.hpp"
#include <sstream>
#include <map>
#include <stdexcept>
#include <cctype>
#include <cstdlib>
#include <cstring>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using Socket = SOCKET;
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
using Socket = int;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

static string jsonEscape(const string& value) {
    string out = "\"";
    const char* digits = "0123456789abcdef";
    for (unsigned char c : value) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (c < 32) { out += "\\u00"; out += digits[c >> 4]; out += digits[c & 15]; }
        else out += (char)c;
    }
    return out + "\"";
}
static string taskJson(const Task& t) {
    return "{\"id\":" + to_string(t.id) + ",\"name\":" + jsonEscape(t.name)
      + ",\"importance\":" + to_string(t.importance) + ",\"deadline\":" + to_string(t.deadline)
      + ",\"remaining\":" + to_string(t.remaining) + ",\"urgency\":" + to_string(t.urgency()) + "}";
}
static string tasksJson(const vector<Task>& tasks) {
    string s = "[";
    for (size_t i = 0; i < tasks.size(); i++) { if (i) s += ","; s += taskJson(tasks[i]); }
    return s + "]";
}
static string conflictsJson(const vector<Conflict>& items) {
    string s = "[";
    for (size_t i = 0; i < items.size(); i++) {
        if (i) s += ",";
        const Conflict& c = items[i];
        s += "{\"deadline\":" + to_string(c.deadline) + ",\"needed\":" + to_string(c.needed)
          + ",\"available\":" + to_string(c.available) + ",\"shortage\":" + to_string(c.shortage) + "}";
    }
    return s + "]";
}
static string planJson(const Plan& p) {
    string s = "{\"unscheduled\":" + to_string(p.unscheduled) + ",\"conflicts\":" + conflictsJson(p.conflicts) + ",\"blocks\":[";
    for (size_t i = 0; i < p.blocks.size(); i++) {
        if (i) s += ",";
        const Block& b = p.blocks[i];
        s += "{\"day\":" + to_string(b.day) + ",\"id\":" + to_string(b.id)
          + ",\"hours\":" + to_string(b.hours) + ",\"name\":" + jsonEscape(b.name) + "}";
    }
    return s + "]}";
}
static string stateJson(const StudyPlanner& planner) {
    string risks = "[";
    vector<pair<int,string>> items = planner.getRisks();
    for (size_t i = 0; i < items.size(); i++) {
        if (i) risks += ",";
        risks += "{\"id\":" + to_string(items[i].first) + ",\"risk\":" + jsonEscape(items[i].second) + "}";
    }
    risks += "]";
    return "{\"ok\":true,\"day\":" + to_string(planner.getCurrentDay())
        + ",\"dailyHours\":" + to_string(planner.getDailyHours())
        + ",\"missedToday\":" + to_string(planner.getMissedToday())
        + ",\"tasks\":" + tasksJson(planner.getTasks())
        + ",\"priority\":" + tasksJson(planner.getPriorityTasks())
        + ",\"conflicts\":" + conflictsJson(planner.getConflicts())
        + ",\"plan\":" + planJson(planner.getPlan())
        + ",\"risks\":" + risks + "}";
}
static string decode(const string& input) {
    string result;
    for (size_t i = 0; i < input.size(); i++) {
        if (input[i] == '+') result += ' ';
        else if (input[i] == '%' && i + 2 < input.size() && isxdigit((unsigned char)input[i+1]) && isxdigit((unsigned char)input[i+2])) {
            result += (char)strtol(input.substr(i+1, 2).c_str(), nullptr, 16); i += 2;
        } else result += input[i];
    }
    return result;
}
static map<string,string> parseForm(const string& body) {
    map<string,string> values;
    istringstream parts(body);
    string item;
    while (getline(parts, item, '&')) {
        size_t pos = item.find('=');
        if (pos != string::npos) values[decode(item.substr(0,pos))] = decode(item.substr(pos+1));
    }
    return values;
}
static bool number(const map<string,string>& values, const string& key, int minValue, int maxValue, int& out) {
    auto it = values.find(key);
    if (it == values.end() || it->second.empty() || it->second.size() > 10) return false;
    for (char c : it->second) if (!isdigit((unsigned char)c)) return false;
    try { long long parsed = stoll(it->second); if (parsed < minValue || parsed > maxValue) return false; out = (int)parsed; return true; }
    catch (...) { return false; }
}
static string err(const string& msg) { return "{\"ok\":false,\"error\":" + jsonEscape(msg) + "}"; }
static string change(StudyPlanner& planner, const string& path, const map<string,string>& data) {
    int id, hours, missed, importance, deadline;
    if (path == "/api/add") {
        auto it = data.find("name");
        if (it == data.end() || it->second.empty() || it->second.size() > 70 ||
            it->second.find_first_not_of(" \t\r\n") == string::npos) return err("Enter a task name (1-70 characters).");
        if (!number(data,"importance",1,3,importance) || !number(data,"deadline",1,365,deadline) ||
            !number(data,"hours",1,10000,hours)) return err("Check importance, deadline and study hours.");
        planner.addTask(it->second,importance,deadline,hours);
    } else if (path == "/api/progress") {
        if (!number(data,"id",1,1000000,id) || !number(data,"hours",1,10000,hours)) return err("Enter a valid task ID and positive study hours.");
        if (!planner.validTaskId(id)) return err("Task not found.");
        if (hours > planner.remainingForId(id)) return err("Study hours cannot exceed remaining hours.");
        planner.recordProgress(id,hours);
    } else if (path == "/api/remove") {
        if (!number(data,"id",1,1000000,id) || !planner.validTaskId(id)) return err("Task not found.");
        planner.deleteTask(id);
    } else if (path == "/api/hours") {
        if (!number(data,"hours",1,24,hours)) return err("Daily hours must be 1 to 24.");
        planner.setHours(hours);
    } else if (path == "/api/recovery") {
        if (!number(data,"missed",0,planner.getDailyHours(),missed)) return err("Missed hours must be between 0 and daily study hours.");
        planner.recover(missed);
    } else if (path == "/api/next") {
        if (planner.getCurrentDay() >= 10000) return err("Maximum study session reached.");
        planner.nextDay();
    } else if (path == "/api/demo") {
        if (!planner.getTasks().empty()) return err("Demo tasks can only be added to an empty planner.");
        planner.addTask("Data Structures assignment",3,1,3);
        planner.addTask("OOP practical",2,2,4);
        planner.addTask("Mathematics revision",2,3,2);
    } else if (path == "/api/whatif") {
        if (!number(data,"hours",1,24,hours) || !number(data,"missed",0,24,missed) || missed > hours)
            return err("Choose 1-24 daily hours and missed hours no higher than daily hours.");
        return "{\"ok\":true,\"scenario\":" + planJson(planner.getPlanFor(hours,missed))
            + ",\"hours\":" + to_string(hours) + ",\"missed\":" + to_string(missed) + "}";
    } else return err("Unknown action.");
    return stateJson(planner);
}
static string readFile(const string& path) {
    ifstream in("frontend/" + path, ios::binary);
    if (!in) return "";
    return string(istreambuf_iterator<char>(in), istreambuf_iterator<char>());
}
static void closeSocket(Socket s) {
#ifdef _WIN32
    closesocket(s);
#else
    close(s);
#endif
}
static void sendAll(Socket client, const string& message) {
    size_t pos = 0;
    while (pos < message.size()) {
        int count = send(client, message.data()+pos, (int)min((size_t)16384,message.size()-pos), 0);
        if (count <= 0) break;
        pos += count;
    }
}
static void process(Socket client, StudyPlanner& planner) {
    string request;
    char buffer[4096];
    size_t headerEnd = string::npos;
    while (request.size() < 65536 && (headerEnd = request.find("\r\n\r\n")) == string::npos) {
        int count = recv(client,buffer,sizeof(buffer),0);
        if (count <= 0) return;
        request.append(buffer,count);
    }
    if (headerEnd == string::npos) return;
    istringstream first(request.substr(0,request.find("\r\n")));
    string method,path,version;
    first >> method >> path >> version;
    size_t contentLength = 0;
    istringstream headers(request.substr(request.find("\r\n")+2,headerEnd));
    string line;
    while (getline(headers,line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.size() >= 15 && line.substr(0,15) == "Content-Length:") {
            try { contentLength = stoul(line.substr(15)); } catch (...) { return; }
        }
    }
    if (contentLength > 16384) return;
    string body = request.substr(headerEnd+4);
    while (body.size() < contentLength) {
        int count = recv(client,buffer,sizeof(buffer),0);
        if (count <= 0) return;
        body.append(buffer,count);
    }
    string mime = "application/json; charset=utf-8";
    string response;
    int status = 200;
    if (method == "GET" && path == "/api/state") response = stateJson(planner);
    else if (method == "POST" && path.compare(0,5,"/api/") == 0) response = change(planner,path,parseForm(body.substr(0,contentLength)));
    else if (method == "GET") {
        string target;
        if (path == "/" || path == "/index.html") target = "index.html";
        else if (path == "/style.css") target = "style.css";
        else if (path == "/script.js") target = "script.js";
        else {status=404; response="Not found";mime="text/plain";}
        if (!target.empty()) {
            response = readFile(target);
            if (response.empty()) {status=404;response="File not found";mime="text/plain";}
            else mime = target.substr(target.size()-3)=="css" ? "text/css; charset=utf-8" :
                target.substr(target.size()-2)=="js" ? "application/javascript; charset=utf-8" : "text/html; charset=utf-8";
        }
    } else { status=404; response=err("Route not found."); }
    string answer = "HTTP/1.1 " + to_string(status) + (status==200 ? " OK" : " Not Found") + "\r\n"
        + "Content-Type: " + mime + "\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\n"
        + "Content-Length: " + to_string(response.size()) + "\r\nConnection: close\r\n\r\n" + response;
    sendAll(client,answer);
}
int main() {
#ifdef _WIN32
    WSADATA data;
    if (WSAStartup(MAKEWORD(2,2), &data) != 0) { cerr << "Winsock failed.\n"; return 1; }
#endif
    Socket server = socket(AF_INET,SOCK_STREAM,0);
    if (server == INVALID_SOCKET) {cerr << "Socket could not start.\n";return 1;}
    sockaddr_in address{};
    address.sin_family = AF_INET;
    int port = 8080;
    if (const char* env = getenv("PLANNER_PORT")) {
        char* end = nullptr;
        long selected = strtol(env, &end, 10);
        if (end && *end == '\0' && selected >= 1024 && selected <= 65535) port = (int)selected;
    }
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(0x7F000001); // Only local laptop can connect.
    int reuse=1;
#ifndef _WIN32
    setsockopt(server,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
#endif
    if (bind(server,(sockaddr*)&address,sizeof(address)) == SOCKET_ERROR || listen(server,8) == SOCKET_ERROR) {
        cerr << "Port " << port << " is unavailable. Close another server and retry.\n";
        closeSocket(server); return 1;
    }
    StudyPlanner planner;
    cout << "\nTEAM AVENGERS - STUDY PLANNER\nOpen http://127.0.0.1:" << port << " in Chrome\nPress Ctrl+C to stop.\n";
    while (true) {
        Socket client = accept(server,nullptr,nullptr);
        if (client == INVALID_SOCKET) continue;
        try { process(client,planner); } catch (const exception& ex) { cerr << "Request error: " << ex.what() << '\n'; }
        closeSocket(client);
    }
}
