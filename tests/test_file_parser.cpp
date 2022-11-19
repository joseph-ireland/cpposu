#include <external/catch2/catch.hpp>

#include <cpposu/line_parser.hpp>



static constexpr char basic_test_config[]=
R"(
  testing123,  strip me ,don't strip
don't strip, strip me 

a line, a nested list ;  more nesting; final, end

last
)";

TEST_CASE("trim operations", "[line_parser]")
{
    CHECK(cpposu::trim_leading_space("  \t te st \t") == "te st \t");
    CHECK(cpposu::trim_trailing_space("  \t test \t") == "  \t test");
    CHECK(cpposu::trim_space("  \t test \t") == "test");
    CHECK(cpposu::trim_space("  \t test test test \t") == "test test test");

    CHECK(cpposu::trim_space("test") == "test");
    CHECK(cpposu::trim_space("") == "");
    CHECK(cpposu::trim_space(" \t") == "");
    CHECK(cpposu::trim_trailing_space(" \t teast test") == " \t teast test");
    CHECK(cpposu::trim_leading_space("trim_leading_space \t ") == "trim_leading_space \t ");


}

TEST_CASE("Basic parsing", "[line_parser]")
{
    std::istringstream test_stream(basic_test_config);
    cpposu::LineParser parser(test_stream);
    auto line = parser.read_line();
    CHECK(line == "testing123,  strip me ,don't strip");
    CHECK(parser.take_column(line) == "testing123");
    CHECK(parser.take_column(line) == "strip me");
    CHECK(cpposu::try_take_column(line) == "don't strip");
    CHECK(line == "");
    CHECK(!cpposu::try_take_column(line));

    
    line = parser.read_line();
    CHECK(line == "don't strip, strip me");
    auto first_column = cpposu::try_take_column(line);
    CHECK(bool(first_column));
    CHECK(first_column.value() == "don't strip");
    CHECK(line == " strip me");
    CHECK(cpposu::try_take_column(line)=="strip me");
    CHECK(line.empty());
}
static constexpr char numeric_test_config[]=
R"(     

1,2,5.0,1e2 

 1,2 ,3;4; 5 ,6,7  
)";

TEST_CASE("Numeric parsing", "[line_parser]")
{
    std::istringstream numeric_test_stream(numeric_test_config);
    cpposu::LineParser parser(numeric_test_stream);
    auto line = parser.read_line();
    CHECK(line == "1,2,5.0,1e2");

    CHECK(parser.take_numeric_column<int>(line) == 1);
    int test_int;
    parser.take_numeric_column(test_int, line);
    CHECK(test_int == 2);
    CHECK(parser.take_numeric_column<double>(line) == 5.0);
    double test_double;
    parser.take_numeric_column(test_double, line);
    CHECK(test_double == Approx(1e2));
    CHECK(line.empty());
    
    line = parser.read_line();
    CHECK(line == "1,2 ,3;4; 5 ,6,7");
    CHECK(parser.take_numeric_column<int>(line, ',') == 1);
    CHECK(parser.take_numeric_column<size_t>(line, ',') == 2ULL);
    auto semicolon_list = parser.take_column(line, ',');
    CHECK(semicolon_list == "3;4; 5");
    CHECK(parser.take_numeric_column<size_t>(semicolon_list, ';') == 3);
    CHECK(parser.take_numeric_column<float>(semicolon_list, ';') == 4.0);
    CHECK(parser.take_numeric_column<float>(semicolon_list) == 5.0);
    CHECK(semicolon_list == "");

    CHECK(cpposu::try_take_numeric_column(line) == 6);
    CHECK(cpposu::try_take_numeric_column(line) == 7.0);
    CHECK(line == "");
    CHECK(cpposu::try_take_numeric_column(line) == std::nullopt);
    CHECK(line == "");
}