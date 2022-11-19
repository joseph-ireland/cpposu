#pragma once

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string_view>
#include <sstream>
#include <charconv>
#include <optional>

namespace cpposu {

#define CPPOSU_RAISE_PARSE_ERROR(args...) do { \
        std::ostringstream ss; ss << "Parse error in " << filename_ << " line " << line_number_ << ": " << args; \
        throw parse_error(ss.str()); \
    } while (0)

struct parse_error : std::runtime_error
{
    using std::runtime_error::runtime_error;
};


inline std::string_view trim_leading_space(std::string_view data)
{
    size_t pos=data.find_first_not_of(" \t");
    if (pos != std::string_view::npos)
        return data.substr(pos);
    return {};
}

inline std::string_view trim_trailing_space(std::string_view data)
{
    size_t pos=data.find_last_not_of(" \t");
    if (pos != std::string_view::npos)
        return data.substr(0,pos+1);
    return {};

}
inline std::string_view trim_space(std::string_view data)
{
    return trim_leading_space(trim_trailing_space(data));
}

inline bool try_take_prefix(std::string_view& data, std::string_view prefix)
{
    if (data.starts_with(prefix))
    {
        data.remove_prefix(prefix.size());
        data = trim_leading_space(data);
        return true;
    }
    return false;
}

inline std::optional<std::string_view> try_take_column(std::string_view& data, char delimiter=',')
{
    size_t pos=data.find_first_of(delimiter);
    if (pos == std::string_view::npos)
    {
        // note: different to data.empty()
        // This won't be true if data.remove_prefix() leaves an empty string.
        // it's only true if the pointer is explicitly nulled - allows detecting empty last column.
        if (!data.data()) return {};
        std::string_view result = data;
        data = {};
        return trim_space(result);
    }

    std::string_view result = trim_space(data.substr(0,pos));
    data.remove_prefix(pos+1);
    return result;
}

template<typename T=double>
std::optional<T> read_number(std::string_view& str)
{
    T result;
    auto [next, ec] = std::from_chars(str.data(), str.data()+str.size(), result);
    if (ec != std::errc{})
    {
        return {};
    }
    return result;
}

template<typename T>
[[nodiscard]] bool read_number(T& result, std::string_view& str)
{
    auto x = read_number<T>(str);
    if (x)
        result = *x;

    return x;
}
template<typename T=double>
std::optional<T> try_take_numeric_column(std::string_view& line, char delimiter=',')
{
    auto str = try_take_column(line, delimiter);
    if (!str) return {};
    return read_number<T>(*str);
}
template<typename T=double>
[[nodiscard]] bool try_take_numeric_column(T& value, std::string_view& line, char delimiter=',')
{
    auto optional_value = try_take_numeric_column(line, delimiter);
    if (optional_value)
        value = *optional_value;
    return optional_value.has_value();
}




class LineParser
{
    void init()
    {
        if (!stream_)
            CPPOSU_RAISE_PARSE_ERROR("Failed to open file");

        line_data_.reserve(1024);
    }
public:
    LineParser(std::istream& stream, std::string filename="<unknown>"):
        stream_(stream),
        filename_(filename)
    {
        init();
    }

    LineParser(std::string filename):
        fstream_(std::make_unique<std::ifstream>(filename)),
        stream_(*fstream_),
        filename_(filename)
    {
        init();
    }


    std::unique_ptr<std::ifstream> fstream_;
    std::istream& stream_;
    std::string filename_;
    std::string line_data_;
    size_t line_number_ = 0;

    struct DebugLocation
    {
        std::string_view line;
        size_t error_index;
        friend std::ostream& operator<<(std::ostream& os, const DebugLocation& d)
        {
            os << "\n\n    "<< d.line << "\n    ";
            if (d.error_index <= d.line.size())
                return  os<< std::string(d.error_index, ' ') << "^\n";
            else
                return os << "^ INVALID ERROR INDEX " << d.error_index <<"\n";
        }
    };
    DebugLocation debug_location(const std::string_view& data)
    {
        size_t index = data.data() ? (data.data()-line_data_.data()) : line_data_.size();
        return DebugLocation{line_data_, index};
    }

    std::string_view read_line()
    {
        while (std::getline(stream_, line_data_))
        {
            ++line_number_;

            auto line = trim_space(line_data_);
            if (!line.empty())
                return line;
        }
        return {};
    }

    std::string_view reread_last_line()
    {
        return trim_space(line_data_);
    }

    template<typename T=double>
    T read_number_or_throw(std::string_view& line)
    {
        auto result = read_number<T>(line);
        if (result) return *result;

        CPPOSU_RAISE_PARSE_ERROR("failed to read number: " << debug_location(line));
    }
    template<typename T>
    void read_number_or_throw(T& result, std::string_view& line)
    {
        result = read_number_or_throw<T>(line);
    }

    std::string_view take_column(std::string_view& data, char delimiter=',')
    {
        auto column = try_take_column(data, delimiter);
        if (!column)
            CPPOSU_RAISE_PARSE_ERROR("expected delimiter '" << delimiter << "' at " << debug_location(data));
        return *column;
    }

    template<typename T=double>
    T take_numeric_column(std::string_view& line, char delimiter=',')
    {
        auto column = take_column(line,delimiter);
        return read_number_or_throw(column);
    }

    template<typename T=double>
    void take_numeric_column(T& result, std::string_view& line, char delimiter=',')
    {
        result = take_numeric_column(line, delimiter);
    }

};

}
