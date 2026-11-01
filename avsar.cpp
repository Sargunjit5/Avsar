// AVSAR - Smart Seat Booking & Resource Optimization System
// Har Slot Ka Sahi Avsar!
// Sir's requirement: seat allocation + booking records + cancellation waiting list
// ADSA: Queue (waitlist), Min-Heap (lowest free seat / earliest slot), Hash Table (booking ID lookup),
//       Searching, Sorting (utilization report)
#include <bits/stdc++.h>
using namespace std;

const int DAYS = 7, H_START = 9, H_END = 17;  // slots 9:00 .. 16:00
struct Resource { int id; string name; int seats; };
struct Booking { int id; string user; int res, day, hour, seat; string status; };

// ---------- Hash Table (chaining): bookingID -> index ----------
class HashTable {
    static const int M = 101;
    list<pair<int,int>> t[M];
public:
    void put(int k, int v) {
        for (auto &p : t[k % M]) if (p.first == k) { p.second = v; return; }
        t[k % M].push_back({k, v});
    }
    int get(int k) {
        for (auto &p : t[k % M]) if (p.first == k) return p.second;
        return -1;
    }
};

vector<Resource> resources = {{1,"Computer Lab",6},{2,"Badminton Court",4},{3,"Seminar Hall",8},{4,"Projector Kit",1}};
vector<Booking> bookings;
HashTable idx;
int nextId = 1001;
map<long, vector<int>> seatMap;        // slot -> seat(1..n) -> booking index, -1 = free
map<long, queue<string>> waitq;        // slot -> FIFO cancellation waiting list (Queue)
unordered_map<string,string> users;
map<string, vector<string>> inbox;
const string ADMIN = "admin";

long key(int r, int d, int h) { return r * 10000L + d * 100 + h; }
string hhmm(int h) { return to_string(h) + ":00"; }
void notify(const string &u, const string &m) { inbox[u].push_back(m); }
bool validRes(int r) { return r >= 1 && r <= (int)resources.size(); }
bool validSlot(int d, int h) { return d >= 1 && d <= DAYS && h >= H_START && h < H_END; }

vector<int>& seatsOf(int r, int d, int h) {
    auto &v = seatMap[key(r, d, h)];
    if (v.empty()) v.assign(resources[r-1].seats + 1, -1);
    return v;
}
int freeCount(int r, int d, int h) {
    auto &v = seatsOf(r, d, h); int c = 0;
    for (size_t i = 1; i < v.size(); i++) c += v[i] == -1;
    return c;
}
// Min-Heap: auto-allocate the lowest-numbered free seat
int lowestFreeSeat(int r, int d, int h) {
    auto &v = seatsOf(r, d, h);
    priority_queue<int, vector<int>, greater<int>> pq;
    for (size_t i = 1; i < v.size(); i++) if (v[i] == -1) pq.push(i);
    return pq.empty() ? -1 : pq.top();
}
// Conflict detection: one person cannot be in two places at the same time
bool userBusy(const string &u, int d, int h) {
    for (auto &b : bookings) if (b.status == "CONFIRMED" && b.user == u && b.day == d && b.hour == h) return true;
    return false;
}
int addBooking(const string &u, int r, int d, int h, int seat) {
    int id = nextId++;
    bookings.push_back({id, u, r, d, h, seat, "CONFIRMED"});
    idx.put(id, bookings.size() - 1);
    seatsOf(r, d, h)[seat] = bookings.size() - 1;
    return id;
}
// Min-Heap: earliest alternative slots that still have a free seat
vector<pair<int,int>> suggest(const string &u, int r, int d, int h, int n = 3) {
    priority_queue<pair<int,int>, vector<pair<int,int>>, greater<>> pq;
    for (int dd = d; dd <= DAYS; dd++)
        for (int hh = H_START; hh < H_END; hh++) {
            if (dd == d && hh <= h) continue;
            if (freeCount(r, dd, hh) > 0 && !userBusy(u, dd, hh)) pq.push({dd, hh});
        }
    vector<pair<int,int>> out;
    while (!pq.empty() && (int)out.size() < n) { out.push_back(pq.top()); pq.pop(); }
    return out;
}
// Auto seat recovery: freed seat goes to the first person in the waiting list
void releaseSeat(int bi, const string &status) {
    Booking b = bookings[bi];                 // copy (vector may grow below)
    bookings[bi].status = status;
    seatsOf(b.res, b.day, b.hour)[b.seat] = -1;
    auto &q = waitq[key(b.res, b.day, b.hour)];
    while (!q.empty()) {
        string next = q.front(); q.pop();
        if (userBusy(next, b.day, b.hour)) continue;
        int id = addBooking(next, b.res, b.day, b.hour, b.seat);
        notify(next, "WAITLIST PROMOTED! Booking #" + to_string(id) + ": " + resources[b.res-1].name +
               ", Day " + to_string(b.day) + " " + hhmm(b.hour) + ", Seat " + to_string(b.seat));
        cout << "  >> Seat " << b.seat << " auto-allocated to waiting user '" << next << "' (Booking #" << id << ")\n";
        break;
    }
}

// ---------- File persistence ----------
void save() {
    ofstream f("bookings.txt");
    for (auto &b : bookings) f << b.id << ' ' << b.user << ' ' << b.res << ' ' << b.day << ' ' << b.hour << ' ' << b.seat << ' ' << b.status << '\n';
    ofstream w("waitlist.txt");
    for (auto &p : waitq) { auto q = p.second; while (!q.empty()) { w << p.first << ' ' << q.front() << '\n'; q.pop(); } }
    ofstream u("users.txt");
    for (auto &p : users) u << p.first << ' ' << p.second << '\n';
}
void load() {
    ifstream f("bookings.txt"); Booking b;
    while (f >> b.id >> b.user >> b.res >> b.day >> b.hour >> b.seat >> b.status) {
        bookings.push_back(b);
        idx.put(b.id, bookings.size() - 1);
        if (b.status == "CONFIRMED") seatsOf(b.res, b.day, b.hour)[b.seat] = bookings.size() - 1;
        nextId = max(nextId, b.id + 1);
    }
    ifstream w("waitlist.txt"); long k; string u;
    while (w >> k >> u) waitq[k].push(u);
    ifstream uf("users.txt"); string n, p;
    while (uf >> n >> p) users[n] = p;
    if (!users.count(ADMIN)) users[ADMIN] = to_string(hash<string>{}("admin123"));
}

// ---------- UI helpers ----------
int readInt(const string &prompt) {
    int x;
    while (true) {
        cout << prompt;
        if (cin >> x) return x;
        cin.clear(); cin.ignore(1000, '\n');
        cout << "  Please enter a number.\n";
    }
}
void listResources() {
    cout << "\n  Resources:\n";
    for (auto &r : resources) cout << "   " << r.id << ". " << r.name << " (" << r.seats << " seats)\n";
}
void showSeatMap(int r, int d) {
    cout << "\n  " << resources[r-1].name << " - Day " << d << "   ([n]=free seat, [X]=taken)\n";
    for (int h = H_START; h < H_END; h++) {
        auto &v = seatsOf(r, d, h);
        cout << "   " << setw(5) << hhmm(h) << "  ";
        for (size_t i = 1; i < v.size(); i++) cout << (v[i] == -1 ? "[" + to_string(i) + "]" : "[X]") << ' ';
        auto &q = waitq[key(r, d, h)];
        if (freeCount(r, d, h) == 0) cout << " FULL" << (q.empty() ? "" : " (" + to_string(q.size()) + " waiting)");
        cout << "\n";
    }
}

// ---------- User actions ----------
void bookSeat(const string &u) {
    listResources();
    int r = readInt("  Resource no: "); if (!validRes(r)) { cout << "  Invalid resource.\n"; return; }
    int d = readInt("  Day (1-7): "); if (d < 1 || d > DAYS) { cout << "  Invalid day.\n"; return; }
    showSeatMap(r, d);
    int h = readInt("  Hour (9-16): "); if (!validSlot(d, h)) { cout << "  Invalid hour.\n"; return; }
    if (userBusy(u, d, h)) { cout << "  Conflict: you already have a booking at that time.\n"; return; }
    if (freeCount(r, d, h) == 0) {
        cout << "  This slot is FULL. Alternative slots with free seats (earliest first):\n";
        auto alt = suggest(u, r, d, h);
        for (auto &a : alt) cout << "   - Day " << a.first << " at " << hhmm(a.second) << "\n";
        if (alt.empty()) cout << "   (none available)\n";
        if (readInt("  Join cancellation waiting list? (1=Yes 0=No): ") == 1) {
            waitq[key(r, d, h)].push(u);
            cout << "  Added to waiting list. Position: " << waitq[key(r, d, h)].size() << "\n";
        }
        return;
    }
    int s = readInt("  Seat no (0 = auto-assign): ");
    auto &v = seatsOf(r, d, h);
    if (s == 0) s = lowestFreeSeat(r, d, h);
    else if (s < 1 || s >= (int)v.size() || v[s] != -1) { cout << "  That seat is invalid or already taken.\n"; return; }
    int id = addBooking(u, r, d, h, s);
    cout << "  Booked! Booking ID: " << id << "  Seat: " << s << "\n";
    notify(u, "Booking #" + to_string(id) + " confirmed: " + resources[r-1].name + ", Day " + to_string(d) + " " + hhmm(h) + ", Seat " + to_string(s));
}
void cancelBooking(const string &u, bool noShow = false) {
    int id = readInt("  Booking ID: ");
    int i = idx.get(id);   // O(1) hash lookup
    if (i < 0 || bookings[i].status != "CONFIRMED") { cout << "  No active booking with that ID.\n"; return; }
    if (u != ADMIN && bookings[i].user != u) { cout << "  Not your booking.\n"; return; }
    releaseSeat(i, noShow ? "NOSHOW" : "CANCELLED");
    cout << "  Booking #" << id << (noShow ? " marked no-show; seat released.\n" : " cancelled; seat released.\n");
}
void modifyBooking(const string &u) {
    int id = readInt("  Booking ID to modify: ");
    int i = idx.get(id);
    if (i < 0 || bookings[i].status != "CONFIRMED" || bookings[i].user != u) { cout << "  No such active booking of yours.\n"; return; }
    int r = bookings[i].res;
    int d = readInt("  New day (1-7): "), h = readInt("  New hour (9-16): ");
    if (!validSlot(d, h)) { cout << "  Invalid day/hour.\n"; return; }
    if (d == bookings[i].day && h == bookings[i].hour) { cout << "  Same slot; nothing to change.\n"; return; }
    if (userBusy(u, d, h) || freeCount(r, d, h) == 0) { cout << "  New slot is full or conflicts; booking unchanged.\n"; return; }
    int s = lowestFreeSeat(r, d, h);
    releaseSeat(i, "MODIFIED");
    int nid = addBooking(u, r, d, h, s);
    cout << "  Booking moved. New Booking ID: " << nid << "  Seat: " << s << "\n";
}
void printRecord(const Booking &b) {
    cout << "  " << b.id << "  " << left << setw(16) << b.user << setw(17) << resources[b.res-1].name << right
         << "Day " << b.day << "  " << setw(5) << hhmm(b.hour) << "  Seat " << b.seat << "  " << b.status << "\n";
}
void myHistory(const string &u) {
    cout << "\n  Your booking records:\n";
    for (auto &b : bookings) if (b.user == u) printRecord(b);
}
void viewNotifications(const string &u) {
    auto &v = inbox[u];
    cout << "\n  Notifications (" << v.size() << "):\n";
    for (auto &m : v) cout << "   * " << m << "\n";
    v.clear();
}
void lookup() {
    int i = idx.get(readInt("  Booking ID: "));
    if (i < 0) cout << "  Not found.\n"; else printRecord(bookings[i]);
}

// ---------- Admin ----------
void dashboard() {
    int conf = 0, canc = 0, ns = 0, wait = 0;
    for (auto &b : bookings) { conf += b.status == "CONFIRMED"; canc += b.status == "CANCELLED"; ns += b.status == "NOSHOW"; }
    for (auto &p : waitq) wait += p.second.size();
    cout << "\n  ===== ADMIN DASHBOARD =====\n  Active: " << conf << " | Cancelled: " << canc
         << " | No-shows: " << ns << " | On waiting lists: " << wait << "\n\n  Seat utilization (sorted):\n";
    vector<pair<double,int>> u;   // Sorting
    for (auto &r : resources) {
        int c = 0;
        for (auto &b : bookings) if (b.res == r.id && b.status == "CONFIRMED") c++;
        u.push_back({100.0 * c / (DAYS * (H_END - H_START) * r.seats), r.id});
    }
    sort(u.rbegin(), u.rend());
    for (auto &p : u)
        cout << "   " << left << setw(17) << resources[p.second-1].name << right << fixed << setprecision(1) << setw(6) << p.first
             << "%  " << string((int)(p.first / 2), '#') << (p.first < 5 ? "  <- underutilized" : "") << "\n";
}
void allRecords() {
    cout << "\n  ALL BOOKING RECORDS\n";
    for (auto &b : bookings) printRecord(b);
}

void userMenu(const string &u) {
    viewNotifications(u);
    while (true) {
        cout << "\n  --- Welcome " << u << " ---\n  1.Book seat  2.Cancel  3.Modify  4.My records  5.Notifications  6.View seat map  7.Find Booking ID  0.Logout\n";
        int c = readInt("  > ");
        if (c == 0) break;
        else if (c == 1) bookSeat(u);
        else if (c == 2) cancelBooking(u);
        else if (c == 3) modifyBooking(u);
        else if (c == 4) myHistory(u);
        else if (c == 5) viewNotifications(u);
        else if (c == 6) { listResources(); int r = readInt("  Resource: "), d = readInt("  Day: "); if (validRes(r) && d >= 1 && d <= DAYS) showSeatMap(r, d); }
        else if (c == 7) lookup();
        save();
    }
}
void adminMenu() {
    while (true) {
        cout << "\n  --- ADMIN ---\n  1.Dashboard  2.Mark No-Show  3.Cancel any booking  4.All records  5.Find Booking ID  0.Logout\n";
        int c = readInt("  > ");
        if (c == 0) break;
        else if (c == 1) dashboard();
        else if (c == 2) cancelBooking(ADMIN, true);
        else if (c == 3) cancelBooking(ADMIN);
        else if (c == 4) allRecords();
        else if (c == 5) lookup();
        save();
    }
}

int main() {
    load();
    cout << "\n  =====================================\n   AVSAR  -  Har Slot Ka Sahi Avsar!\n  =====================================\n";
    while (true) {
        cout << "\n  1.Login  2.Register  0.Exit\n";
        int c = readInt("  > ");
        if (c == 0) break;
        string n, p;
        cout << "  Username: "; cin >> n; cout << "  Password: "; cin >> p;
        string h = to_string(hash<string>{}(p));
        if (c == 2) {
            if (users.count(n)) cout << "  Username taken.\n";
            else { users[n] = h; save(); cout << "  Registered! Now login.\n"; }
        } else if (c == 1) {
            if (!users.count(n) || users[n] != h) cout << "  Wrong username or password.\n";
            else if (n == ADMIN) adminMenu();
            else userMenu(n);
        }
    }
    save();
    cout << "  Data saved. Goodbye!\n";
}
