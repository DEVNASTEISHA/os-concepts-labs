#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <limits>
#include <cstring>
#include <cstdlib>

using namespace std;

static constexpr size_t MAX_SIZE = 1 << 24; // 16 MB
enum Role { Hole, Process };

struct section {
    int _id;
    string _name;
    Role _role;
    size_t _base;
    size_t _limit;

    section(Role role, size_t base, size_t limit, string name, int id=0)
        : _id(id), _name(move(name)), _role(role), _base(base), _limit(limit) {}

    void setId(int id) {_id = id; }
    void setName(string name) { _name = name; }
    void markAsHole() { _id = 0; _name = "hole"; _role = Hole; }
    void markAsProcess(int id) { _role = Process; _id = id; }
    void setBase(size_t base) { _base = base; }
    void setLimit(size_t limit) { _limit = limit; }

    int id() const { return _id; }
    string name() const { return _name; }
    Role role() const { return _role; }
    size_t base() const { return _base; }
    size_t limit() const { return _limit; }
};

struct Memory {
    vector<section*> sections;
    size_t _size;
    char* _memory;
    int processCount{};
    int lastPid{};
    
    Memory(size_t requested) {
        requested = min(requested, MAX_SIZE);
        _size = (requested > 0) ? requested : 1; // Allocate at least 1 for portability
        _memory = new char[_size];
        memset(_memory, 0, _size);
        sections.push_back(new section(Hole, 0, _size, "hole", 0));
    }

    ~Memory() {
        for (section*s : sections) delete s;
        delete[] _memory;
    }

    void initialize(size_t requested) {
        requested = min(requested, MAX_SIZE);
        _size = (requested < 0) ? 0 : requested;

        for (section* s : sections) delete s;
        sections.clear();

        delete[] _memory;
        _memory = new char[_size];
        memset(_memory, 0, _size);

        sections.push_back(new section(Hole, 0, _size, "hole", 0));
        processCount = 0;
        lastPid = 0;
    }

    int nextPid() { return ++lastPid; }

    // Merge neighbors of the element at iterator 'it'. Returns iterator to merged element.
    vector<section*>::iterator combine(vector<section*>::iterator it) {
        if (it == sections.end()) return it;
        if (it != sections.begin()) {
            auto prev = it - 1;
            if ((*prev)->role() == Hole && (*prev)->base() + (*prev)->limit() == (*it)->base()) {
                (*prev)->setLimit((*prev)->limit() + (*it)->limit());
                delete *it;
                it = sections.erase(it);
                it = prev;
            }
        }
        if (it + 1 != sections.end()) {
            auto next = it + 1;
            if ((*next)->role() == Hole && (*it)->base() + (*it)->limit() == (*next)->base()) {
                (*it)->setLimit((*it)->limit() + (*next)->limit());
                delete *next;
                sections.erase(next);
            }
        }
        return it;
    }

    // Find hole based on pre-defined algorithm(f:first, b:best, w:worst)
    section* findHole(int reqSize, const string& algo="f") {
        if (reqSize == 0)
            return nullptr;

        string a = algo;
        if (!a.empty()) a[0] = (char)tolower(a[0]);

        section* res = nullptr;

        if (a.empty() || a[0] == 'f') {
            for (section* s : sections) {
                if ( s->role() == Hole && s->limit() >= reqSize)
                    return s;
            }
        } else if (a[0] == 'b') {
            size_t bestSize {SIZE_MAX};
            for (section* s : sections) {
                if (s->role() == Hole && s->limit() >= reqSize && s->limit() < bestSize) {
                    bestSize = s->limit();
                    res = s;
                }
            }
            return res;
        } else if (a[0] == 'w') {
            int worstSize {};
            for (section* s : sections) {
                if (s->role() == Hole && s->limit() >= reqSize && s->limit() > worstSize) {
                    worstSize = s->limit();
                    res = s;
                }
            }
            return res;
        }
        return nullptr;
    }

    section* findProcess(string pName) {
        if (pName == "hole") return nullptr;
        for (section*  s : sections) {
            if (s->name() == pName && s->role() == Process) return s;
        }
        return nullptr;
    }

    void push(section* s) {
        sections.push_back(s);
        int idx = (int)sections.size() - 1;
        while (idx > 0 && sections[idx]->base() < sections[idx-1]->base()) {
            swap(sections[idx], sections[idx-1]);
            --idx;
        }
        if (s->role() == Hole) combine(sections.begin() + idx);
    }

    void request(const string& pName, size_t size, const string& algo) {
        if (size == 0) {
            cout << "-> Occupication cancelled for size 0" << endl;
            return;
        }
        if (pName.empty() || pName == "hole") {
            cout << "-> Invalid process name" << endl;
            return;
        }
        if (findProcess(pName)) {
            cout << "-> Process name already exists: [" << pName << "]" << endl;
            return;
        }

        section* hole = findHole(size, algo);
        if (!hole) { cout << "-> Not enough space available for [" << pName << "]" << endl; return; }

        size_t base = hole->base();
        size_t limit = hole->limit();

        int pid = nextPid();
        if (limit == size) { // If size exactly fits hole
            hole->markAsProcess(pid);
            hole->setName(pName);
            memset(_memory + base, static_cast<unsigned char>(pid % 256), size);
            ++processCount;
        } else { // Split hole and take first 'size' bytes for the process
            hole->setLimit(size);
            hole->markAsProcess(pid);
            hole->setName(pName);
            memset(_memory + base, static_cast<unsigned char>(pid % 256), size);
            ++processCount;

            size_t newBase = base + size;
            size_t newLimit = limit - size;
            section* newHole = new section(Hole, newBase, newLimit, "hole", 0);
            push(newHole);
        }
        cout << "-> Allocated " << size << "for [" << pName << "] successfully." << endl;
    }

    void release(section* p) {
        if (!p || p->role() == Hole) return;

        size_t base = p->base();
        size_t limit = p->limit();
    
        memset(_memory + base, 0, limit);
        p->markAsHole();
        --processCount;

        // Combine conjugates
        auto it = find(sections.begin(), sections.end(), p);
        if (it == sections.end()) return;
        combine(it);
    }

    // Compact
    void compact() {
        vector<section*> newSections;
        size_t base = 0;

        for (section* s : sections) {
            if (s->role() == Process) {
                size_t oldBase = s->base();
                if (oldBase != base)
                    memmove(_memory + base, _memory + oldBase, s->limit());
                s->setBase(base);
                newSections.push_back(s);
                base += s->limit();
            } else {
                delete s;
            }
        }
        size_t holeSize = (_size > base) ? (_size - base) : 0;
        if (holeSize > 0)
            newSections.push_back(new section(Hole, base, holeSize, "hole", 0));
        sections.swap(newSections);
        if (holeSize > 0 )
            memset(_memory + base, 0, _size - base);
    }

    void reportEstate() {
        for (section* s : sections)
            cout << "[" << s->id() << "] " << s->name()
                 << " - [" << s->base() << ": " << (s->base() + s->limit()) << ") - "
                 << s->limit() << endl;
    }
};

bool parseSize(const string& token, size_t& out) {
    if (token.empty())
        return false;

    string s = token;
    char last = s.back();
    size_t multiplier = 1;

    if (last == 'K' || last == 'k') {
        multiplier = 1024;
        s.pop_back();
    } else if (last == 'M' || last == 'm') {
        multiplier = 1024 * 1024;
        s.pop_back();
    }
    if (s.empty())
        return false;
    for (char c : s)
        if (!isdigit((unsigned char)c))
            return false;
    try {
        unsigned long long val = stoull(s);
        unsigned long long total = val * (unsigned long long)multiplier;

        if (total > (unsigned long long)MAX_SIZE)
            return false;

        out = (size_t)total;
        return true;
    } catch (...) {
        return false;
    }
}

vector<string> tokenize(const string& line) {
    vector<string> toks;

    istringstream iss(line);
    string tok;

    while (iss >> tok)
        toks.push_back(tok);
    return toks; }

string toUpper(const string& s) {
    string r = s;
    for (char &c : r)
        c = (char)toupper((unsigned char)c);
    return r; }

int main() {
    Memory memory(1024 * 1024);
    cout << "Simple contiguous allocator. Type HELP for commands.\n";

    while (true) {
        cout << "allocator> ";

        string line;
        if (!getline(cin, line))
            break;

        auto cmd = tokenize(line);
        if (cmd.empty())
            continue;

        string command = toUpper(cmd[0]);
        try {
            if (command == "HELP" || command == "?") {
                cout << "Commands:\n"
                     << "  RQ <name> <size[K|M]> [f|b|w] - Request (first/best/worst)\n"
                     << "  RL <name> - Release\n"
                     << "  CMP | COMPACT - Compact memory\n"
                     << "  STAT - Show segments\n"
                     << "  SIZE <size[K|M]> - Reinitialize memory size\n"
                     << "  X | EXIT - Exit\n";
            } else if (command == "X" || command == "EXIT" || command == "Q" || command == "QUIT") {
                break;
            } else if (command == "SIZE") {
                if (cmd.size() < 2) {
                    cout << "Usage: SIZE <size>\n";
                    continue;
                }

                size_t s;
                if (!parseSize(cmd[1], s)) {
                    cout << "Invalid size\n";
                    continue;
                }
                memory.initialize(s);
                cout << "-> Allocated " << s << '\n';
            } else if (command == "RQ") {
                if (cmd.size() < 3) {
                    cout << "Usage: RQ <name> <size> [f|b|w]\n";
                    continue;
                }
                string name = cmd[1];
                size_t s;
                if (!parseSize(cmd[2], s)) {
                    cout << "Invalid size\n";
                    continue;
                }
                string algo = (cmd.size() >= 4) ? cmd[3] : "f";
                memory.request(name, s, algo);
            } else if (command == "RL") {
                if (cmd.size() < 2) {
                    cout << "Usage: RL <name>\n";
                    continue;
                }
                section* p = memory.findProcess(cmd[1]);
                if (!p) {
                    cout << "-> No such process: " << cmd[1] << '\n';
                    continue;
                }
                size_t pSize = p->limit();
                memory.release(p);
                cout << "-> Released [" << cmd[1] << "]: " << pSize << '\n';
            } else if (command == "CMP" || command == "COMPACT") {
                memory.compact();
                cout << "-> Successfully compacted memory\n";
            }
            else if (command == "STAT")
                memory.reportEstate();
            else {
                size_t s;
                if (parseSize(cmd[0], s)) {
                    memory.initialize(s);
                    cout << "-> Allocated " << s << '\n';
                }
                else cout << "Unknown command. Type HELP.\n";
            }
        } catch (...) {
            cout << "WARNING: Error has occurred\n";
            continue;
        }
    }
    return 0;
}
