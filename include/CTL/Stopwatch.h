#ifndef STOPWATCH_H
#define STOPWATCH_H

#include <ctime>
#include <sstream>

using namespace std;

class Stopwatch {
    bool _running = false;
    clock_t _start;
    clock_t _stop;

  public:
    [[nodiscard]] auto started() const -> double { return _start; }
    [[nodiscard]] auto stopped() const -> double { return _stop; }
    [[nodiscard]] auto running() const -> bool { return _running; }
    void start() {
        _running = true;
        _start = clock();
    }
    void stop() {
        _stop = clock();
        _running = false;
    }
    [[nodiscard]] auto duration() const -> double {
        return ((double(_stop - _start)) * 1000) / CLOCKS_PER_SEC;
    }

    auto operator<<(ostream &os) -> ostream & {
        os << duration() << " ms";
        return os;
    }

    auto to_string() -> std::string {
        stringstream ss;
        ss << this;
        return ss.str();
    }
};

#endif // STOPWATCH_H
