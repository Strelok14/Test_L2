#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <memory>
#include <exception>

using namespace std;

struct Point { string group; uint32_t x, y; };

bool ends_with(const string& str, const string& suffix) {
    return str.size() >= suffix.size() &&
        str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

class Parser {
public:
    virtual vector<Point> parse(const string& filename) = 0;
    virtual ~Parser() = default;
};

class TxtParser : public Parser {
public:
    vector<Point> parse(const string& filename) override {
        vector<Point> points;
        ifstream in(filename);
        if (!in) throw runtime_error("Can't open txt: " + filename);
        string line;
        while (getline(in, line)) {
            auto c = line.find(':');
            auto com = line.find(',');
            if (c == string::npos || com == string::npos) throw runtime_error("Bad txt: " + filename);
            string group = line.substr(0, c);
            uint32_t x = stoul(line.substr(c + 1, com - c - 1));
            uint32_t y = stoul(line.substr(com + 1));
            points.push_back({ group, x, y });
        }
        return points;
    }
};

class BinParser : public Parser {
public:
    vector<Point> parse(const string& filename) override {
        vector<Point> points;
        ifstream in(filename, ios::binary);
        if (!in) throw runtime_error("Can't open bin: " + filename);
        while (true) {
            char group;
            uint32_t x = 0, y = 0;
            in.read(&group, 1);
            if (!in) break;
            in.read(reinterpret_cast<char*>(&x), sizeof(x));
            in.read(reinterpret_cast<char*>(&y), sizeof(y));
            if (!in) throw runtime_error("Bad bin: " + filename);
            points.push_back({ string(1, group), x, y });
        }
        return points;
    }
};

class JsonParser : public Parser {
public:
    vector<Point> parse(const string& filename) override {
        vector<Point> points;
        ifstream in(filename);
        if (!in) throw runtime_error("Can't open json: " + filename);
        string data((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
        size_t pos = data.find("[");
        if (pos == string::npos) throw runtime_error("Bad json: " + filename);
        size_t end = data.find("]", pos);
        if (end == string::npos) throw runtime_error("Bad json: " + filename);

        string arr = data.substr(pos + 1, end - pos - 1);
        stringstream ss(arr);
        string item;
        while (getline(ss, item, '{')) {
            size_t close = item.find('}');
            if (close == string::npos) continue;
            string obj = item.substr(0, close);
            string group;
            uint32_t x = 0, y = 0;
            size_t gpos = obj.find("\"group\"");
            if (gpos != string::npos) {
                size_t q1 = obj.find('"', gpos + 7);
                size_t q2 = obj.find('"', q1 + 1);
                if (q1 != string::npos && q2 != string::npos)
                    group = obj.substr(q1 + 1, q2 - q1 - 1);
            }
            size_t xpos = obj.find("\"x\"");
            if (xpos != string::npos) {
                size_t colon = obj.find(':', xpos);
                size_t comma = obj.find(',', colon);
                if (colon != string::npos)
                    x = stoul(obj.substr(colon + 1, comma - colon - 1));
            }
            size_t ypos = obj.find("\"y\"");
            if (ypos != string::npos) {
                size_t colon = obj.find(':', ypos);
                size_t comma = obj.find(',', colon);
                if (colon != string::npos)
                    y = stoul(obj.substr(colon + 1, comma - colon - 1));
            }
            if (!group.empty())
                points.push_back({ group, x, y });
        }
        return points;
    }
};

unique_ptr<Parser> make_parser(const string& filename) {
    if (ends_with(filename, ".txt")) return make_unique<TxtParser>();
    if (ends_with(filename, ".bin")) return make_unique<BinParser>();
    if (ends_with(filename, ".json")) return make_unique<JsonParser>();
    throw runtime_error("Unknown ext: " + filename);
}

int main(int argc, char* argv[]) {
    if (argc < 2) { cerr << "Usage: point_parser file1 [file2...]\n"; return 1; }
    try {
        for (int i = 1; i < argc; ++i) {
            string fname = argv[i];
            auto parser = make_parser(fname);
            auto points = parser->parse(fname);
            cout << "file: " << fname << "\n";
            cout << "group,x,y\n";
            for (const auto& pt : points) {
                cout << pt.group << "," << pt.x << "," << pt.y << "\n";
            }
        }
        return 0;
    }
    catch (const exception& e) {
        cerr << e.what() << endl;
        return 2;
    }
}