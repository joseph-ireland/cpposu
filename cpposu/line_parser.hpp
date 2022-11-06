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

inline std::optional<std::string_view> try_take_column(std::string_view& data, char delimiter=',', bool allow_empty=false)
{
    size_t pos=data.find_first_of(delimiter);
    if (pos == std::string_view::npos)
    {
        if (allow_empty || !data.empty())
        {
            std::string_view result = data;
            data = {};
            return result;
        }
        else
            return {};
    }
    
    std::string_view result = trim_space(data.substr(0,pos));
    data.remove_prefix(pos+1);
    data = trim_leading_space(data);
    return result;
}

template<typename T=double>
std::optional<T> try_take_number(std::string_view& line)
{
    T result;
    auto [next, ec] = std::from_chars(line.data(), line.data()+line.size(), result);
    if (ec != std::errc{})
    {
        return {}; 
    }
    line = line.substr(next-line.data());
    line = trim_leading_space(line);
    return result;
}
template<typename T>
bool try_take_number(T& result, std::string_view& line)
{
    auto x = try_take_number<T>(line);
    if (x)
        result = *x;

    return x;
}
template<typename T=double>
std::optional<T> try_take_numeric_column(std::string_view& line, char delimiter=',')
{
    if (!line.empty())
    {
        auto result = try_take_number<T>(line);
        try_take_column(line, delimiter);
        return result;
    }
    return {};
}
template<typename T=double>
bool try_take_numeric_column(T& value, std::string_view& line, char delimiter=',')
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
        fstream_(std::make_unique<std::fstream>(filename)),
        stream_(*fstream_),
        filename_(filename)
    {
        init();
    }


    std::unique_ptr<std::fstream> fstream_;
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
            return os << "\n\n    " << d.line << "\n    " << std::string(d.error_index, ' ') << "^\n";
        }
    };
    DebugLocation debug_location(const std::string_view& data)
    {
        size_t index = (data.data()-line_data_.data());
        if (index > line_data_.size()) index=0; // prevents crash if we're somehow out of bounds, e.g. if data is empty/doesn't reference current line
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
    T take_number(std::string_view& line)
    {
        auto result = try_take_number<T>(line);
        if (result) return *result;
        
        CPPOSU_RAISE_PARSE_ERROR("failed to read number: " << debug_location(line)); 
    }
    template<typename T>
    void take_number(T& result, std::string_view& line)
    {
        result = take_number<T>(line);
    }

    std::string_view take_column(std::string_view& data, char delimiter=',', bool allow_empty=false)
    {
        auto column = try_take_column(data, delimiter, allow_empty);
        if (!column)
            CPPOSU_RAISE_PARSE_ERROR("expected delimiter '" << delimiter << "' at " << debug_location(data));
        return *column;
    }

    /// works for last column since umpty column would be invalid
    template<typename T=double>
    T take_numeric_column(std::string_view& line, char delimiter=',')
    {
        auto d = debug_location(line);
        auto column = try_take_numeric_column<T>(line, delimiter);
        if (!column)
            CPPOSU_RAISE_PARSE_ERROR("Expected number: " << d);
        return *column;
    }

    template<typename T=double>
    void take_numeric_column(T& result, std::string_view& line, char delimiter=',')
    {
        result = take_numeric_column(line, delimiter);
    }

};

}