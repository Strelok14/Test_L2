#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <future>

struct Point { std::string group; uint32_t x, y; };
struct FilePoints { std::string filename; std::vector<Point> points; };

bool ends_with(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() &&
        str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

class Parser {
public:
    virtual std::vector<Point> parse(const std::string& filename) = 0;
    virtual ~Parser() = default;
};

class TxtParser : public Parser {
public:
    std::vector<Point> parse(const std::string& filename) override {
        std::vector<Point> points;
        std::ifstream in(filename);
        if (!in) throw std::runtime_error("Can't open txt: " + filename);
        std::string line;
        while (std::getline(in, line)) {
            auto c = line.find(':');
            auto com = line.find(',');
            if (c == std::string::npos || com == std::string::npos) throw std::runtime_error("Bad txt: " + filename);
            std::string group = line.substr(0, c);
            uint32_t x = std::stoul(line.substr(c + 1, com - c - 1));
            uint32_t y = std::stoul(line.substr(com + 1));
            points.push_back({ group, x, y });
        }
        return points;
    }
};

class BinParser : public Parser {
public:
    std::vector<Point> parse(const std::string& filename) override {
        std::vector<Point> points;
        std::ifstream in(filename, std::ios::binary);
        if (!in) throw std::runtime_error("Can't open bin: " + filename);
        while (true) {
            uint32_t raw = 0;
            in.read(reinterpret_cast<char*>(&raw), sizeof(raw));
            if (in.gcount() == 0) break;
            if (in.gcount() != sizeof(raw)) throw std::runtime_error("Bad bin: " + filename);
            uint32_t group = (raw >> 24) & 0xFF;
            uint32_t x = (raw >> 12) & 0xFFF;
            uint32_t y = raw & 0xFFF;
            points.push_back({ std::to_string(group), x, y });
        }
        return points;
    }
};

class JsonParser : public Parser {
public:
    std::vector<Point> parse(const std::string& filename) override {
        // ОЧЕНЬ простой парсер для формата:
        // {"points":[{"group":"A","x":5,"y":7},{"group":"B","x":9,"y":10}]}
        std::vector<Point> points;
        std::ifstream in(filename);
        if (!in) throw std::runtime_error("Can't open json: " + filename);
        std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        size_t pos = data.find("[");
        if (pos == std::string::npos) throw std::runtime_error("Bad json: " + filename);
        size_t end = data.find("]", pos);
        if (end == std::string::npos) throw std::runtime_error("Bad json: " + filename);

        std::string arr = data.substr(pos + 1, end - pos - 1);
        std::stringstream ss(arr);
        std::string item;
        while (std::getline(ss, item, '{')) {
            size_t close = item.find('}');
            if (close == std::string::npos) continue;
            std::string obj = item.substr(0, close);
            std::string group;
            uint32_t x = 0, y = 0;
            size_t gpos = obj.find("\"group\"");
            if (gpos != std::string::npos) {
                size_t q1 = obj.find('"', gpos + 7);
                size_t q2 = obj.find('"', q1 + 1);
                if (q1 != std::string::npos && q2 != std::string::npos)
                    group = obj.substr(q1 + 1, q2 - q1 - 1);
            }
            size_t xpos = obj.find("\"x\"");
            if (xpos != std::string::npos) {
                size_t colon = obj.find(':', xpos);
                size_t comma = obj.find(',', colon);
                if (colon != std::string::npos)
                    x = std::stoul(obj.substr(colon + 1, comma - colon - 1));
            }
            size_t ypos = obj.find("\"y\"");
            if (ypos != std::string::npos) {
                size_t colon = obj.find(':', ypos);
                size_t comma = obj.find(',', colon);
                if (colon != std::string::npos)
                    y = std::stoul(obj.substr(colon + 1, comma - colon - 1));
            }
            if (!group.empty())
                points.push_back({ group, x, y });
        }
        return points;
    }
};

std::unique_ptr<Parser> make_parser(const std::string& filename) {
    if (ends_with(filename, ".txt")) return std::make_unique<TxtParser>();
    if (ends_with(filename, ".bin")) return std::make_unique<BinParser>();
    if (ends_with(filename, ".json")) return std::make_unique<JsonParser>();
    throw std::runtime_error("Unknown ext: " + filename);
}

int main(int argc, char* argv[]) {
    if (argc < 2) { std::cerr << "Usage: point_parser file1 [file2...]\n"; return 1; }
    std::vector<std::string> files(argv + 1, argv + argc);
    std::vector<std::future<FilePoints>> fut;
    try {
        for (const auto& f : files) {
            fut.push_back(std::async([f] {
                auto p = make_parser(f);
                return FilePoints{ f, p->parse(f) };
                }));
        }
        std::vector<FilePoints> all;
        for (auto& f : fut) all.push_back(f.get());

        // Выводим результат для каждого файла
        for (const auto& fp : all) {
            std::cout << "file: " << fp.filename << "\n";
            std::cout << "group,x,y\n";
            for (const auto& pt : fp.points) {
                std::cout << pt.group << "," << pt.x << "," << pt.y << "\n";
            }
        }
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 2;
    }
}