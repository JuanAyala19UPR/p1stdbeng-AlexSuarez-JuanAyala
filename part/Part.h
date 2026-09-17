//
// Created by Manuel Rodriguez on 9/14/26.
//

#ifndef P1DBENG_PART_H
#define P1DBENG_PART_H
enum PartColor {
    PART_COLOR_WHITE,
    PART_COLOR_BLACK,
    PART_COLOR_RED,
    PART_COLOR_GREEN,
    PART_COLOR_YELLOW,
    PART_COLOR_BLUE
};
typedef struct Part {
    int part_id;
    char part_name[10];
    float part_weight;
    int part_color;
    float part_price;
    char part_material[10];
} Part;
#endif //P1DBENG_PART_H
