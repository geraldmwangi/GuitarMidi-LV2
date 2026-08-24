#include <guitarmidi/fretboardrepresentation.hpp>

GuitarMidi::FretBoardRepresentation::FretBoardRepresentation()
{
m_strings.push_back(GuitarString({{0, 82}, {1, 87}, {2, 92}, {3, 98}, {4, 104}}));
m_strings.push_back(GuitarString({{5, 110}, {6, 117}, {7, 123}, {8, 131}, {9, 139}}));
m_strings.push_back(GuitarString({{10, 147}, {11, 156}, {12, 165}, {13, 175}, {14, 185}}));
m_strings.push_back(GuitarString({{15, 196}, {16, 208}, {17, 220}, {18, 233}}));
m_strings.push_back(GuitarString({{19, 247}, {20, 262}, {21, 277}, {22, 294}, {23, 311}}));
m_strings.push_back(GuitarString({{24, 329}, {25, 349}, {26, 370}, {27, 392}, {28, 415}, {29, 440}, {30, 466}, {31, 494}, {32, 523}, {33, 554}, {34, 587}, {35, 622}, {36, 659}}));
}