//
// Created by alex on 9/20/26.
//

#include "PartCsv.h"

#include <charconv>
#include <fstream>
#include <sstream>

namespace bufman {
    namespace {

    bool parse_integer(const std::string& text, int& value) {
        if (text.empty()) {
            return false;
        }
        const char* first = text.data();
        const char* last = first + text.size();
        const auto result = std::from_chars(first, last, value);
        return result.ec == std::errc{} && result.ptr == last;
    }
    bool parse_float(const std::string& text, float& value) {
    if (text.empty()) {
        return false;
    }
    const char* first = text.data();
    const char* last = first + text.size();
    const auto result = std::from_chars(first, last, value);
    return result.ec == std::errc{} && result.ptr == last;
    }


    bool parse_line(const std::string& line, Part& part, std::string& error) {
        std::stringstream input(line);
        std::string part_id_text;
        std::string name;
        std::string weight_text;
        std::string color_text;
        std::string price_text;
        std::string material;
        std::string extra;
        if (!std::getline(input, part_id_text, ',') ||
            !std::getline(input, name, ',') ||
            !std::getline(input, weight_text, ',') ||
            !std::getline(input, color_text, ',') ||
            !std::getline(input, price_text, ',') ||
            !std::getline(input, material, ',') ||
            std::getline(input, extra, ','))
        {
            error = "e  xpected exactly six comma-separated fields";
            return false;
        }
        int part_id = 0;
        float weight=0;
        int color=0;
        float price=0;
        if (!parse_integer(part_id_text, part_id) || part_id <= 0) {
            error = "part_id must be a positive integer";
            return false;
        }
        if (!parse_float(weight_text, weight) || weight < 0) {
            error = "part_weight must be a nonnegative float";
            return false;
        }

        if (!parse_float(price_text, price) || price < 0) {
            error = "part_price must be a nonnegative float";
            return false;
        }

        if (!parse_integer(color_text, color) || color <0 || color>5) {
            error = "part_color must be an integer in the inclusive range 0 through 5";
            return false;
        }


        if (name.size() > 9) {
            error = "part_name must contain at most 9 characters";
            return false;
        }

        if (material.size() > 9) {
            error = "part_material must contain at most 9 characters";
            return false;
        }

        part = Part{};
        part.part_id = part_id;
        name.copy(part.part_name, name.size());
        part.part_weight=weight;
        part.part_color=color;
        part.part_price=price;
        material.copy(part.part_material,material.size());
        return true;


    }

    }

    PartLoadResult load_parts(const std::string& path, std::ostream& diagnostics) {
        PartLoadResult result;
        std::ifstream input(path);

        if (!input) {
            diagnostics <<"cannot open CSV file: " << path <<'\n';
            //Preguntar al profesor por que si no se pudo abrir se pone 1
            result.skipped=1;
            return result;
        }
        std::string line;
        std::size_t line_number=0;
        while (std::getline(input, line)) {
            ++line_number;
            if (line.empty()) {
                ++result.skipped;
                diagnostics<<"line"<< line_number<<": blank line\n";
                continue;
            }
            Part part{};
            std::string error;
            if (!parse_line(line, part, error)) {
                ++result.skipped;
                diagnostics <<"line" <<line_number<<": "<< error <<'\n';
                continue;
            }
            result.parts.push_back(part);
        }
        return result;


    }
}
