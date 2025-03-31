#include <iostream> // cout, string, from_chars()
#include <span> // span
#include <ranges> // vws::transform, vws::iota, rng::to, etc.
#include <fstream> // ifstream, open(), rdbuf()
#include <sstream> // stringstream
#include <cctype> // isalpha(), tolower()
#include <unordered_map> // unordered_map
#include <algorithm> // max, find
#include <string_view> // sv literal

namespace rng = std::ranges;
namespace vws = rng::views;

using namespace std::string_view_literals;

int main(int argc, char* argv[])
{
    if (argc == 1) {
        std::cout << "Please provide a path to a text file as an argument, --help for more details\n";
        return 0;
    }

    std::span args{ argv, (size_t)argc };

    if (rng::any_of(args, [](std::string s) { return s == "--help"; })) {
        std::string usage = 
        "Usage: histogrammer input_file_path [OPTIONS]\n"
        "Prints histogram of characters in text file at input_file_path\n"
        "OPTIONS, any subset of the following:\n"
        "\t-r row_count\toverride row_count, program draws row_count rows of text-based histogram, default = 10\n"
        "\t-s tick_stride\toverride tick_stride, program draws a tick every tick_stride rows, default = 3\n"
        "Arguments must be positive integers\n";
        std::cout << usage;
        return 0;
    }

    // read given file into string
    std::string filename{ args[1] };
    std::ifstream file;
    file.open(filename);
    if (!file.is_open()) {
        std::cout << "File \"" << filename << "\" not found\n";
        return 1;
    }
    std::stringstream buf;
    buf << file.rdbuf();
    auto input = buf.str();

    // default settings
    size_t rows{ 10 };
    size_t tickStride{ 3 };

    // search for flag [[param]] in [[args]], override setting based on it,
    // return false if invalid arguments are encountered
    auto parseParam = [&](auto param, auto& setting) {
        auto it = rng::find(args, param);
        if (it != args.end()) {
            auto next = it + 1; // grab the next arg after the flag
            if (next != args.end()) {
                std::string s{ *next }; // parse it as typeof(setting)
                auto res = std::from_chars(s.data(), s.data() + s.size(), setting);
                if (res.ec != std::errc{} || rng::any_of(s, [&](auto c) { return !isdigit(c); })) {
                    // error could be e.g. arg is NaN, or number is out of range
                    // also reject non-digit chars in general because from_chars allows garbage in the tail
                    std::cout << "Invalid argument for flag " << param << '\n';
                    return false;
                }
            }
            else {
                std::cout << "Missing argument for flag " << param << '\n';
                return false;
            }
        }
        return true;
    };
    // check for -r and -s settings in args, exit on failure
    if (!parseParam("-r"sv, rows)) {
        return 1;
    };
    if (!parseParam("-s"sv, tickStride)) {
        return 1;
    };

    size_t peak{}; // number of hits in the most populated bin
    std::unordered_map<char, size_t> hist; // histogram data
    auto alphabet = vws::iota(0, 26) | vws::transform([](auto i) { return 'a' + i; }); // range of chars from a to z
    // bin the input text into histogram
    for (const auto& c : input) {
        if (isalpha(c)) {
            auto& bin = hist[tolower(c)]; // either fetches bin or default constructs the size_t bin to 0
            bin++;
            peak = std::max(bin, peak);
        }
    }

    // figure out how much padding is needed for the ticks
    // e.g. for values between 100 ... 999, log10 will be 2.0 ... ~2.99, so flooring it and adding one gives the number of digits
    size_t digits = static_cast<size_t>(floor(log10(peak))) + 1ULL;

    // draw histogram upper part, one row at a time, counting down to 0 from [[rows]]
    for (const auto& r : vws::iota(0ULL, rows) | vws::reverse) {
        // determine row bounds: [rowFloor, rowCeil)
        auto rowFloor = (static_cast<double>(r) / rows) * peak;
        auto rowCeil = (static_cast<double>(r + 1) / rows) * peak;

        // draw ticks only on rows based on given tickStride
        // invert because we want the top row to always have a tick, -1 because [[rows]] is one-past-end
        bool drawTick = (rows - 1 - r) % tickStride == 0;

        // mark the tick as the middle of the range covered by the row (TODO: check if this is actually the desired behavior)
        auto tickStr = (drawTick ? std::to_string(static_cast<size_t>(0.5 * (rowFloor + rowCeil))) : "");
        tickStr.insert(tickStr.begin(), digits - tickStr.size(), ' ');

        // draw vertical axis and ticks
        std::cout << tickStr;
        std::cout << "|";

        // draw filled/empty bins
        for (const auto& c : alphabet) {
            std::cout << ((hist[c] > rowFloor) ? '*' : ' ');
        }
        std::cout << '\n';
    }

    // draw histogram horizontal axis and ticks
    std::cout << std::string(digits, ' ') << '+' << std::string(alphabet.size(), '-') << '\n';
    auto abc = rng::to<std::string>(alphabet); // string a...z
    abc.insert(abc.begin(), '|'); // axis line
    abc.insert(abc.begin(), digits, ' '); // left-pad with spaces to line up with ticks above
    std::cout << abc << '\n';
}
